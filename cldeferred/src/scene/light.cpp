#include "light.h"
#include "scene.h"
#include "debug.h"
#include "assert.h"

Light::Light() :
    _shadowMapping(false),
    _shadowMappingInit(false),
    _ambientColor(Qt::black), _diffuseColor(Qt::white), _specColor(Qt::white),
    _filteredDepth(0)
{
}

bool Light::setupShadowMap(cl_context context, cl_device_id device, QSize shadowMapSize,
                           GLenum storedDepthFormat, GLenum depthTestingFormat)
{
    // Init/resize depth FBO
    if(!_depthFbo.resize(context, 2 * shadowMapSize, QList<GLenum>() << storedDepthFormat,
                         depthTestingFormat))
    {
        debugWarning("Could not init depth FBO.");
        return false;
    }

    // Init/resize filtered depth image
    if(_filteredDepth)
        clCheckError(clReleaseMemObject(_filteredDepth), "clReleaseMemObject");
    cl_image_format format= CLUtils::gl2clFormat(storedDepthFormat);
    cl_int error;
    _filteredDepth= clCreateImage2D(context, CL_MEM_READ_WRITE, &format,
                                    shadowMapSize.width(), shadowMapSize.height(), 0,
                                    0, &error);
    if(clCheckError(error, "clCreateImage2D"))
        return false;


    // Load FXAA kernel
    static cl_kernel kernel= 0;
    if(!kernel) {
        kernel= CLUtils::loadKernelPath(context, device, ":/kernels/downHalfFilter.cl",
                "downHalfFilter", CLUtils::KernelDefines(), QStringList("../res/kernels/"));
        if(!kernel) {
            debugFatal("Error loading kernel.");
            return false;
        }
    }
    _downsampleKernel= kernel;


    _shadowMappingInit= true;
    return true;
}

bool Light::updateShadowMap(const Scene& scene, cl_command_queue queue)
{
    if(!_shadowMapping) {
        debugWarning("Shadow mapping disabled.");
        return false;
    }
    if(!_shadowMappingInit) {
        debugWarning("Shadow mapping not initialized.");
        return false;
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

    // Render scene from the light's point of view
    _depthFbo.bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    scene.draw(_lightCamera, program, Scene::MVPMatrix);
    _depthFbo.unbind();

    // Filter the shadow map
    if(!_depthFbo.acquireColorBuffers(queue))
        return false;

    cl_mem depthImage= _depthFbo.colorBuffers().at(0);

    clKernelArg(_downsampleKernel, 0, depthImage);
    clKernelArg(_downsampleKernel, 1, _filteredDepth);
    if(!clLaunchKernel(_downsampleKernel, queue, _depthFbo.size()))
        return false;

    if(!_depthFbo.releaseColorBuffers(queue))
        return false;

    return true;
}
