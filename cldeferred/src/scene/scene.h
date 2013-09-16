#ifndef SCENE_H
#define SCENE_H

#include "cameracl.h"
#include "lightmanager.h"
#include <Qt3D/QGLAbstractScene>
#include <QOpenGLShaderProgram>

class Scene
{
public:
    enum UniformsFlags {
        ModelMatrix    = 1 << 0, // uniform mat4 modelMatrix
        ModelInvMatrix = 1 << 1, // uniform mat4 modelInvMatrix
        ModelITMatrix  = 1 << 2, // uniform mat4 modelITMatrix
        ViewMatrix     = 1 << 3, // uniform mat4 viewMatrix
        ViewInvMatrix  = 1 << 4, // uniform mat4 viewInvMatrix
        ProjMatrix     = 1 << 5, // uniform mat4 projMatrix
        ProjInvMatrix  = 1 << 6, // uniform mat4 projInvMatrix
        MVPMatrix      = 1 << 7  // uniform mat4 mvpMatrix
    };

    Scene();
    ~Scene();

    // init MUST be called before calling draw
    void init(QGLPainter* painter, cl_context context);

    void draw(QOpenGLShaderProgram* program, int uniformsFlags) const;
    void draw(const Camera& camera, QOpenGLShaderProgram* program,
              int uniformsFlags, bool bindProgram=true) const;

    QGLSceneNode* models() { return _models; }
    void setModels(QGLSceneNode* models) { _models= models; }
    bool loadScene(QString path);

    CameraCL& camera() { return _camera; }
    void setCamera(const CameraCL& camera) { _camera= camera; }

    LightManager& lightManager() { return _lights; }
    void updateShadowMaps(cl_command_queue queue) { _lights.updateShadowMaps(*this, queue); }

    void updateStructsCL(cl_command_queue queue);

private:
    CameraCL _camera;

    LightManager _lights;

    QGLSceneNode* _models;
    QGLAbstractScene* _scene;

    mutable QGLPainter* _painter;
};

#endif // SCENE_H
