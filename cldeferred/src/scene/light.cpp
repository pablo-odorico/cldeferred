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
    /*assert(shadowMapDownsamples >= 1);
    // Check that shadowMapSize is divisible by 2 shadowMapDownsamples times
    assert(shadowMapSize.x() & ((1 << shadowMapDownsamples) - 1) == 0);
    assert(shadowMapSize.y() & ((1 << shadowMapDownsamples) - 1) == 0);*/

    if(!_depthFbo.resize(context, shadowMapSize, QList<GLenum>() << storedDepthFormat, depthTestingFormat))
    {
        debugFatal("Could not init depth FBO.");
        return false;
    }
/*
    // Generate depth downsamples
    cl_image_format clFormat;
    if(!gl2clFormat(storedDepthFormat, clFormat)) {
        debugError("Could not map OpenGL depth normals type.");
        return false;
    }

    if(_shadowMappingInit) {
        for(int i=0; i<_depthDownsamples.count(); i++)
            checkCLError(clReleaseMemObject(_depthDownsamples[i]), "clReleaseMemObject");
        _depthDownsamples.clear();
    }

    _depthDownsamples.resize(shadowMapDownsamples);
    for(int i=0; i<_depthDownsamples.count(); i++) {
        const QSize size= depthDownsampleSize(i+1);
        cl_int error;
        _depthDownsamples[i]= clCreateImage2D(context, CL_MEM_READ_WRITE, &clFormat, size.width(), size.height(), 0, NULL, &error);
        if(checkCLError(error, "clCreateImage2D"))
            return false;
    }
*/
    _shadowMappingInit= true;
    return true;
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

/*QSize Light::depthDownsampleSize(int level)
{
    const int width=  _depthFbo.width() >> i;
    const int height= _depthFbo.height() >> i;
    return QSize(width, height);
}*/
