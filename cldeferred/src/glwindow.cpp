#include "glwindow.h"

#include <GL/glut.h>

GLWindow::GLWindow()
    : program(0), painter(0)
{
}

void GLWindow::initialize()
{
    program = new QOpenGLShaderProgram(this);
    program->addShaderFromSourceFile(QOpenGLShader::Vertex, "shader.vert");
    program->addShaderFromSourceFile(QOpenGLShader::Fragment, "shader.frag");
    program->link();

    modelMatrixUniform = program->uniformLocation("modelMatrix");
    viewMatrixUniform = program->uniformLocation("viewMatrix");
    projMatrixUniform = program->uniformLocation("projMatrix");

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    connect(&timer, SIGNAL(timeout()), this, SLOT(renderNow()));
    timer.start(1000/30);

    //scene = QGLAbstractScene::loadScene("models/m24/3ds file.3DS");
    //scene = QGLAbstractScene::loadScene("models/lighthouse/3ds file.3DS");
    scene = QGLAbstractScene::loadScene("models/untitled/untitled.obj");
    //scene = QGLAbstractScene::loadScene("models/internal_skeleton/internal_skelout.obj");
    node= scene->mainNode();

    qDebug() << fbo.init(size());
}

void GLWindow::render()
{
    qDebug() << width() << height();

    fbo.bind();

    glViewport(0, 0, width(), height());

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    projMatrix.setToIdentity();
    projMatrix.perspective(60.0f, (float)width()/height(), 0.1f, 100.0f);

    viewMatrix.setToIdentity();
    viewMatrix.lookAt(QVector3D(5,5,5), QVector3D(0,0,0), QVector3D(0,1,0));

    static float rot= 1.0f;
    modelMatrix.setToIdentity();
    modelMatrix.scale(10);
    modelMatrix.rotate(rot, 0,1,0);
    rot++;

    program->bind();

    program->setUniformValue(viewMatrixUniform, viewMatrix);
    program->setUniformValue(projMatrixUniform, projMatrix);
    program->setUniformValue(modelMatrixUniform, modelMatrix);

    if(!painter)
        painter= new QGLPainter(this);

    node->draw(painter);

    program->release();

    static int frameId= 0;
    frameId++;
    if(frameId==10)
        fbo.colorToImage().save("test.png");
}
