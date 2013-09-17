#include "dirlight.h"
#include <cassert>

DirLight::DirLight()
    : Light()
{
    setParams(50, 50);
}

void DirLight::setParams(float areaWidth, float areaHeight, float near, float far)
{
    _areaWidth= areaWidth;
    _areaHeight= areaHeight;
    _near= near;
    _far= far;

    assert(areaHeight);

    _lightCamera.setOrthoProjection(-areaWidth/2, areaWidth/2,
                                    -areaHeight/2, areaHeight/2,
                                    near, far);
}

void DirLight::updateStructCL(cl_command_queue queue, cl_mem buffer, size_t index)
{
    cl_dirlight clStruct;

    QMatrix4x4 viewProjMatrix= _lightCamera.projMatrix() * _lightCamera.viewMatrix();
    memcpy(&clStruct.viewProjMatrix, viewProjMatrix.transposed().data(), sizeof(cl_float16));

    clStruct.hasShadows= _shadowMapping;

    cl_int error;
    error= clEnqueueWriteBuffer(queue, buffer, CL_FALSE, index * sizeof(cl_dirlight),
                                sizeof(cl_dirlight), &clStruct, 0, NULL, NULL);
    checkCLError(error, "clEnqueueWriteBuffer");
}
