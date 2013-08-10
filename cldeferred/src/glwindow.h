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

    void initialize();
    void render();

private:

    QOpenGLShaderProgram* program;

    QGLAbstractScene* scene;
    QGLSceneNode* node;
    QGLPainter* painter;

    GLuint modelMatrixUniform;
    QMatrix4x4 modelMatrix;

    GLuint viewMatrixUniform;
    QMatrix4x4 viewMatrix;

    GLuint projMatrixUniform;
    QMatrix4x4 projMatrix;

    QTimer timer;

    FBO fbo;
};

#endif // GLWIDGET_H
