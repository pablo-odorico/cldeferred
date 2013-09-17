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

    clStruct.position.x= _lightCamera.position().x();
    clStruct.position.y= _lightCamera.position().y();
    clStruct.position.z= _lightCamera.position().z();

    QVector3D lv= lookVector();
    clStruct.lookVector.x= lv.x();
    clStruct.lookVector.y= lv.y();
    clStruct.lookVector.z= lv.z();

    clStruct.ambient.x= _ambientColor.redF();
    clStruct.ambient.y= _ambientColor.greenF();
    clStruct.ambient.z= _ambientColor.blueF();

    clStruct.diffuse.x= _diffuseColor.redF();
    clStruct.diffuse.y= _diffuseColor.greenF();
    clStruct.diffuse.z= _diffuseColor.blueF();

    clStruct.specular.x= _specColor.redF();
    clStruct.specular.y= _specColor.greenF();
    clStruct.specular.z= _specColor.blueF();

    clStruct.hasShadows= _shadowMapping;

    cl_int error;
    error= clEnqueueWriteBuffer(queue, buffer, CL_FALSE, index * sizeof(cl_dirlight),
                                sizeof(cl_dirlight), &clStruct, 0, NULL, NULL);
    checkCLError(error, "clEnqueueWriteBuffer");
}
