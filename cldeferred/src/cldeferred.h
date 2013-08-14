#ifndef GLWIDGET_H
#define GLWIDGET_H

#include "clglwindow.h"
#include "fbocl.h"

#include <QtGui>
#include <QDebug>
#include <Qt3D/QGLTeapot>
#include <Qt3D/QGLPainter>
#include <Qt3D/QGLBuilder>
#include <Qt3D/QGLAbstractScene>

class CLDeferred : public CLGLWindow
{
public:
    CLDeferred(QSize maxSize=QSize(1920, 1080));

    void initializeGL();
    void initializeCL();
    void renderGL();
    void resizeGL(QSize size);

private:
    void renderToGBuffer();
    void updateOutputTex();
    void drawOutputTex();

    // Max windows size
    QSize maxSize;

    // Program used to fill the gbuffer
    QOpenGLShaderProgram* firstPassProgram;
    // Program used to render outputTex
    QOpenGLShaderProgram* outputProgram;

    // Scene, camera, etc.
    QGLAbstractScene* scene;

    QMatrix4x4 modelMatrix;
    QMatrix4x4 viewMatrix;
    QMatrix4x4 projMatrix;

    // Geometry buffer
    FBOCL gBuffer;
    // COLOR0: Diffuse texture sample + Specular power
    static const GLenum diffuseSpecFormat= GL_RGBA;
    // COLOR1: Normals in world coords
    static const GLenum normalsFormat= GL_RG16F;
    // DEPTH buffer
    static const GLenum depthFormat= GL_DEPTH_COMPONENT32F;

    // Output texture
    GLuint outputTex;
    cl_mem outputBuffer;
    cl_kernel outputKernel;

    // Misc
    QGLPainter* painter;
    QTimer renderTimer;

    uint frameId; // TODO sacar
};


#endif // GLWIDGET_H
