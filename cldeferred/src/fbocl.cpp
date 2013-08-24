#include "fbocl.h"

void FBOCL::setupCL(cl_context context, cl_command_queue queue)
{
    _clContext= context;
    _clQueue= queue;
}

bool FBOCL::init(QSize size, QList<GLenum> colorFormats, GLenum depthFormat)
{
    if(!FBO::init(size, colorFormats, depthFormat)) {
        qDebug() << "FBOCL::init: FBO::init failed";
        return false;
    }

    cl_int error;

    // Map color buffers
    _colorBuffers.resize(_colorAttachs.count());
    for(int i=0; i<_colorAttachs.count(); i++) {
        _colorBuffers[i]= clCreateFromGLRenderbuffer(_clContext, CL_MEM_READ_ONLY,
                                                     _colorAttachs[i].bufferId,
                                                     &error);
        checkCLError(error, "clCreateFromGLRenderbuffer color attach");
    }

    // Map depth buffer
    /*_depthBuffer= clCreateFromGLRenderbuffer(_clContext, CL_MEM_READ_ONLY,
                                             _depthAttach.bufferId, &error);
    checkCLError(error, "clCreateFromGLRenderbuffer depth attach");
    */

    return true;
}

bool FBOCL::enqueueAquireBuffers()
{
    cl_int error;

    error= clEnqueueAcquireGLObjects(_clQueue, _colorBuffers.count(), _colorBuffers.data(), 0, 0, 0);
    if(checkCLError(error, "FBOCL::enqueueAquireBuffers: clEnqueueAcquireGLObjects colors"))
        return false;
/*
    error= clEnqueueAcquireGLObjects(_clQueue, 1, &_depthBuffer, 0, 0, 0);
    if(checkCLError(error, "FBOCL::enqueueAquireBuffers: clEnqueueAcquireGLObjects depth"))
        return false;
*/
    return true;
}

bool FBOCL::enqueueReleaseBuffers()
{
    cl_int error;

    error= clEnqueueReleaseGLObjects(_clQueue, _colorBuffers.count(), _colorBuffers.data(), 0, 0, 0);
    if(checkCLError(error, "FBOCL::enqueueReleaseBuffers: clEnqueueReleaseGLObjects color"))
        return false;
/*
    error= clEnqueueReleaseGLObjects(_clQueue, 1, &_depthBuffer, 0, 0, 0);
    if(checkCLError(error, "FBOCL::enqueueReleaseBuffers: clEnqueueReleaseGLObjects depth"))
        return false;
*/
    return true;
}
