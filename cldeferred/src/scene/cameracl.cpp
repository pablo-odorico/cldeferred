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

    cl_int error;
    error= clEnqueueWriteBuffer(queue, _clMem, CL_FALSE, 0, sizeof(cl_camera),
                             &_clStruct, 0, NULL, NULL);

    clCheckError(error, "Enqueue write struct");
}

cl_mem CameraCL::structCL()
{
    assert(_initialized);
    return _clMem;
}
