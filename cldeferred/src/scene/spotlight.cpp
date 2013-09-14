#include "spotlight.h"
#include "debug.h"

SpotLight::SpotLight()
{
    setParams(45, 1, 10);
}

void SpotLight::setParams(float cutOff, float exponent, float linearAtenuation, float nearValue)
{
    _cutOff= cutOff;
    _exponent= exponent;
    _linearAtenuation= linearAtenuation;

    float aspectRatio;
    if(_depthFbo.width() and _depthFbo.height()) {
        aspectRatio= (float)_depthFbo.width()/_depthFbo.height();
    } else {
        debugWarning("FBO size not set, call Light::setupShadowMap first.");
        aspectRatio= 1.0f;
    }

    _lightCamera.setPerspective(_cutOff, aspectRatio, nearValue, 1.0f/_linearAtenuation);
}

void SpotLight::updateStructCL(cl_command_queue queue, cl_mem buffer, size_t index)
{
    cl_spotlight clStruct;

    QMatrix4x4 viewProjMatrix= _lightCamera.projMatrix() * _lightCamera.viewMatrix();
    memcpy(&clStruct.viewProjMatrix, viewProjMatrix.transposed().data(), sizeof(cl_float16));

    clStruct.hasShadows= _shadowMapping;

    cl_int error;
    error= clEnqueueWriteBuffer(queue, buffer, CL_FALSE, index * sizeof(cl_spotlight),
                                sizeof(cl_spotlight), &clStruct, 0, NULL, NULL);
    checkCLError(error, "clEnqueueWriteBuffer");
}
