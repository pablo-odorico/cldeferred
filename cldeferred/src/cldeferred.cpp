#include "cldeferred.h"
#include <cassert>
#include "debug.h"

const GLenum CLDeferred::diffuseSpecFormat;
const GLenum CLDeferred::normalsFormat;
const GLenum CLDeferred::depthFormat;
const GLenum CLDeferred::depthTestFormat;

CLDeferred::CLDeferred(QSize maxSize)
    : CLGLWindow()
    , firstPassProgram(0), outputProgram(0)
    , maxSize(maxSize)
    , fpsFrameCount(0), fpsLastTime(0)
{
}

void CLDeferred::initializeGL()
{
    // General OpenGL config
    glEnable(GL_DEPTH_TEST);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    // GL painter config
    glPainter()->setStandardEffect(QGL::LitModulateTexture2D);

    // 1st pass init
    firstPassProgram= new QOpenGLShaderProgram(this);
    firstPassProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/firstpass.vert");
    firstPassProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/firstpass.frag");
    firstPassProgram->link();

    outputProgram= new QOpenGLShaderProgram(this);
    outputProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/outputTex.vert");
    outputProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/outputTex.frag");
    outputProgram->link();

    // 2nd pass init
    // Create output texture
    glGenTextures(1, &outputTex);
    glBindTexture(GL_TEXTURE_2D, outputTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, maxSize.width(), maxSize.height(), 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

void CLDeferred::initializeCL()
{
    cl_int error;
    outputTexBuffer= clCreateFromGLTexture2D(
                clCtx(), CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, outputTex, &error);
    if(checkCLError(error, "clCreateFromGLTexture2D"))
        return;

    if(!loadKernel(clCtx(), &deferredPassKernel, clDevice(),
                   ":/kernels/deferredPass.cl", "deferredPass",
                   "-I../res/kernels/ -Werror")) {
        debugError("Error loading kernel.");
        return;
    }
}

void CLDeferred::finalizeInit()
{
    scene.init(glPainter(), clCtx());

    if(!scene.loadScene("models/untitled/untitled2.obj"))
        debugError("Could not load scene!");
    scene.camera().lookAt(QVector3D(-8, .5f, -8), QVector3D(0, 0, 0));
    scene.camera().setMoveSpeed(5);

    SpotLight* spotLight= new SpotLight();
    spotLight->lookAt(QVector3D(10, 10, 10), QVector3D(0, 0, 0));
    spotLight->enableShadows(true);    
    spotLight->setupShadowMap(clCtx(), QSize(512,512));
    spotLight->setParams(30, 1, 1/30.0f, 1.0f);
    scene.lightManager().addSpotLight(spotLight);

    startRenderTimer(30);
    sceneTime.start();
    lastRenderTime= sceneTime.nsecsElapsed();
    fpsLastTime= sceneTime.nsecsElapsed();
}

void CLDeferred::resizeGL(QSize size)
{
    debugMsg("Resize to %d x %d.", size.width(), size.height());
    assert(size.width() <= maxSize.width());
    assert(size.height() <= maxSize.height());

    bool ok;

    QList<GLenum> colorFormats= QList<GLenum>() << diffuseSpecFormat << normalsFormat << depthFormat;
    ok= gBuffer.resize(clCtx(), size, colorFormats, depthTestFormat);
    if(!ok)
        debugError("Error initializing G-Buffer FBO.");

    ok= occlusionBuffer.resize(clCtx(), clDevice(), size);
    if(!ok)
        debugError("Error initializing occlusion buffer.");

    scene.camera().setPerspective(60.0f, (float)size.width()/size.height(), 0.1f, 5000.0f);
}

void CLDeferred::renderGL()
{
    const qint64 now= sceneTime.nsecsElapsed();
    QVector<qint64> times(4);

    // Move camera
    scene.camera().move((now - lastRenderTime)/1000.0f);

    static float a= 0;
    a += 0.1f;
    SpotLight* ssl= scene.lightManager().spotLight(0);
    ssl->lookAt(QVector3D(10,10+2*sinf(a),10), QVector3D(0,0,0));

    // Update OpenCL structs
    scene.updateStructsCL(clQueue());

    // 1st pass
    renderToGBuffer();
    times[0]= sceneTime.nsecsElapsed();

    // Update occlusion buffer
    updateShadowMaps();
    times[1]= sceneTime.nsecsElapsed();

    // 2nd pass
    deferredPass();
    times[2]= sceneTime.nsecsElapsed();

    static int frame= 0;
    frame++;
    if(frame==55) {
        QPixmap::grabWindow(winId()).save("screen.ppm");
    }

    // Draw output texture
    drawOutput();
    times[3]= sceneTime.nsecsElapsed();

    const qint64 fpsElapsed= now - fpsLastTime;
    if(fpsElapsed >4e9) {
        const float elapsed1st = (times[0] - now     ) / 1e6f;
        const float elapsedOccl= (times[1] - times[0]) / 1e6f;
        const float elapsed2nd = (times[2] - times[1]) / 1e6f;
        const float elapsedDraw= (times[3] - times[2]) / 1e6f;
        const float totalTime= elapsed1st + elapsedOccl + elapsed2nd + elapsedDraw;

        qDebug(" ");
        qDebug("FPS   : %.01f Hz. (Max. %.01f Hz.)", fpsFrameCount/(fpsElapsed/1e9d), 1000/totalTime);
        qDebug("Times : G-Buffer | Occlusion | Deferred  | Draw quad | TOTAL");
        qDebug("        %2.02f ms. | %2.02f ms.  | %2.02f ms.  | %2.02f ms.  | %.02f ms.",
               elapsed1st, elapsedOccl, elapsed2nd, elapsedDraw, totalTime);

        fpsLastTime= now;
        fpsFrameCount= 0;
    }

    fpsFrameCount++;
    lastRenderTime= now;
}

//
// Render stages
//

void CLDeferred::renderToGBuffer()
{
    gBuffer.bind();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    scene.draw(firstPassProgram, Scene::MVPMatrix | Scene::ModelITMatrix);

    gBuffer.unbind();
}

void CLDeferred::updateShadowMaps()
{
    // Update the shadow maps of all lights
    scene.updateShadowMaps();

    LightManager& lm= scene.lightManager();

    cl_mem camStruct= scene.camera().structCL();
    cl_mem camDepth= gBuffer.aquireColorBuffers(clQueue()).at(2);
    cl_mem spotLightStructs= lm.spotStructs();
    QVector<cl_mem> spotLightDepthImgs= lm.aquireSpotDepths(clQueue());

    bool ok;
    ok= occlusionBuffer.update(
                clQueue(), camStruct, camDepth,
                spotLightStructs, spotLightDepthImgs,
                gBuffer.size());
    if(!ok)
        debugError("Error updating occlusion buffer.");

    lm.releaseSpotDephts(clQueue());
    gBuffer.releaseColorBuffers(clQueue());

/*
    static int con= 0;
    con++;
    if(con==10) {
        lm.spotLight(0)->shadowMapFBO().diffuseToImage().save("depth.png");

        cl_float4 data[gBuffer.width() * gBuffer.height()];

        cl_int error;
        error= clEnqueueReadBuffer(clQueue(), occlusionBuffer.buffer(), CL_TRUE, 0,
                                   occlusionBuffer.bufferBytes(), data, 0, NULL, NULL);
        if(checkCLError(error, "clEnqueueReadBuffer"))
            return;
        clFinish(clQueue());

        QMatrix4x4 m= (scene.camera().projMatrix() * scene.camera().viewMatrix()).inverted();

        QFile file("out.txt");
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
            return;
        QTextStream out(&file);

        for(int y=0; y<gBuffer.height(); y+=1) {
            for(int x=0; x<gBuffer.width(); x+=1) {
                cl_float4 pos= data[x + y * gBuffer.width()];
                QVector4D vec(pos.x, pos.y, pos.z, pos.w);

                //vec= m * vec;
                if(pos.z != -1.0f)
                    out << vec.x() << " " << vec.y() << " " << vec.z() << " " << "\n";
            }
        }
        file.close();
    }
*/

    checkCLError(clFinish(clQueue()), "clFinish");
}

void CLDeferred::deferredPass()
{
    cl_int error;

    error= clEnqueueAcquireGLObjects(clQueue(), 1, &outputTexBuffer, 0, 0, 0);
    if(checkCLError(error, "clEnqueueAcquireGLObjects"))
        return;
    QVector<cl_mem> gBufferChannels= gBuffer.aquireColorBuffers(clQueue());
    if(gBufferChannels.count() != 3) {
        debugError("Wrong number of gbuffer channels %d", gBufferChannels.count());
        return;
    }

    // Work group and NDRange
    size_t workGroupSize[2] = { 16, 16 };
    size_t ndRangeSize[2];
    ndRangeSize[0]= roundUp(gBuffer.width() , workGroupSize[0]);
    ndRangeSize[1]= roundUp(gBuffer.height(), workGroupSize[1]);

    cl_mem gbDiffuseSpec= gBufferChannels[0];
    cl_mem gbNormals= gBufferChannels[1];
    cl_mem gbDepth= gBufferChannels[2];
    cl_mem oBuffer= occlusionBuffer.buffer();
    cl_mem cameraStruct= scene.camera().structCL();
    cl_mem spotLightStructs= scene.lightManager().spotStructs();

    // Launch kernel
    error  = clSetKernelArg(deferredPassKernel, 0, sizeof(cl_mem), (void*)&gbDiffuseSpec);
    error |= clSetKernelArg(deferredPassKernel, 1, sizeof(cl_mem), (void*)&gbNormals);
    error |= clSetKernelArg(deferredPassKernel, 2, sizeof(cl_mem), (void*)&gbDepth);
    error |= clSetKernelArg(deferredPassKernel, 3, sizeof(cl_mem), (void*)&oBuffer);
    error |= clSetKernelArg(deferredPassKernel, 4, sizeof(cl_mem), (void*)&outputTexBuffer);
    error |= clSetKernelArg(deferredPassKernel, 5, sizeof(cl_mem), (void*)&cameraStruct);
    error |= clSetKernelArg(deferredPassKernel, 6, sizeof(cl_mem), (void*)&spotLightStructs);
    error |= clEnqueueNDRangeKernel(clQueue(), deferredPassKernel, 2, NULL,
                                    ndRangeSize, workGroupSize, 0, NULL, NULL);
    checkCLError(error, "outputKernel");

    error= clEnqueueReleaseGLObjects(clQueue(), 1, &outputTexBuffer, 0, 0, 0);
    if(checkCLError(error, "clEnqueueReleaseGLObjects"))
        return;
    gBuffer.releaseColorBuffers(clQueue());

    // Wait until the kernel finishes and the GL resources are released
    error= clFinish(clQueue());
    checkCLError(error, "clFinish");
}

void CLDeferred::drawOutput()
{
    glViewport(0, 0, width(), height());

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
    scene.camera().setDeltaYaw( -delta.x() * sensibility);
    scene.camera().setDeltaPitch(delta.y() * sensibility);
}

void CLDeferred::grabbedKeyPressEvent(int key)
{
    if(key == Qt::Key_W) scene.camera().toggleMovingDir(Camera::Front, true);
    if(key == Qt::Key_S) scene.camera().toggleMovingDir(Camera::Back , true);
    if(key == Qt::Key_D) scene.camera().toggleMovingDir(Camera::Right, true);
    if(key == Qt::Key_A) scene.camera().toggleMovingDir(Camera::Left , true);
    if(key == Qt::Key_Shift) scene.camera().setMoveSpeed(scene.camera().moveSpeed() * 2.0f);
}

void CLDeferred::grabbedKeyReleaseEvent(int key)
{
    if(key == Qt::Key_W) scene.camera().toggleMovingDir(Camera::Front, false);
    if(key == Qt::Key_S) scene.camera().toggleMovingDir(Camera::Back , false);
    if(key == Qt::Key_D) scene.camera().toggleMovingDir(Camera::Right, false);
    if(key == Qt::Key_A) scene.camera().toggleMovingDir(Camera::Left , false);
    if(key == Qt::Key_Shift) scene.camera().setMoveSpeed(scene.camera().moveSpeed() / 2.0f);
}
