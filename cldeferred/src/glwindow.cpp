#include "glwindow.h"

GLWindow::GLWindow()
    : program(0), painter(0)
{

}

void GLWindow::initializeGL()
{
    qDebug() << "Initialize GL";

    painter= new QGLPainter(this);

    program = new QOpenGLShaderProgram(this);
    program->addShaderFromSourceFile(QOpenGLShader::Vertex, "shader.vert");
    program->addShaderFromSourceFile(QOpenGLShader::Fragment, "shader.frag");
    program->link();

    modelMatrixUniform = program->uniformLocation("modelMatrix");
    modelITMatrixUniform = program->uniformLocation("modelITMatrix");
    viewMatrixUniform = program->uniformLocation("viewMatrix");
    projMatrixUniform = program->uniformLocation("projMatrix");
    mvpMatrixUniform = program->uniformLocation("mvpMatrix");

    glEnable(GL_DEPTH_TEST);

    connect(&timer, SIGNAL(timeout()), this, SLOT(renderLater()));
    timer.start(1000/30);

    scene = QGLAbstractScene::loadScene("models/untitled/untitled.obj");
}

void GLWindow::resizeGL(QSize size)
{
    qDebug() << "Resize GL" << size;

    // Create/resize FBO
    fbo.init(size);
    // Set viewport
    glViewport(0, 0, size.width(), size.height());
}

void GLWindow::renderGL()
{
    fbo.bind();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    projMatrix.setToIdentity();
    projMatrix.perspective(60.0f, (float)width()/height(), 1.0f, 50.0f);

    viewMatrix.setToIdentity();
    viewMatrix.lookAt(QVector3D(5,5,5), QVector3D(0,0,0), QVector3D(0,1,0));

    static float rot= 1.0f;
    modelMatrix.setToIdentity();
    modelMatrix.scale(10);
    modelMatrix.rotate(rot, 0,1,0);
    rot++;

    program->bind();

    program->setUniformValue(modelMatrixUniform, modelMatrix);
    program->setUniformValue(modelITMatrixUniform, modelMatrix.inverted().transposed());
    program->setUniformValue(viewMatrixUniform, viewMatrix);    
    program->setUniformValue(projMatrixUniform, projMatrix);
    program->setUniformValue(mvpMatrixUniform, projMatrix * viewMatrix * modelMatrix);

    scene->mainNode()->draw(painter);

    program->release();

    static int frameId= 0;
    frameId++;
    if(frameId==10) {
        fbo.diffuseToImage().save("diffuse.png");
        fbo.depthToImage().save("depth.png");
        fbo.normalsToImage().save("normals.png");
    }
}
