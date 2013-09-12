#include "light.h"
#include "scene.h"
#include "debug.h"

Light::Light() :
    _shadowMapping(false),
    _shadowMappingInit(false),
    _ambientColor(50,50,50), _diffuseColor(Qt::white), _specColor(Qt::white)
{
}

bool Light::setupShadowMap(cl_context context, QSize shadowMapSize,
                           GLenum storedDepthFormat, GLenum depthTestingFormat)
{
    if(!_shadowMapping) {
        debugWarning("Shadow mapping disabled.");
        return false;
    }
    if(!_depthFbo.resize(context, shadowMapSize, QList<GLenum>() << storedDepthFormat,
                       depthTestingFormat))
    {
        debugError("Could not init depth FBO.");
        return false;
    }

    _shadowMappingInit= true;
    return true;
}

void Light::enableShadows(bool value)
{
    if(_shadowMapping and value)
        return;
    _shadowMapping= value;
    _shadowMappingInit= false;
}

void Light::updateShadowMap(const Scene& scene)
{
    if(!_shadowMapping) {
        debugWarning("Shadow mapping disabled.");
        return;
    }
    if(!_shadowMappingInit) {
        debugWarning("Shadow mapping not initialized.");
        return;
    }

    static QOpenGLShaderProgram* program= 0;
    if(!program) {
        program= new QOpenGLShaderProgram();
        program->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/shadowmapping.vert");
        program->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/shadowmapping.frag");
        program->link();
    }

    // Make sure depth testing is on
    glEnable(GL_DEPTH_TEST);

    _depthFbo.bind();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    scene.draw(_lightCamera, program, Scene::MVPMatrix);

    _depthFbo.unbind();


}
