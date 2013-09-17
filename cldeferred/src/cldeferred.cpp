#include "cldeferred.h"
#include <cassert>
#include "debug.h"

const GLenum CLDeferred::diffuseSpecFormat;
const GLenum CLDeferred::normalsFormat;
const GLenum CLDeferred::depthFormat;
const GLenum CLDeferred::depthTestFormat;

CLDeferred::CLDeferred()
    : CLGLWindow()
    , firstPassProgram(0), outputProgram(0)
    , fpsFrameCount(0), fpsLastTime(0)
    , enableAA(true)
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
    // gBuffer is created/resized on resizeGL()
    firstPassProgram= new QOpenGLShaderProgram(this);
    firstPassProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/firstpass.vert");
    firstPassProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/firstpass.frag");
    firstPassProgram->link();

    outputProgram= new QOpenGLShaderProgram(this);
    outputProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/outputQuad.vert");
    outputProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/outputQuad.frag");
    outputProgram->link();

    // 2nd pass init
    // outputTex is created/resized on resizeGL()
    outputTex.setBindOptions(QGLTexture2D::InvertedYBindOption);
    outputTex.setVerticalWrap(QGL::ClampToEdge);
    outputTex.setHorizontalWrap(QGL::ClampToEdge);

    outputTexAA.setBindOptions(QGLTexture2D::InvertedYBindOption);
    outputTexAA.setVerticalWrap(QGL::ClampToEdge);
    outputTexAA.setHorizontalWrap(QGL::ClampToEdge);

}

void CLDeferred::initializeCL()
{
    if(!loadKernel(clCtx(), &deferredPassKernel, clDevice(),
                   ":/kernels/deferredPass.cl", "deferredPass",
                   "-I../res/kernels/ -Werror")) {
        debugFatal("Error loading kernel.");
    }

    // Load the FXAA setting the pre-computed luma flag
    if(!loadKernel(clCtx(), &fxaaKernel, clDevice(),
                   ":/kernels/fxaa.cl", "fxaa",
                   "-D FXAA_ALPHALUMA -I../res/kernels/ -Werror")) {
        debugFatal("Error loading kernel.");
    }
}

void CLDeferred::finalizeInit()
{
    scene.init(glPainter(), clCtx());

    if(!scene.loadScene("models/untitled/untitled2.obj"))
        debugFatal("Could not load scene!");
    //scene.camera().lookAt(QVector3D(-8, .5f, -8), QVector3D(0, 0, 0));
    scene.camera().lookAt(QVector3D(-5.28565, 5.13663, -11.9598), QVector3D(-5.28565, 5.13663, -11.9598)+QVector3D(0.0942199, -0.670006, 0.736353));
    scene.camera().setMoveSpeed(5);

    SpotLight* spotLight= new SpotLight();
    spotLight->lookAt(QVector3D(10, 10, 10), QVector3D(0, 0, 0));
    spotLight->enableShadows(true);
    spotLight->setupShadowMap(clCtx(), clDevice(), QSize(512,512));
    spotLight->setParams(30, 1, 1/30.0f, 1.0f);
    scene.lightManager().addSpotLight(spotLight);
/*
    SpotLight* spotLight2= new SpotLight();
    spotLight2->lookAt(QVector3D(-10, 10, -10), QVector3D(0, 0, 0));
    spotLight2->enableShadows(true);
    spotLight2->setupShadowMap(clCtx(), clDevice(), QSize(256,256));
    spotLight2->setParams(30, 1, 1/30.0f, 1.0f);
    scene.lightManager().addSpotLight(spotLight2);
*/
    startRenderTimer(30);
    sceneTime.start();
    lastRenderTime= sceneTime.nsecsElapsed();
    fpsLastTime= sceneTime.nsecsElapsed();
}

void CLDeferred::resizeGL(QSize size)
{
    debugMsg("Resize to %d x %d.", size.width(), size.height());

    bool ok;

    // Set camera projection
    scene.camera().setPerspective(60.0f, (float)size.width()/size.height(), 0.1f, 100.0f);

    // Resize G-Buffer
    QList<GLenum> colorFormats= QList<GLenum>() << diffuseSpecFormat << normalsFormat << depthFormat;
    ok= gBuffer.resize(clCtx(), size, colorFormats, depthTestFormat);
    if(!ok)
        debugFatal("Error initializing G-Buffer FBO.");

    // Resize Occlusion Buffer
    ok= occlusionBuffer.resize(clCtx(), clDevice(), size);
    if(!ok)
        debugFatal("Error initializing occlusion buffer.");

    // Resize and map Output Texture
    cl_int error;

    outputTex.setSize(size);
    outputTex.bind();
    outputImage= clCreateFromGLTexture2D(
        clCtx(), CL_MEM_READ_WRITE, GL_TEXTURE_2D, 0, outputTex.textureId(), &error);
    if(checkCLError(error, "clCreateFromGLTexture2D"))
        debugFatal("Could not map output texture.");

    outputTexAA.setSize(size);
    outputTexAA.bind();
    outputImageAA= clCreateFromGLTexture2D(
        clCtx(), CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, outputTexAA.textureId(), &error);
    if(checkCLError(error, "clCreateFromGLTexture2D"))
        debugFatal("Could not map output texture AA.");
}

void CLDeferred::renderGL()
{
    const qint64 now= sceneTime.nsecsElapsed();
    QVector<qint64> times;

    // Move camera
    scene.camera().move((now - lastRenderTime)/1000.0f);

    static float a= 0;
    a += 0.1f;
    SpotLight* ssl1= scene.lightManager().spotLight(0);
    ssl1->lookAt(QVector3D(10,10+2*sinf(a),10), QVector3D(0,0,0));

    // Update OpenCL structs
    scene.updateStructsCL(clQueue());

    // 1st pass
    renderToGBuffer();
    times << sceneTime.nsecsElapsed();

    // Update shadow maps: Render the scene once per each light with shadows
    renderShadowMaps();
    times << sceneTime.nsecsElapsed();

    acquireCLObjects();
    times << sceneTime.nsecsElapsed();

    // Update the occlusion buffer
    updateOcclusionBuffer();
    times << sceneTime.nsecsElapsed();

    // 2nd pass
    deferredPass();
    times << sceneTime.nsecsElapsed();

    // Antialiasing
    antialiasPass();
    times << sceneTime.nsecsElapsed();

    releaseCLObjects();
    times << sceneTime.nsecsElapsed();

    // Draw output texture
    drawOutput();
    times << sceneTime.nsecsElapsed();

    static int frame= 0;
    frame++;
    if(frame==55) {
        saveScreenshot();
    }

    const qint64 fpsElapsed= now - fpsLastTime;
    if(fpsElapsed > 3e9) {
        int i= 0;
        const float elapsed1st = (times[i] - now       ) / 1e6f; i++;
        const float elapsedShad= (times[i] - times[i-1]) / 1e6f; i++;
        const float elapsedAcqi= (times[i] - times[i-1]) / 1e6f; i++;
        const float elapsedOccl= (times[i] - times[i-1]) / 1e6f; i++;
        const float elapsed2nd = (times[i] - times[i-1]) / 1e6f; i++;
        const float elapsedAA  = (times[i] - times[i-1]) / 1e6f; i++;
        const float elapsedRele= (times[i] - times[i-1]) / 1e6f; i++;
        const float elapsedDraw= (times[i] - times[i-1]) / 1e6f; i++;
        const float totalTime= elapsed1st + elapsedShad + elapsedAcqi + elapsedOccl
                + elapsed2nd + elapsedAA + elapsedRele + elapsedDraw;

        qDebug(" ");
        qDebug("FPS   : %.01f Hz. (Max. %.01f Hz.)", fpsFrameCount/(fpsElapsed/1e9d), 1000/totalTime);
        qDebug("Times : G-Buffer | Shadow   | CL Aquire | Occlusion | Deferred  | FXAA     | CL Release | Draw quad | TOTAL");
        qDebug("        %2.02f ms. | %2.02f ms. | %2.02f ms.  | %2.02f ms.  | %2.02f ms.  | %2.02f ms. | %2.02f ms.   | %2.02f ms.  | %.02f ms.",
               elapsed1st, elapsedShad, elapsedAcqi, elapsedOccl, elapsed2nd, elapsedAA, elapsedRele,
               elapsedDraw, totalTime);

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

void CLDeferred::renderShadowMaps()
{
    // Update the shadow maps of all lights
    scene.updateShadowMaps(clQueue());
}

void CLDeferred::acquireCLObjects()
{
    acquiredBuffers.clear();
    acquiredBuffers << gBuffer.colorBuffers();
    acquiredBuffers << outputImage << outputImageAA;
    // The shadow map images of the lights don't have to be acquired/released

    cl_int error;
    error= clEnqueueAcquireGLObjects(clQueue(), acquiredBuffers.count(),
                                     acquiredBuffers.data(), 0, 0, 0);
    if(checkCLError(error, "clEnqueueAcquireGLObjects"))
        debugFatal("Could not acquire buffers.");

    // Sync
    checkCLError(clFinish(clQueue()), "clFinish");
}

void CLDeferred::releaseCLObjects()
{
    cl_int error;
    error= clEnqueueReleaseGLObjects(clQueue(), acquiredBuffers.count(),
                                     acquiredBuffers.data(), 0, 0, 0);
    if(checkCLError(error, "clEnqueueReleaseGLObjects"))
        debugFatal("Could not release buffers.");

    // Sync
    checkCLError(clFinish(clQueue()), "clFinish");
}


void CLDeferred::updateOcclusionBuffer()
{
    LightManager& lm= scene.lightManager();

    // If no light has shadows, quit, but make sure that the occlusion buffer
    // is updated at least once (as it has to be created for the deferred pass)
    static bool firstTime= true;
    if(!lm.lightsWithShadows() and !firstTime)
        return;
    firstTime= false;

    // Use those shadow maps to update the occlusion buffer
    cl_mem camStruct= scene.camera().structCL();
    cl_mem camDepth= gBuffer.colorBuffers().at(2);
    cl_mem spotLightStructs= lm.spotStructs();
    QVector<cl_mem> spotLightDepthImgs= lm.spotDepths();

    bool ok= occlusionBuffer.update(
                clQueue(), camStruct, camDepth, lm.lightsWithShadows(),
                spotLightStructs, spotLightDepthImgs,
                gBuffer.size());
    if(!ok)
        debugFatal("Error updating occlusion buffer.");

    checkCLError(clFinish(clQueue()), "clFinish");
}

void CLDeferred::deferredPass()
{
    cl_int error;

    QVector<cl_mem> gBufferChannels= gBuffer.colorBuffers();
    if(gBufferChannels.count() != 3) {
        debugFatal("Wrong number of gbuffer channels %d", gBufferChannels.count());
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
    int    spotLightCount= scene.lightManager().spotLightCount();
    int    lightsWithShadows= scene.lightManager().lightsWithShadows();

    // Launch kernel
    error  = clSetKernelArg(deferredPassKernel, 0, sizeof(cl_mem), (void*)&gbDiffuseSpec);
    error |= clSetKernelArg(deferredPassKernel, 1, sizeof(cl_mem), (void*)&gbNormals);
    error |= clSetKernelArg(deferredPassKernel, 2, sizeof(cl_mem), (void*)&gbDepth);
    error |= clSetKernelArg(deferredPassKernel, 3, sizeof(cl_mem), (void*)&oBuffer);
    error |= clSetKernelArg(deferredPassKernel, 4, sizeof(cl_mem), (void*)&outputImage);
    error |= clSetKernelArg(deferredPassKernel, 5, sizeof(cl_mem), (void*)&cameraStruct);
    error |= clSetKernelArg(deferredPassKernel, 6, sizeof(cl_mem), (void*)&spotLightStructs);
    error |= clSetKernelArg(deferredPassKernel, 7, sizeof(int),    (void*)&spotLightCount);
    error |= clSetKernelArg(deferredPassKernel, 8, sizeof(int),    (void*)&lightsWithShadows);
    error |= clEnqueueNDRangeKernel(clQueue(), deferredPassKernel, 2, NULL,
                                    ndRangeSize, workGroupSize, 0, NULL, NULL);
    checkCLError(error, "outputKernel");

    // Sync
    checkCLError(clFinish(clQueue()), "clFinish");
}

void CLDeferred::antialiasPass()
{
    if(!enableAA)
        return;

    // Work group and NDRange
    size_t workGroupSize[2] = { 16, 16 };
    size_t ndRangeSize[2];
    ndRangeSize[0]= roundUp(gBuffer.width() , workGroupSize[0]);
    ndRangeSize[1]= roundUp(gBuffer.height(), workGroupSize[1]);

    cl_int error;
    error  = clSetKernelArg(fxaaKernel, 0, sizeof(cl_mem), (void*)&outputImage);
    error |= clSetKernelArg(fxaaKernel, 1, sizeof(cl_mem), (void*)&outputImageAA);
    error |= clEnqueueNDRangeKernel(clQueue(), fxaaKernel, 2, NULL,
                                    ndRangeSize, workGroupSize, 0, NULL, NULL);
    checkCLError(error, "fxaaKernel");

    // Sync
    checkCLError(clFinish(clQueue()), "clFinish");
}

void CLDeferred::drawOutput()
{
    glViewport(0, 0, width(), height());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    outputProgram->bind();

    if(enableAA)
        outputTexAA.bind();
    else
        outputTex.bind();

    glBegin(GL_QUADS);
        glVertex2f(-1,-1);
        glVertex2f( 1,-1);
        glVertex2f( 1, 1);
        glVertex2f(-1, 1);
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

void CLDeferred::keyPressEvent(QKeyEvent *event)
{
    CLGLWindow::keyPressEvent(event);
    const int key= event->key();
    if(key == Qt::Key_P) saveScreenshot();
    if(key == Qt::Key_M) enableAA= !enableAA;
}

void CLDeferred::saveScreenshot(QString prefix, QString ext)
{
    gBuffer.colorAttachImage(0).save(prefix + "_diffuse." + ext);
    gBuffer.colorAttachImage(1).save(prefix + "_normals." + ext);
    gBuffer.depthAttachImage().save(prefix + "_depth." + ext);
    screen()->grabWindow(winId()).save(prefix + "_output." + ext);
}
