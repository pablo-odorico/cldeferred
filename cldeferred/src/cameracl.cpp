#include "cameracl.h"

CameraCL::CameraCL(QObject* parent)
    : Camera(parent)
    , _initialized(false), _clMem(0)
{
}

bool CameraCL::init(cl_context context)
{
    cl_int error;
    _clMem= clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(cl_camera), NULL, &error);
    if(checkCLError(error, "CameraCL::init: clCreateBuffer"))
        return false;

    _initialized= true;
    return true;
}

cl_mem CameraCL::clStructMem(cl_command_queue queue)
{
    if(!_initialized) {
        qDebug() << "CameraCL::clMem: Not initialized!";
        return 0;
    }

    cl_camera clStruct;
    // TODO implement operators= for cl_float3 and QVector3D, etc.

    QMatrix4x4 viewMatrixInv= _viewMatrix.inverted();
    memcpy(&clStruct.viewMatrix, _viewMatrix.data(), sizeof(clStruct.viewMatrix));
    memcpy(&clStruct.viewMatrixInv, viewMatrixInv.data(), sizeof(clStruct.viewMatrixInv));

    QMatrix4x4 projMatrixInv= _projMatrix.inverted();
    memcpy(&clStruct.projMatrix, _projMatrix.data(), sizeof(clStruct.projMatrix));
    memcpy(&clStruct.projMatrixInv, projMatrixInv.data(), sizeof(clStruct.projMatrixInv));

    clStruct.position.x= _position.x();
    clStruct.position.y= _position.y();
    clStruct.position.z= _position.z();

    const QVector3D look= lookVector();
    clStruct.lookVector.x= look.x();
    clStruct.lookVector.y= look.y();
    clStruct.lookVector.z= look.z();

    clEnqueueWriteBuffer(queue, _clMem, CL_FALSE, 0, sizeof(clStruct),
                         &clStruct, 0, NULL, NULL);

    return _clMem;
}

