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

protected:
    void grabbedMouseMoveEvent(QPointF delta);
    void grabbedKeyPressEvent(int key);
    void grabbedKeyReleaseEvent(int key);

private:
    // Render stages
    void renderToGBuffer();
    void updateShadowMaps();
    void deferredPass();
    void drawOutput();

    // GL Program used to fill the gbuffer
    QOpenGLShaderProgram* firstPassProgram;
    // Light occlusion buffer, used to calculate shadows
    OcclusionBuffer occlusionBuffer;
    // CL Kernel for the 2nd pass
    cl_kernel deferredPassKernel;
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
    cl_mem outputImage;

    // Time metrics
    QElapsedTimer sceneTime;
    qint64 lastRenderTime;

    int fpsFrameCount;
    qint64 fpsLastTime;
};

#endif // CLDEFERRED_H
