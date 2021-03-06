#include "cldeferred.h"
#include <cassert>
#include "debug.h"
#include "analytics.h"

const GLenum CLDeferred::diffuseMatFormat;
const GLenum CLDeferred::normalsFormat;
const GLenum CLDeferred::depthFormat;
const GLenum CLDeferred::depthTestFormat;

CLDeferred::CLDeferred()
    : CLGLWindow(), firstPassProgram(0), outputProgram(0),
    enableAA(true), enableMotionBlur(true), doneMotionBlur(false),
    dirLightAngle(90.0f)
{
}

CLDeferred::~CLDeferred()
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

    outputTexMotionBlur.setBindOptions(QGLTexture2D::InvertedYBindOption);
    outputTexMotionBlur.setVerticalWrap(QGL::ClampToEdge);
    outputTexMotionBlur.setHorizontalWrap(QGL::ClampToEdge);

}

void CLDeferred::initializeCL()
{
    // Load deferred pass kernel
    KernelDefines deferredDefines;
    deferredDefines["GAMMA_CORRECT"]= "2.2f";
    deferredKernel= loadKernelPath(clCtx(), clDevice(), ":/kernels/deferredPass.cl",
            "deferredPass", deferredDefines, QStringList("../res/kernels/"));
    if(!deferredKernel)
        debugFatal("Error loading kernel.");

    // Load FXAA kernel with the pre-computed luma flag set
    KernelDefines fxaaDefines;
    fxaaDefines["LUMA_IN_ALPHA"]= "1";
    fxaaKernel= loadKernelPath(clCtx(), clDevice(), ":/kernels/fxaa.cl", "fxaa", fxaaDefines);
    if(!fxaaKernel)
        debugFatal("Error loading kernel.");

    // Motion blur kernel
    motionBlurKernel= loadKernelPath(clCtx(), clDevice(), ":/kernels/motionBlur.cl",
            "motionBlur", KernelDefines(), QStringList("../res/kernels/"));
    if(!motionBlurKernel)
        debugFatal("Error loading kernel.");
}

void CLDeferred::finalizeInit()
{
    scene.init(glPainter(), clCtx());

    if(!scene.loadScene("models/untitled/untitled2.obj"))
        debugFatal("Could not load scene!");
    //scene.camera().lookAt(QVector3D(-8, 7, -8), QVector3D(0, 0.5f, 0));
    scene.camera().lookAt(QVector3D(9.10233, 2.11752, -0.262795), QVector3D(9.10233, 2.11752, -0.262795)+QVector3D(-0.997687, 0.0263598, -0.0626599));
    scene.camera().setMoveSpeed(5);

    DirLight* dirLight= new DirLight();
    dirLight->lookAt(QVector3D(0, 40, 0), QVector3D(0, 0, 0));
    dirLight->enableShadows(true);
    dirLight->setupShadowMap(clCtx(), clDevice(), QSize(512,512));
    dirLight->setParams(50, 50, 10.0f, 200.0f);
    //dirLight->setDiffuseColor(QColor(200,200,200));
    dirLight->setAmbientColor(QColor(10,10,10));
    scene.lightManager().addDirLight(dirLight);


    SpotLight* spotLight= new SpotLight();
    //spotLight->lookAt(QVector3D(10, 10, 10), QVector3D(0, 0, 0));
    spotLight->enableShadows(true);
    spotLight->setupShadowMap(clCtx(), clDevice(), QSize(512,512));
    spotLight->setParams(15, 3, 0.0015f, 1.0f);
    //spotLight->setDiffuseColor(QColor(200,200,200));
    scene.lightManager().addSpotLight(spotLight);

/*
    SpotLight* spotLight2= new SpotLight();
    spotLight2->lookAt(QVector3D(0, 10, 0), QVector3D(0, 0, 0));
    spotLight2->enableShadows(true);
    spotLight2->setupShadowMap(clCtx(), clDevice(), QSize(512,512));
    spotLight2->setParams(30, 5, 0.01f, 0.5f);
    //spotLight2->setDiffuseColor(Qt::blue);
    scene.lightManager().addSpotLight(spotLight2);
*/
    const int targetFPS= 50;
    startRenderTimer(targetFPS);
    sceneTime.start();
    lastRenderTime= sceneTime.nsecsElapsed();

    if(!autoExposure.init(clCtx(), clDevice()))
        debugFatal("Could not init exposure.");
    autoExposure.setAdjustSpeed(3.0f / targetFPS); // Appox, works OK for 50hz
    //autoExposure.setUpdatePeriod(2);

    if(!bloom.init(clCtx(), clDevice()))
        debugFatal("Could not init bloom.");
}

void CLDeferred::resizeGL(QSize size)
{
    debugMsg("Resize to %d x %d.", size.width(), size.height());

    bool ok;

    // Set camera projection
    scene.camera().setPerspective(60.0f, (float)size.width()/size.height(), 0.1f, 100.0f);

    // Resize G-Buffer
    QList<GLenum> colorFormats= QList<GLenum>() << diffuseMatFormat << normalsFormat << depthFormat;
    ok= gBuffer.resize(clCtx(), size, colorFormats, depthTestFormat);
    if(!ok)
        debugFatal("Error initializing G-Buffer FBO.");

    // Resize Occlusion Buffer
    ok= occlusionBuffer.resize(clCtx(), clDevice(), size);
    if(!ok)
        debugFatal("Error initializing occlusion buffer.");

    // Resize bloom images
    ok= bloom.resize(size);
    if(!ok)
        debugFatal("Could not resize bloom images.");

    // Resize and map Output Texture
    cl_int error;

    outputTex.setSize(size);
    outputTex.bind();
    outputImage= clCreateFromGLTexture2D(
        clCtx(), CL_MEM_READ_WRITE, GL_TEXTURE_2D, 0, outputTex.textureId(), &error);
    if(clCheckError(error, "clCreateFromGLTexture2D"))
        debugFatal("Could not map output texture.");

    outputTexAA.setSize(size);
    outputTexAA.bind();
    // This image could be WRITE_ONLY
    outputImageAA= clCreateFromGLTexture2D(
        clCtx(), CL_MEM_READ_WRITE, GL_TEXTURE_2D, 0, outputTexAA.textureId(), &error);
    if(clCheckError(error, "clCreateFromGLTexture2D"))
        debugFatal("Could not map output texture AA.");

    outputTexMotionBlur.setSize(size);
    outputTexMotionBlur.bind();
    // This image could be WRITE_ONLY
    outputImageMotionBlur= clCreateFromGLTexture2D(
        clCtx(), CL_MEM_READ_WRITE, GL_TEXTURE_2D, 0, outputTexMotionBlur.textureId(), &error);
    if(clCheckError(error, "clCreateFromGLTexture2D"))
        debugFatal("Could not map output texture Motion Blur.");
}

void CLDeferred::renderGL()
{
    Analytics::EventTimer eventTimer("renderGL");

    // Move camera
    const qint64 now= sceneTime.nsecsElapsed();
    scene.camera().move((now - lastRenderTime)/1000.0f);
    lastRenderTime= now;

    static float a= 0;
    a += 0.05f;
    SpotLight* ssl1= scene.lightManager().spotLight(0);
    if(ssl1)
        ssl1->lookAt(QVector3D(10,10+2*sinf(a),10), QVector3D(0,0,0));

/*
    SpotLight* ssl2= scene.lightManager().spotLight(1);
    const QVector3D pos= scene.camera().position()+1*scene.camera().lookVector();
    ssl2->lookAt(pos, pos+scene.camera().lookVector());
*/

    DirLight* light= scene.lightManager().dirLight(0);
    if(light) {
        const float dist= light->position().length();
        const float aRad= dirLightAngle * (M_PI / 180.0f);
        const QVector3D pos(dist * cosf(aRad), dist * sinf(aRad), 0);
        light->lookAt(pos, QVector3D(0,0,0));
    }

    // Update OpenCL structs
    scene.updateStructsCL(clQueue());

    // 1st pass
    renderToGBuffer();

    // Update shadow maps: Render the scene once per each light with shadows
    renderShadowMaps();

    // Acquire objects
    acquireCLObjects();

    // Update the occlusion buffer
    updateOcclusionBuffer();

    // Deferred
    deferredPass();

    // Bloom blending
    bloomPass();

    // Antialiasing
    antialiasPass();

    // Motion Blur
    motionBlurPass();

    // Exposure compensation
    updateExposure();

    // Release CL objects and sync the queue
    releaseCLObjects();

    // Draw output texture
    drawOutput();

    eventTimer.finish();
    analytics.fpsUpdate();
}

//
// Render stages
//

void CLDeferred::renderToGBuffer()
{
    Analytics::EventTimer eventTimer("G-Buffer");

    gBuffer.bind();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    scene.draw(firstPassProgram, Scene::MVPMatrix | Scene::ModelITMatrix);

    gBuffer.unbind();

    glFinish();
}

void CLDeferred::renderShadowMaps()
{
    Analytics::EventTimer eventTimer("ShadowMaps");

    // Update the shadow maps of all lights
    scene.updateShadowMaps(clQueue());

    glFinish();
}

void CLDeferred::acquireCLObjects()
{
    Analytics::EventTimer eventTimer("CLAcquire");

    acquiredBuffers.clear();
    acquiredBuffers << gBuffer.colorBuffers();
    acquiredBuffers << outputImage;
    if(enableAA)
        acquiredBuffers << outputImageAA;
    if(enableMotionBlur)
        acquiredBuffers << outputImageMotionBlur;
    // The shadow map images of the lights don't have to be acquired/released

    cl_int error;
    error= clEnqueueAcquireGLObjects(clQueue(), acquiredBuffers.count(), acquiredBuffers.data(),
                                     0, 0, 0);
    if(clCheckError(error, "clEnqueueAcquireGLObjects"))
        debugFatal("Could not acquire buffers.");
}

void CLDeferred::updateExposure()
{
    // Calculate the exposure based on the output of the deferred rendering stage
    // "Bright" values (over 1.0f) in bloom.input will be clamped, so bloom has
    // no effect on the exposure.
    autoExposure.update(clQueue(), bloom.input());
}

void CLDeferred::releaseCLObjects()
{
    Analytics::EventTimer eventTimer("CLRelease");

    cl_int error;
    error= clEnqueueReleaseGLObjects(clQueue(), acquiredBuffers.count(), acquiredBuffers.data(),
                                     0, 0, 0);
    if(clCheckError(error, "clEnqueueReleaseGLObjects"))
        debugFatal("Could not release buffers.");

    clCheckError(clFinish(clQueue()), "clFinish");
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

    cl_mem camStruct= scene.camera().structCL();
    cl_mem camDepth= gBuffer.colorBuffers().at(2);

    bool ok= occlusionBuffer.update(
                clQueue(), camStruct, camDepth, lm.lightsWithShadows(),
                lm.spotStructs(), lm.dirStructs(), lm.spotDepths(), lm.dirDepths(),
                gBuffer.size());
    if(!ok)
        debugFatal("Error updating occlusion buffer.");
}

void CLDeferred::deferredPass()
{
    QVector<cl_mem> gBufferChannels= gBuffer.colorBuffers();
    if(gBufferChannels.count() != 3) {
        debugFatal("Wrong number of gbuffer channels %d", gBufferChannels.count());
        return;
    }

    cl_mem gbDiffuseMat= gBufferChannels[0];
    cl_mem gbNormals= gBufferChannels[1];
    cl_mem gbDepth= gBufferChannels[2];
    cl_mem cameraStruct= scene.camera().structCL();
    int    spotLightCount= scene.lightManager().spotLightCount();
    cl_mem spotLightStructs= scene.lightManager().spotStructs();
    int    dirLightCount= scene.lightManager().dirLightCount();
    cl_mem dirLightStructs= scene.lightManager().dirStructs();
    int    lightsWithShadows= scene.lightManager().lightsWithShadows();
    float  expo= autoExposure.exposure();
    float  brightThreshold= bloom.brightThreshold();

    // Launch kernel
    int ai= 0;
    clKernelArg(deferredKernel, ai++, gbDiffuseMat);
    clKernelArg(deferredKernel, ai++, gbNormals);
    clKernelArg(deferredKernel, ai++, gbDepth);
    clKernelArg(deferredKernel, ai++, occlusionBuffer.buffer());
    clKernelArg(deferredKernel, ai++, bloom.input());
    clKernelArg(deferredKernel, ai++, cameraStruct);
    clKernelArg(deferredKernel, ai++, spotLightCount);
    clKernelArg(deferredKernel, ai++, spotLightStructs);
    clKernelArg(deferredKernel, ai++, dirLightCount);
    clKernelArg(deferredKernel, ai++, dirLightStructs);
    clKernelArg(deferredKernel, ai++, lightsWithShadows);
    clKernelArg(deferredKernel, ai++, expo);
    clKernelArg(deferredKernel, ai++, brightThreshold);
    clLaunchKernelEvent(deferredKernel, clQueue(), gBuffer.size(), "Deferred");
}

void CLDeferred::bloomPass()
{
    if(!bloom.update(clQueue(), outputImage))
        debugFatal("Could not update bloom images.");
}

void CLDeferred::antialiasPass()
{
    if(!enableAA)
        return;

    clKernelArg(fxaaKernel, 0, outputImage);
    clKernelArg(fxaaKernel, 1, outputImageAA);
    clLaunchKernelEvent(fxaaKernel, clQueue(), gBuffer.size(), "FXAA");
}

void CLDeferred::motionBlurPass()
{
    if(!enableMotionBlur)
        return;

    // If the camera has not moved, don't perform this pass
    doneMotionBlur= scene.camera().vpMatrixChanged();
    if(!doneMotionBlur)
        return;

    cl_mem input= enableAA ? outputImageAA : outputImage;
    cl_mem cameraStruct= scene.camera().structCL();
    cl_mem depth= gBuffer.colorBuffers()[2];

    int ai= 0;
    clKernelArg(motionBlurKernel, ai++, input);
    clKernelArg(motionBlurKernel, ai++, depth);
    clKernelArg(motionBlurKernel, ai++, outputImageMotionBlur);
    clKernelArg(motionBlurKernel, ai++, cameraStruct);
    clLaunchKernelEvent(motionBlurKernel, clQueue(), gBuffer.size(), "MotionBlur");
}

void CLDeferred::drawOutput()
{
    Analytics::EventTimer eventTimer("drawOutput");

    glViewport(0, 0, width(), height());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    outputProgram->bind();

    if(enableMotionBlur and doneMotionBlur) {
        outputTexMotionBlur.bind();
    } else {
        if(enableAA)
            outputTexAA.bind();
        else
            outputTex.bind();
    }

    glBegin(GL_QUADS);
        glVertex2f(-1,-1);
        glVertex2f( 1,-1);
        glVertex2f( 1, 1);
        glVertex2f(-1, 1);
    glEnd();

    outputProgram->release();

    glFinish();
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
    if(key == Qt::Key_N) enableMotionBlur= !enableMotionBlur;

    if(key == Qt::Key_Right) dirLightAngle= qMin(dirLightAngle + 1.0f, 179.0f);
    if(key == Qt::Key_Left) dirLightAngle= qMax(dirLightAngle - 1.0f, 1.0f);

    if(key == Qt::Key_B) bloom.toggleEnabled();
    if(key == Qt::Key_Up) bloom.setBrightThreshold(bloom.brightThreshold() + 0.1f);
    if(key == Qt::Key_Down) bloom.setBrightThreshold(bloom.brightThreshold() - 0.1f);
    if(key == Qt::Key_T) bloom.setBloomBlend(bloom.bloomBlend() + 0.1f);
    if(key == Qt::Key_G) bloom.setBloomBlend(bloom.bloomBlend() - 0.1f);

    if(key == Qt::Key_E) autoExposure.toggleEnabled();
    if(key == Qt::Key_R)
        autoExposure.setMeteringMode((AutoExposure::MeteringMode)(((int)autoExposure.meteringMode() + 1) % 4));

}

void CLDeferred::saveScreenshot(QString prefix, QString ext)
{
    gBuffer.colorAttachImage(0).save(prefix + "_diffuse." + ext);
    gBuffer.colorAttachImage(1).save(prefix + "_normals." + ext);
    gBuffer.depthAttachImage().save(prefix + "_depth." + ext);
    screen()->grabWindow(winId()).save(prefix + "_output." + ext);
}
