#include "cldeferred.h"

const GLenum CLDeferred::diffuseSpecFormat;
const GLenum CLDeferred::normalsFormat;
const GLenum CLDeferred::depthFormat;

CLDeferred::CLDeferred(QSize maxSize)
    : firstPassProgram(0), painter(0)
{
    this->maxSize= maxSize;
    frameId= 0;
}

void CLDeferred::initializeGL()
{
    qDebug() << "Initialize GL";

    painter= new QGLPainter(this);

    // General OpenGL config
    glEnable(GL_DEPTH_TEST);
    //glCullFace(GL_BACK);
    //glEnable(GL_CULL_FACE);

    // 1st pass init
    firstPassProgram= new QOpenGLShaderProgram(this);
    firstPassProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, "shaders/firstpass.vert");
    firstPassProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, "shaders/firstpass.frag");
    firstPassProgram->link();

    outputProgram= new QOpenGLShaderProgram(this);
    outputProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, "shaders/outputTex.vert");
    outputProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, "shaders/outputTex.frag");
    outputProgram->link();

    // 2nd pass init
    // Create output texture
    QImage tempImg(maxSize.width(), maxSize.height(), QImage::Format_RGB32);
    tempImg.fill(Qt::darkGray);
    glGenTextures(1, &outputTex);
    glBindTexture(GL_TEXTURE_2D, outputTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tempImg.width(), tempImg.height(), 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, static_cast<GLvoid*>(tempImg.bits()));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // Load scene
    scene= QGLAbstractScene::loadScene("models/untitled/untitled.obj");
    //scene= QGLAbstractScene::loadScene("models/a10/untitled.obj");

    startRenderTimer(30);
}

void CLDeferred::initializeCL()
{
    cl_int error;

    outputBuffer= clCreateFromGLTexture2D(clCtx(), CL_MEM_WRITE_ONLY,
                                          GL_TEXTURE_2D, 0, outputTex, &error);
    if(checkCLError(error, "clCreateFromGLTexture2D"))
        return;

    if(!loadKernel(clCtx(), &outputKernel, clDevice(), "kernels/output.cl", "outputKernel")) {
        qDebug() << "Error loading kernel.";
        return;
    }

    gBuffer.setupCL(clCtx(), clQueue());
}

void CLDeferred::resizeGL(QSize size)
{
    qDebug() << "Resize GL" << size;

    QList<GLenum> colorFormats= QList<GLenum>() << diffuseSpecFormat << normalsFormat << depthFormat;
    gBuffer.init(size, colorFormats, depthTestFormat);
    // Set G-Buffer viewport
    glViewport(0, 0, size.width(), size.height());

    gBuffer.unbind();
    // Set default FBO viewport
    glViewport(0, 0, size.width(), size.height());
}

void CLDeferred::renderGL()
{
    // 1st pass, fill the geometry buffer
    renderToGBuffer();

    // 2nd pass

    // Update output texture from the CL output buffer
    updateOutputTex();

    // Draw output texture
    drawOutputTex();

    frameId++;
}

void CLDeferred::renderToGBuffer()
{
    gBuffer.bind();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    projMatrix.setToIdentity();
    projMatrix.perspective(60.0f, (float)width()/height(), 1.0f, 15.0f);

    viewMatrix.setToIdentity();
    viewMatrix.lookAt(QVector3D(5,5,5), QVector3D(0,0,0), QVector3D(0,1,0));

    static float rot= 1.0f;
    modelMatrix.setToIdentity();
    modelMatrix.rotate(rot, 0,1,0);
    rot++;

    firstPassProgram->bind();
    firstPassProgram->setUniformValue("modelMatrix", modelMatrix);
    firstPassProgram->setUniformValue("modelITMatrix", modelMatrix.inverted().transposed());
    firstPassProgram->setUniformValue("viewMatrix", viewMatrix);
    firstPassProgram->setUniformValue("projMatrix", projMatrix);
    firstPassProgram->setUniformValue("mvpMatrix", projMatrix * viewMatrix * modelMatrix);

    scene->mainNode()->draw(painter);

    firstPassProgram->release();

    gBuffer.unbind();
}

void CLDeferred::updateOutputTex()
{
    cl_int error;

    error= clEnqueueAcquireGLObjects(clQueue(), 1, &outputBuffer, 0, 0, 0);
    if(checkCLError(error, "clEnqueueAcquireGLObjects"))
        return;
    gBuffer.enqueueAquireBuffers();

    // Work group and NDRange
    int outputWidth= width();
    int outputHeight= height();
    size_t workGroupSize[2] = { 16, 16 };
    size_t ndRangeSize[2];
    ndRangeSize[0]= roundUp(outputWidth, workGroupSize[0]);
    ndRangeSize[1]= roundUp(outputHeight, workGroupSize[1]);

    cl_mem gbDiffuseSpec= gBuffer.getColorBuffer(0);
    cl_mem gbNormals= gBuffer.getColorBuffer(1);
    cl_mem gbDepth= gBuffer.getColorBuffer(2);

    /*
    static int first= 0;
    if(first == 10) {
        float* d= new float[gBuffer.width() * gBuffer.height()];
        if(!d)
            qDebug() << "Alloc error";
        memset(d, 0, sizeof(d));

        size_t origin[3] = {0, 0, 0};
        size_t region[3] = {gBuffer.width(), gBuffer.height(), 1};
        error= clEnqueueReadImage(clQueue(), gbDepth, CL_TRUE, origin, region, 0, 0, d, 0, NULL, NULL);
        if(checkCLError(error, "read image"))
            return;

        QFile file("depth.txt");
        file.open(QIODevice::WriteOnly | QIODevice::Text);
        QTextStream out(&file);
        for(int y=0; y<gBuffer.height(); y++) {
            for(int x=0; x<gBuffer.width(); x++) {
                const float z= d[x + y * gBuffer.width()];
                if(!z) continue;

                const float ndcX= (2.0f * x)/gBuffer.width() - 1.0f;
                const float ndcY= (2.0f * y)/gBuffer.height() - 1.0f;
                const float ndcZ= 2.0f * z - 1.0f;

                const float clipW= projMatrix(2,3) / (ndcZ - projMatrix(2,2)/projMatrix(3,2));
                const float clipX= ndcX * clipW;
                const float clipY= ndcY * clipW;
                const float clipZ= ndcZ * clipW;
                QVector4D clipPos(clipX, clipY, clipZ, clipW);

                float eyeZ = projMatrix(2,3) / ((projMatrix(3,2) * ndcZ) - projMatrix(2,2));
                const QVector3D eyeDirection(ndcX, ndcY, -1);
                QVector4D vVector= (eyeDirection * eyeZ).toVector4D();
                vVector.setW(1);

                //QVector4D vVector= projMatrix.inverted() * clipPos;

                QVector4D wVector= (viewMatrix * modelMatrix).inverted() * vVector;

                out << wVector.x() << " " << wVector.y() << " " << wVector.z() << "\n";
            }
        }
        file.close();
    }
    first++;
    */

    // Launch kernel
    error  = clSetKernelArg(outputKernel, 0, sizeof(   int), (void*)&outputWidth);
    error |= clSetKernelArg(outputKernel, 1, sizeof(   int), (void*)&outputHeight);
    error |= clSetKernelArg(outputKernel, 2, sizeof(cl_mem), (void*)&gbDiffuseSpec);
    error |= clSetKernelArg(outputKernel, 3, sizeof(cl_mem), (void*)&gbNormals);
    error |= clSetKernelArg(outputKernel, 4, sizeof(cl_mem), (void*)&gbDepth);
    error |= clSetKernelArg(outputKernel, 5, sizeof(cl_mem), (void*)&outputBuffer);
    error |= clEnqueueNDRangeKernel(clQueue(), outputKernel, 2, NULL, ndRangeSize, workGroupSize, 0, NULL, NULL);
    checkCLError(error, "outputKernel");

    error= clEnqueueReleaseGLObjects(clQueue(), 1, &outputBuffer, 0, 0, 0);
    if(checkCLError(error, "clEnqueueReleaseGLObjects"))
        return;
    gBuffer.enqueueReleaseBuffers();

    // Sync
    clFinish(clQueue());
    checkCLError(error, "clFinish");
}

void CLDeferred::drawOutputTex()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    outputProgram->bind();

    glBindTexture(GL_TEXTURE_2D, outputTex);
    const float uMax= (float)width() / maxSize.width();
    const float vMax= (float)height() / maxSize.height();
    glBegin(GL_QUADS);
        glVertex4f(-1,-1,    0,    0);
        glVertex4f( 1,-1, uMax,    0);
        glVertex4f( 1, 1, uMax, vMax);
        glVertex4f(-1, 1,    0, vMax);
    glEnd();

    outputProgram->release();
}
