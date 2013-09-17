#ifndef CLDEFERRED_H
#define CLDEFERRED_H

#include "clglwindow.h"
#include "fbocl.h"
#include "scene.h"
#include "occlusionbuffer.h"

#include <QtGui>

class CLDeferred : public CLGLWindow
{
public:
    CLDeferred();
    virtual ~CLDeferred() { }

    void initializeGL();
    void initializeCL();
    void finalizeInit();
    void renderGL();
    void resizeGL(QSize size);

    void saveScreenshot(QString prefix= "screen", QString ext= "ppm");

protected:
    void grabbedMouseMoveEvent(QPointF delta);
    void grabbedKeyPressEvent(int key);
    void grabbedKeyReleaseEvent(int key);
    void keyPressEvent(QKeyEvent *event);

private:
    // OpenGL render stages
    void renderToGBuffer();  // Update the GBuffer
    void renderShadowMaps(); // Update the shadow maps
    // OpenCL render stages
    void acquireCLObjects();
    void updateOcclusionBuffer();
    void deferredPass();
    void antialiasPass();
    void releaseCLObjects();
    // Final stage (OpenGL)
    void drawOutput();

    // GL Program used to fill the gbuffer
    QOpenGLShaderProgram* firstPassProgram;
    // Light occlusion buffer, used to calculate shadows
    OcclusionBuffer occlusionBuffer;
    // CL Kernel for the 2nd pass
    cl_kernel deferredPassKernel;
    // CL Kernel for antialiasing the output texture
    cl_kernel fxaaKernel;
    // GL Program used to render outputTex
    QOpenGLShaderProgram* outputProgram;

    // Scene
    Scene scene;

    // Geometry buffer
    FBOCL gBuffer;
    // COLOR0: Diffuse texture sample + Specular power
    static const GLenum diffuseSpecFormat= GL_RGBA;
    // COLOR1: Normals in world coords
    static const GLenum normalsFormat= GL_RG16F;
    // COLOR2: Depth
    static const GLenum depthFormat= GL_R32F;
    // DEPTH: Used only for depth testing in the first pass
    static const GLenum depthTestFormat= GL_DEPTH_COMPONENT24;

    // Output texture and mapped OpenCL Image
    QGLTexture2D outputTex;
    QGLTexture2D outputTexAA;
    cl_mem outputImage;
    cl_mem outputImageAA;

    // Time metrics
    QElapsedTimer sceneTime;
    qint64 lastRenderTime;

    int fpsFrameCount;
    qint64 fpsLastTime;

    // Misc        
    bool enableAA;
    float dirLightAngle;

    QVector<cl_mem> acquiredBuffers;
};

#endif // CLDEFERRED_H
