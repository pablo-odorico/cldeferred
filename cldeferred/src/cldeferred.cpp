#include "cldeferred.h"

const GLenum CLDeferred::diffuseSpecFormat;
const GLenum CLDeferred::normalsFormat;
const GLenum CLDeferred::depthFormat;
const GLenum CLDeferred::depthTestFormat;

CLDeferred::CLDeferred(QSize maxSize)
    : CLGLWindow()
    , firstPassProgram(0), outputProgram(0)
    , scene(0), painter(0), maxSize(maxSize)
    , fpsFrameCount(0), fpsLastTime(0)
{
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
    glGenTextures(1, &outputTex);
    glBindTexture(GL_TEXTURE_2D, outputTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, maxSize.width(), maxSize.height(), 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // Load scene
    scene= QGLAbstractScene::loadScene("models/untitled/untitled.obj");

    startRenderTimer(30);
    sceneTime.start();
    lastRenderTime= sceneTime.nsecsElapsed();
    fpsLastTime= sceneTime.nsecsElapsed();

    camera.setPosition(5, 5, 5);
    camera.lookAt(0, 0, 0);
    camera.setMoveSpeed(5);
}

void CLDeferred::initializeCL()
{
    cl_int error;
    outputBuffer= clCreateFromGLTexture2D(clCtx(), CL_MEM_WRITE_ONLY,
                                          GL_TEXTURE_2D, 0, outputTex, &error);
    if(checkCLError(error, "clCreateFromGLTexture2D"))
        return;

    if(!loadKernel(clCtx(), &deferredPassKernel, clDevice(),
                   "kernels/deferredPass.cl", "deferredPass")) {
        qDebug() << "Error loading kernel.";
        return;
    }
}

void CLDeferred::resizeGL(QSize size)
{
    qDebug() << "Resize GL" << size;

    QList<GLenum> colorFormats= QList<GLenum>() << diffuseSpecFormat << normalsFormat << depthFormat;
    gBuffer.init(clCtx(), size, colorFormats, depthTestFormat);

    glViewport(0, 0, size.width(), size.height());

    camera.setPerspective(60.0f, (float)size.width()/size.height(), 0.01f, 50.0f);
}

void CLDeferred::renderGL()
{
    const qint64 now= sceneTime.nsecsElapsed();
    QVector<qint64> times(3);

    camera.move((now - lastRenderTime)/1000.0f);

    // 1st pass
    renderToGBuffer();
    times[0]= sceneTime.nsecsElapsed();

    // 2nd pass
    deferredPass();
    times[1]= sceneTime.nsecsElapsed();

    // Draw output texture
    drawOutput();
    times[2]= sceneTime.nsecsElapsed();

    const qint64 fpsElapsed= now - fpsLastTime;
    if(fpsElapsed > 5e9) {
        const float elapsed1st = (times[0] - now     ) / 1e6f;
        const float elapsed2nd = (times[1] - times[0]) / 1e6f;
        const float elapsedDraw= (times[2] - times[1]) / 1e6f;
        const float totalTime= elapsed1st + elapsed2nd + elapsedDraw;

        qDebug("");
        qDebug("  FPS       : %.01f Hz.", fpsFrameCount/(fpsElapsed/1e9d));
        qDebug("  1st pass  : %.02f ms.", elapsed1st);
        qDebug("  2nd pass  : %.02f ms.", elapsed2nd);
        qDebug("  Draw quad : %.02f ms.", elapsedDraw);
        qDebug("  Total time: %.02f ms. (Max. %.01f FPS)", totalTime, 1000/totalTime);

        fpsLastTime= now;
        fpsFrameCount= 0;
    }

    fpsFrameCount++;
    lastRenderTime= now;
}

void CLDeferred::renderToGBuffer()
{
    gBuffer.bind();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    firstPassProgram->bind();
    firstPassProgram->setUniformValue("modelMatrix", modelMatrix);
    firstPassProgram->setUniformValue("modelITMatrix", modelMatrix.inverted().transposed());
    firstPassProgram->setUniformValue("viewMatrix", camera.viewMatrix());
    firstPassProgram->setUniformValue("projMatrix", camera.projMatrix());
    firstPassProgram->setUniformValue("mvpMatrix", camera.projMatrix() * camera.viewMatrix() * modelMatrix);

    // Draw scene
    scene->mainNode()->draw(painter);

    firstPassProgram->release();

    gBuffer.unbind();
}

void CLDeferred::deferredPass()
{
    cl_int error;

    error= clEnqueueAcquireGLObjects(clQueue(), 1, &outputBuffer, 0, 0, 0);
    if(checkCLError(error, "clEnqueueAcquireGLObjects"))
        return;
    gBuffer.enqueueAquireBuffers(clQueue());

    // Work group and NDRange
    size_t workGroupSize[2] = { 16, 16 };
    size_t ndRangeSize[2];
    ndRangeSize[0]= roundUp(width() , workGroupSize[0]);
    ndRangeSize[1]= roundUp(height(), workGroupSize[1]);

    cl_mem gbDiffuseSpec= gBuffer.getColorBuffer(0);
    cl_mem gbNormals= gBuffer.getColorBuffer(1);
    cl_mem gbDepth= gBuffer.getColorBuffer(2);

    // Launch kernel
    error  = clSetKernelArg(deferredPassKernel, 0, sizeof(cl_mem), (void*)&gbDiffuseSpec);
    error |= clSetKernelArg(deferredPassKernel, 1, sizeof(cl_mem), (void*)&gbNormals);
    error |= clSetKernelArg(deferredPassKernel, 2, sizeof(cl_mem), (void*)&gbDepth);
    error |= clSetKernelArg(deferredPassKernel, 3, sizeof(cl_mem), (void*)&outputBuffer);
    error |= clEnqueueNDRangeKernel(clQueue(), deferredPassKernel, 2, NULL,
                                    ndRangeSize, workGroupSize, 0, NULL, NULL);
    checkCLError(error, "outputKernel");

    error= clEnqueueReleaseGLObjects(clQueue(), 1, &outputBuffer, 0, 0, 0);
    if(checkCLError(error, "clEnqueueReleaseGLObjects"))
        return;
    gBuffer.enqueueReleaseBuffers(clQueue());

    // Wait until the kernel finishes and the GL resources are released
    error= clFinish(clQueue());
    checkCLError(error, "clFinish");
}

void CLDeferred::drawOutput()
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

//
// User input
//

void CLDeferred::grabbedMouseMoveEvent(QPointF delta)
{
    const float sensibility= 100;
    camera.setDeltaYaw( -delta.x() * sensibility);
    camera.setDeltaPitch(delta.y() * sensibility);
}

void CLDeferred::grabbedKeyPressEvent(int key)
{
    if(key == Qt::Key_W) camera.toggleMovingDir(Camera::Front, true);
    if(key == Qt::Key_S) camera.toggleMovingDir(Camera::Back , true);
    if(key == Qt::Key_D) camera.toggleMovingDir(Camera::Right, true);
    if(key == Qt::Key_A) camera.toggleMovingDir(Camera::Left , true);
    if(key == Qt::Key_Shift) camera.setMoveSpeed(camera.moveSpeed() * 2.0f);
}

void CLDeferred::grabbedKeyReleaseEvent(int key)
{
    if(key == Qt::Key_W) camera.toggleMovingDir(Camera::Front, false);
    if(key == Qt::Key_S) camera.toggleMovingDir(Camera::Back , false);
    if(key == Qt::Key_D) camera.toggleMovingDir(Camera::Right, false);
    if(key == Qt::Key_A) camera.toggleMovingDir(Camera::Left , false);
    if(key == Qt::Key_Shift) camera.setMoveSpeed(camera.moveSpeed() / 2.0f);
}
