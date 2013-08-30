#include "light.h"
#include "scene.h"

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
        qDebug() << "Light::setupShadowMap: Shadow mapping disabled.";
        return false;
    }
    if(!_depthFbo.resize(context, shadowMapSize, QList<GLenum>() << storedDepthFormat,
                       depthTestingFormat))
    {
        qDebug() << "Light::setupShadowMap: could not init _depthFbo";
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
        qDebug() << "Light::updateShadowMap: Shadow mapping disabled.";
        return;
    }
    if(!_shadowMappingInit) {
        qDebug() << "Light::updateShadowMap: Shadow mapping not initialized.";
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

    program->bind();
    program->setUniformValue("fboSize", _depthFbo.size());
    scene.draw(_lightCamera, program, Scene::MVPMatrix, false);
    program->release();

    _depthFbo.unbind();
}
