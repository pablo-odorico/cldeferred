#include "light.h"
#include "scene.h"
#include "debug.h"
#include "assert.h"

Light::Light() :
    _shadowMapping(false),
    _shadowMappingInit(false),
    _ambientColor(50,50,50), _diffuseColor(Qt::white), _specColor(Qt::white),
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
        checkCLError(clReleaseMemObject(_filteredDepth), "clReleaseMemObject");
    cl_image_format format;
    if(!CLUtils::gl2clFormat(storedDepthFormat, format)) {
        debugWarning("Could not map depth format to OpenCL.");
        return false;
    }
    cl_int error;
    _filteredDepth= clCreateImage2D(context, CL_MEM_READ_WRITE, &format,
                                    shadowMapSize.width(), shadowMapSize.height(), 0,
                                    0, &error);
    if(checkCLError(error, "clCreateImage2D"))
        return false;


    // Load FXAA kernel
    static cl_kernel kernel= 0;
    if(!kernel) {
        if(!CLUtils::loadKernel(context, &kernel, device,
                       ":/kernels/depthDownsample.cl", "depthDownsample",
                       "-I../res/kernels/ -Werror")) {
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
    cl_int error;


    size_t workGroupSize[2] = { 16, 16 };
    size_t ndRangeSize[2];
    ndRangeSize[0]= CLUtils::roundUp(_depthFbo.width() , workGroupSize[0]);
    ndRangeSize[1]= CLUtils::roundUp(_depthFbo.height(), workGroupSize[1]);


    error  = clSetKernelArg(_downsampleKernel, 0, sizeof(cl_mem), (void*)&depthImage);
    error |= clSetKernelArg(_downsampleKernel, 1, sizeof(cl_mem), (void*)&_filteredDepth);
    error |= clEnqueueNDRangeKernel(queue, _downsampleKernel, 2, NULL,
                                    ndRangeSize, workGroupSize, 0, NULL, NULL);
    if(checkCLError(error, "Downsample kernel"))
        return false;

    if(!_depthFbo.releaseColorBuffers(queue))
        return false;

    return true;
}
