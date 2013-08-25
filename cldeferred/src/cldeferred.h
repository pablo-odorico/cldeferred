#ifndef GLWIDGET_H
#define GLWIDGET_H

#include "clglwindow.h"
#include "fbocl.h"
#include "camera.h"

#include <QtGui>
#include <QDebug>
#include <Qt3D/QGLTeapot>
#include <Qt3D/QGLPainter>
#include <Qt3D/QGLBuilder>
#include <Qt3D/QGLAbstractScene>

class CLDeferred : public CLGLWindow
{
public:
    CLDeferred(QSize maxSize= QSize(1920,1080));

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

    QGLPainter* painter;
    // Program used to fill the gbuffer
    QOpenGLShaderProgram* firstPassProgram;
    // Program used to render outputTex
    QOpenGLShaderProgram* outputProgram;

    // Scene, camera, etc.
    Camera camera;
    QGLAbstractScene* scene;

    QElapsedTimer sceneTime;
    qint64 lastRenderTime;

    QMatrix4x4 modelMatrix;

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
    cl_kernel outputKernel;

    int fpsFrameCount;
    qint64 fpsLastTime;
};


#endif // GLWIDGET_H
