#ifndef GLWIDGET_H
#define GLWIDGET_H

#include "clglwindow.h"
#include "fbo.h"

#include <QtGui>
#include <QDebug>
#include <Qt3D/QGLTeapot>
#include <Qt3D/QGLPainter>
#include <Qt3D/QGLBuilder>
#include <Qt3D/QGLAbstractScene>

class GLWindow : public CLGLWindow
{
public:
    GLWindow(QSize maxSize=QSize(1920, 1080));

    void initializeGL();
    void initializeCL();
    void renderGL();
    void resizeGL(QSize size);

private:
    void renderGeometryBuffer();
    void updateOutputTex();
    void drawOutputTex();

    QSize maxSize;

    // Program used to fill the gbuffer
    QOpenGLShaderProgram* firstPassProgram;
    // Program used to render outputTex
    QOpenGLShaderProgram* outputProgram;

    QGLAbstractScene* scene;
    QGLPainter* painter;

    QMatrix4x4 modelMatrix;
    QMatrix4x4 viewMatrix;
    QMatrix4x4 projMatrix;

    QTimer renderTimer;

    FBO gBuffer;

    GLuint outputTex;
    cl_mem outputBuffer;
    cl_kernel outputKernel;

    uint frameId; // TODO sacar
};

#endif // GLWIDGET_H
