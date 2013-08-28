#ifndef CLDEFERRED_H
#define CLDEFERRED_H

#include "clglwindow.h"
#include "fbocl.h"
#include "scene.h"
#include "occlusionkernel.h"

#include <QtGui>

class CLDeferred : public CLGLWindow
{
public:
    CLDeferred(QSize maxSize= QSize(1920,1080));
    virtual ~CLDeferred() { }

    void initializeGL();
    void initializeCL();
    void renderGL();
    void resizeGL(QSize size);

protected:
    void grabbedMouseMoveEvent(QPointF delta);
    void grabbedKeyPressEvent(int key);
    void grabbedKeyReleaseEvent(int key);

private:
    void renderToGBuffer();
    void deferredPass();
    void drawOutput();

    QSize maxSize;

    // GL Program used to fill the gbuffer
    QOpenGLShaderProgram* firstPassProgram;
    // Kernel used to calculate light occlusions
    OcclusionBuffer occlusionKernel;
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

    // Output texture
    GLuint outputTex;
    cl_mem outputBuffer;

    // Time metrics
    QElapsedTimer sceneTime;
    qint64 lastRenderTime;

    int fpsFrameCount;
    qint64 fpsLastTime;
};

#endif // CLDEFERRED_H
