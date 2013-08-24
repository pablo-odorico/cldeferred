#include "cldeferred.h"

const GLenum CLDeferred::diffuseSpecFormat;
const GLenum CLDeferred::normalsFormat;
const GLenum CLDeferred::depthFormat;
const GLenum CLDeferred::depthTestFormat;

CLDeferred::CLDeferred(QSize maxSize) :
    firstPassProgram(0), outputProgram(0),
    scene(0), painter(0)
{
    this->maxSize= maxSize;
}

void CLDeferred::initializeGL()
{
    qDebug() << "Initialize GL";

    painter= new QGLPainter(this);

    // General OpenGL config
    glEnable(GL_DEPTH_TEST);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);

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
    static int frameId= 0;
    frameId++;
    bool showTime= (frameId % 20) == 0;

    QElapsedTimer time;
    time.start();

    // 1st pass, fill the geometry buffer
    renderToGBuffer();
    if(showTime) qDebug() << "GBuff" << time.nsecsElapsed()/1e6f;

    // 2nd pass

    // Update output texture from the CL output buffer
    updateOutputTex();
    if(showTime) qDebug() << "Update" << time.nsecsElapsed()/1e6f;

    // Draw output texture
    drawOutputTex();
    if(showTime) qDebug() << "DrawTex" << time.nsecsElapsed()/1e6f;
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
    size_t workGroupSize[2] = { 16, 16 };
    size_t ndRangeSize[2];
    ndRangeSize[0]= roundUp(width(), workGroupSize[0]);
    ndRangeSize[1]= roundUp(height(), workGroupSize[1]);

    cl_mem gbDiffuseSpec= gBuffer.getColorBuffer(0);
    cl_mem gbNormals= gBuffer.getColorBuffer(1);
    cl_mem gbDepth= gBuffer.getColorBuffer(2);

    // Launch kernel
    error  = clSetKernelArg(outputKernel, 0, sizeof(cl_mem), (void*)&gbDiffuseSpec);
    error |= clSetKernelArg(outputKernel, 1, sizeof(cl_mem), (void*)&gbNormals);
    error |= clSetKernelArg(outputKernel, 2, sizeof(cl_mem), (void*)&gbDepth);
    error |= clSetKernelArg(outputKernel, 3, sizeof(cl_mem), (void*)&outputBuffer);
    error |= clEnqueueNDRangeKernel(clQueue(), outputKernel, 2, NULL, ndRangeSize, workGroupSize, 0, NULL, NULL);
    checkCLError(error, "outputKernel");

    error= clEnqueueReleaseGLObjects(clQueue(), 1, &outputBuffer, 0, 0, 0);
    if(checkCLError(error, "clEnqueueReleaseGLObjects"))
        return;
    gBuffer.enqueueReleaseBuffers();

    // Sync
    error= clFinish(clQueue());
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
