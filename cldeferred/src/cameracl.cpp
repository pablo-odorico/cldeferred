#include "cameracl.h"
#include <cassert>

CameraCL::CameraCL()
    : Camera()
    , _initialized(false), _clMem(0)
{
}

bool CameraCL::init(cl_context context)
{
    assert(!_initialized);

    cl_int error;
    _clMem= clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(cl_camera), NULL, &error);
    if(checkCLError(error, "CameraCL::init: clCreateBuffer"))
        return false;

    _initialized= true;
    return true;
}

void CameraCL::updateStructCL(cl_command_queue queue)
{
    assert(_initialized);

    cl_camera clStruct;
    // TODO implement operators= for cl_float3 and QVector3D, etc.

    memcpy(&clStruct.viewMatrix, _viewMatrix.transposed().data(), sizeof(cl_float16));

    QMatrix4x4 viewMatrixInv= _viewMatrix.inverted();
    memcpy(&clStruct.viewMatrixInv, viewMatrixInv.transposed().data(), sizeof(cl_float16));

    memcpy(&clStruct.projMatrix, _projMatrix.transposed().data(), sizeof(cl_float16));

    QMatrix4x4 projMatrixInv= _projMatrix.inverted();
    memcpy(&clStruct.projMatrixInv, projMatrixInv.transposed().data(), sizeof(cl_float16));

    QMatrix4x4 vpMatrixInv= (_projMatrix * _viewMatrix).inverted();
    memcpy(&clStruct.vpMatrixInv, vpMatrixInv.transposed().data(), sizeof(cl_float16));

    clStruct.position.x= _position.x();
    clStruct.position.y= _position.y();
    clStruct.position.z= _position.z();

    const QVector3D look= lookVector();
    clStruct.lookVector.x= look.x();
    clStruct.lookVector.y= look.y();
    clStruct.lookVector.z= look.z();

/*
    memset(&clStruct, 0, sizeof(cl_camera));
    QMatrix4x4 test= QMatrix4x4();
    test(0,0)=0.3;
    test(0,1)=0.6;
    test(0,2)=0.8;
    test(0,3)=1.0;
    memcpy(&clStruct.projMatrix, test.transposed().data(), sizeof(cl_float16));
*/
    qDebug() << _projMatrix(2,3) << _projMatrix(2,2) << _projMatrix(3,2);

    cl_int error;
    error= clEnqueueWriteBuffer(queue, _clMem, CL_FALSE, 0, sizeof(cl_camera),
                             &clStruct, 0, NULL, NULL);

    checkCLError(error, "Enqueue write struct");
}

cl_mem CameraCL::structCL()
{
    assert(_initialized);
    return _clMem;
}
