#include "cldeferred.h"

#include "clutils.h"

using namespace CLUtils;

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

    connect(&renderTimer, SIGNAL(timeout()), this, SLOT(renderLater()));
    renderTimer.start(1000/30);

    // General OpenGL config
    glEnable(GL_DEPTH_TEST);

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
    scene = QGLAbstractScene::loadScene("models/untitled/untitled.obj");
}

void CLDeferred::initializeCL()
{
    cl_int error;
    outputBuffer= clCreateFromGLTexture2D(clCtx(), CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, outputTex, &error);
    if(checkError(error, "clCreateFromGLTexture2D"))
        return;

    if(!loadKernel(clCtx(), &outputKernel, clDevice(), "kernels/output.cl", "outputKernel")) {
        qDebug() << "Error loading kernel.";
        return;
    }
}

void CLDeferred::resizeGL(QSize size)
{
    qDebug() << "Resize GL" << size;

    // Create/resize FBO
    gBuffer.init(size);
    // Set viewport
    glViewport(0, 0, size.width(), size.height());

    gBuffer.unbind();
    glViewport(0, 0, size.width(), size.height());
}

void CLDeferred::renderGL()
{
    // 1st pass
    renderGeometryBuffer();

    // 2nd pass

    // Final stage: render output texture
    updateOutputTex();

    drawOutputTex();

    frameId++;
}

void CLDeferred::renderGeometryBuffer()
{
    gBuffer.bind();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    projMatrix.setToIdentity();
    projMatrix.perspective(60.0f, (float)width()/height(), 0.5f, 50.0f);

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
    if(checkError(error, "clEnqueueAcquireGLObjects"))
        return;

    // Work group and NDRange
    int outputWidth= width();
    int outputHeight= height();
    size_t workGroupSize[2] = { 16, 16 };
    size_t ndRangeSize[2];
    ndRangeSize[0]= roundUp(outputWidth, workGroupSize[0]);
    ndRangeSize[1]= roundUp(outputHeight, workGroupSize[1]);

    // Launch kernel
    error  = clSetKernelArg(outputKernel, 0, sizeof(int), (void*)&outputWidth);
    error |= clSetKernelArg(outputKernel, 1, sizeof(int), (void*)&outputHeight);
    error |= clSetKernelArg(outputKernel, 2, sizeof(int), (void*)&frameId);
    error |= clSetKernelArg(outputKernel, 3, sizeof(cl_mem), (void*)&outputBuffer);
    error |= clEnqueueNDRangeKernel(clQueue(), outputKernel, 2, NULL, ndRangeSize, workGroupSize, 0, NULL, NULL);
    checkError(error, "outputKernel");

    error= clEnqueueReleaseGLObjects(clQueue(), 1, &outputBuffer, 0, 0, 0);
    if(checkError(error, "clEnqueueReleaseGLObjects"))
        return;

    // Sync
    clFinish(clQueue());
    checkError(error, "clFinish");
}

void CLDeferred::drawOutputTex()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    outputProgram->bind();

    glBindTexture(GL_TEXTURE_2D, outputTex);
    const float uMax= (float)width() / maxSize.width();
    const float vMax= (float)height() / maxSize.height();
    glBegin(GL_QUADS);
        glVertex4f(-1,-1,    0, vMax);
        glVertex4f( 1,-1, uMax, vMax);
        glVertex4f( 1, 1, uMax,    0);
        glVertex4f(-1, 1,    0,    0);
    glEnd();

    outputProgram->release();
}
