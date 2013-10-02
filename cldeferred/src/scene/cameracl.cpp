#include "cameracl.h"
#include <cassert>

CameraCL::CameraCL()
    : Camera()
    , _initialized(false), _clMem(0), _vpMatrixChanged(false)
{
}

bool CameraCL::init(cl_context context)
{
    assert(!_initialized);

    cl_int error;
    _clMem= clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(cl_camera), NULL, &error);
    if(clCheckError(error, "clCreateBuffer"))
        return false;

    _initialized= true;
    return true;
}

void CameraCL::updateStructCL(cl_command_queue queue)
{
    assert(_initialized);

    // TODO implement operators= for cl_float3 and QVector3D, etc.

    memcpy(&_clStruct.viewMatrix, _viewMatrix.transposed().data(), sizeof(cl_float16));

    QMatrix4x4 viewMatrixInv= _viewMatrix.inverted();
    memcpy(&_clStruct.viewMatrixInv, viewMatrixInv.transposed().data(), sizeof(cl_float16));

    memcpy(&_clStruct.projMatrix, _projMatrix.transposed().data(), sizeof(cl_float16));

    QMatrix4x4 projMatrixInv= _projMatrix.inverted();
    memcpy(&_clStruct.projMatrixInv, projMatrixInv.transposed().data(), sizeof(cl_float16));

    QMatrix4x4 vpMatrix= _projMatrix * _viewMatrix;
    QMatrix4x4 vpMatrixInv= vpMatrix.inverted();
    memcpy(&_clStruct.vpMatrixInv, vpMatrixInv.transposed().data(), sizeof(cl_float16));

    _clStruct.position.x= _position.x();
    _clStruct.position.y= _position.y();
    _clStruct.position.z= _position.z();

    const QVector3D look= lookVector();
    _clStruct.lookVector.x= look.x();
    _clStruct.lookVector.y= look.y();
    _clStruct.lookVector.z= look.z();

    // Motion blur matrix: (PrevProj * PrevView) * (InvView * InvProj)
    QMatrix4x4 motionBlurMatrix= _lastVPMatrix * vpMatrixInv;
    _vpMatrixChanged= vpMatrix != _lastVPMatrix;
    _lastVPMatrix= vpMatrix;
    memcpy(&_clStruct.motionBlurMatrix, motionBlurMatrix.transposed().data(), sizeof(cl_float16));

    setDoFParams();

    cl_int error;
    error= clEnqueueWriteBuffer(queue, _clMem, CL_FALSE, 0, sizeof(cl_camera),
                             &_clStruct, 0, NULL, NULL);

    clCheckError(error, "Enqueue write struct");
}

float CameraCL::getZ(float wordUnits)
{
    const float z= -_far * _near / (wordUnits * (_far-_near) - _far);
    return qBound(0.0f, z, 1.0f);
}

QPointF CameraCL::getLineParams(float x1, float y1, float x2, float y2)
{
    const float a= (y2-y1) / (x2-x1);
    const float b= y1 - a * x1;
    return QPointF(a, b);
}

void CameraCL::setDoFParams()
{
    _clStruct.cocNearFar.x= _nearCoC;
    _clStruct.cocNearFar.y= _farCoC;

    QPointF line1= getLineParams(getZ(_minClampDist), _nearCoc,
                                 getZ(_focusDistance-_depthOfField/2), 0);
    QPointF line2= getLineParams(getZ(_focusDistance+_depthOfField/2), 0,
                                 getZ(_maxClampDist), _farCoC);
    _clStruct.cocParams.x= line1.x();
    _clStruct.cocParams.y= line1.y();
    _clStruct.cocParams.z= line2.x();
    _clStruct.cocParams.w= line2.y();

    // The CoC in pixels is calculated as
    // z= sample z from depth buffer
    // f1= cocParams.x * z + cocParams.y
    // f2= cocParams.z * z + cocParams.w
    // coc= clamp(f1, 0, cocNearFar.x) + clamp(f2, 0, cocNearFar.y)
    // coc *= screenHeight
}

cl_mem CameraCL::structCL()
{
    assert(_initialized);
    return _clMem;
}
