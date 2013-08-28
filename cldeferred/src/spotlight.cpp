#include "spotlight.h"

SpotLight::SpotLight()
{
    setParams(45, 1, 10);
}

void SpotLight::setParams(float cutOff, float exponent, float linearAtenuation, float nearValue)
{
    _cutOff= cutOff;
    _exponent= exponent;
    _linearAtenuation= linearAtenuation;

    _lightCamera.setPerspective(_cutOff, 1.0f, nearValue, 1.0f/_linearAtenuation);
}

void SpotLight::clUpdateStruct(cl_command_queue queue, cl_mem buffer, size_t index)
{
    cl_spotlight clStruct;

    QMatrix4x4 viewProjMatrix= _lightCamera.projMatrix() * _lightCamera.viewMatrix();
    memcpy(&clStruct.viewProjMatrix, viewProjMatrix.data(), sizeof(clStruct.viewProjMatrix));

    clEnqueueWriteBuffer(queue, buffer, CL_FALSE, index * sizeof(cl_spotlight),
                         sizeof(cl_spotlight), &clStruct, 0, NULL, NULL);
}
