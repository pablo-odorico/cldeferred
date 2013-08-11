#ifndef GLWIDGET_H
#define GLWIDGET_H

#include "openglwindow.h"
#include "fbo.h"

#include <QtGui>
#include <QDebug>
#include <Qt3D/QGLTeapot>
#include <Qt3D/QGLPainter>
#include <Qt3D/QGLBuilder>
#include <Qt3D/QGLAbstractScene>

class GLWindow : public OpenGLWindow
{
public:
    GLWindow();

    void initializeGL();
    void renderGL();
    void resizeGL(QSize size);

private:

    QOpenGLShaderProgram* program;

    QGLAbstractScene* scene;
    QGLPainter* painter;

    GLuint modelMatrixUniform;
    QMatrix4x4 modelMatrix;
    GLuint modelITMatrixUniform; // Inverse transpose model-view

    GLuint viewMatrixUniform;
    QMatrix4x4 viewMatrix;

    GLuint projMatrixUniform;
    QMatrix4x4 projMatrix;

    GLuint mvpMatrixUniform;

    QTimer timer;

    FBO fbo;
};

#endif // GLWIDGET_H
