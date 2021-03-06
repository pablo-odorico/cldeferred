#include "spotlight.h"
#include "debug.h"

SpotLight::SpotLight()
    : Light()
{
    setParams(30, 3, 1, 10);
}

void SpotLight::setParams(float cutOff, float fadeDegrees, float atenuation, float near, float far)
{
    _cutOff= cutOff;
    _fadeDegrees= qBound(0.001f, fadeDegrees, cutOff);
    _atenuation= atenuation;

    float aspectRatio;
    if(_depthFbo.width() and _depthFbo.height()) {
        aspectRatio= (float)_depthFbo.width()/_depthFbo.height();
    } else {
        debugWarning("FBO size not set, call Light::setupShadowMap first.");
        aspectRatio= 1.0f;
    }

    _lightCamera.setPerspective(2 * _cutOff, aspectRatio, near, far);
}

void SpotLight::updateStructCL(cl_command_queue queue, cl_mem buffer, size_t index)
{
    cl_spotlight clStruct;

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

    clStruct.cutOffMax= _cutOff * (M_PI/180.0f);
    clStruct.cutOffMin= (_cutOff-_fadeDegrees) * (M_PI/180.0f);

    clStruct.attenuation= _atenuation;

    clStruct.hasShadows= _shadowMapping;

    cl_int error;
    error= clEnqueueWriteBuffer(queue, buffer, CL_FALSE, index * sizeof(cl_spotlight),
                                sizeof(cl_spotlight), &clStruct, 0, NULL, NULL);
    clCheckError(error, "clEnqueueWriteBuffer");
}
