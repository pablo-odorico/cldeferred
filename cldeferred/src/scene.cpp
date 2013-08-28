#include "scene.h"

Scene::Scene()
    : _models(0), _scene(0), _painter(0)
{
}

Scene::~Scene()
{
    delete _scene;
}

void Scene::draw(const Camera& camera, QOpenGLShaderProgram* program, int uniformsFlags) const
{
    if(!_models or !_painter) {
        qDebug() << "Scene::draw: Null scene or painter not set.";
        return;
    }

    program->bind();

    if(uniformsFlags & ViewMatrix)
        program->setUniformValue("viewMatrix", camera.viewMatrix());
    if(uniformsFlags & ViewInvMatrix)
        program->setUniformValue("viewInvMatrix", camera.viewMatrix().inverted());
    if(uniformsFlags & ProjMatrix)
        program->setUniformValue("projMatrix", camera.projMatrix());
    if(uniformsFlags & ProjInvMatrix)
        program->setUniformValue("projInvMatrix", camera.projMatrix().inverted());

    QList<QGLSceneNode*> nodes= _models->allChildren();
    nodes << _models;

    foreach(QGLSceneNode* n, nodes) {
        if(!n->count())
            continue;

        const QMatrix4x4 modelMatrix= n->localTransform();

        if(uniformsFlags & ModelMatrix)
            program->setUniformValue("modelMatrix", modelMatrix);
        if(uniformsFlags & ModelInvMatrix)
            program->setUniformValue("modelInvMatrix", modelMatrix.inverted());
        if(uniformsFlags & ModelITMatrix)
            program->setUniformValue("modelITMatrix", modelMatrix.inverted().transposed());
        if(uniformsFlags & MVPMatrix)
            program->setUniformValue("mvpMatrix", camera.projMatrix() * camera.viewMatrix() * modelMatrix);

        n->material()->bind(_painter);
        n->geometry().draw(_painter, n->start(), n->count());
    }

    program->release();
}

void Scene::draw(QOpenGLShaderProgram* program, int uniformsFlags) const
{
    draw(_camera, program, uniformsFlags);
}

bool Scene::loadScene(QString path)
{
    delete _scene;
    _scene= QGLAbstractScene::loadScene(path);

    if(!_scene)
        return false;

    _models= _scene->mainNode();
    return true;
}
