#include "fbocl.h"

bool FBOCL::init(cl_context clContext, QSize size, QList<GLenum> colorFormats, GLenum depthFormat)
{
    if(!FBO::init(size, colorFormats, depthFormat)) {
        qDebug() << "FBOCL::init: FBO::init failed";
        return false;
    }

    cl_int error;

    // Map color buffers
    _colorBuffers.resize(_colorAttachs.count());
    for(int i=0; i<_colorAttachs.count(); i++) {
        _colorBuffers[i]= clCreateFromGLRenderbuffer(clContext, CL_MEM_READ_ONLY,
                                                     _colorAttachs[i].bufferId,
                                                     &error);
        checkCLError(error, "clCreateFromGLRenderbuffer color attach");
    }

    // Map depth buffer
    /*_depthBuffer= clCreateFromGLRenderbuffer(clContext, CL_MEM_READ_ONLY,
                                             _depthAttach.bufferId, &error);
    checkCLError(error, "clCreateFromGLRenderbuffer depth attach");
    */

    return true;
}

bool FBOCL::enqueueAquireBuffers(cl_command_queue clQueue)
{
    cl_int error;

    error= clEnqueueAcquireGLObjects(clQueue, _colorBuffers.count(), _colorBuffers.data(), 0, 0, 0);
    if(checkCLError(error, "FBOCL::enqueueAquireBuffers: clEnqueueAcquireGLObjects colors"))
        return false;
/*
    error= clEnqueueAcquireGLObjects(clQueue, 1, &_depthBuffer, 0, 0, 0);
    if(checkCLError(error, "FBOCL::enqueueAquireBuffers: clEnqueueAcquireGLObjects depth"))
        return false;
*/
    return true;
}

bool FBOCL::enqueueReleaseBuffers(cl_command_queue clQueue)
{
    cl_int error;

    error= clEnqueueReleaseGLObjects(clQueue, _colorBuffers.count(), _colorBuffers.data(), 0, 0, 0);
    if(checkCLError(error, "FBOCL::enqueueReleaseBuffers: clEnqueueReleaseGLObjects color"))
        return false;
/*
    error= clEnqueueReleaseGLObjects(clQueue, 1, &_depthBuffer, 0, 0, 0);
    if(checkCLError(error, "FBOCL::enqueueReleaseBuffers: clEnqueueReleaseGLObjects depth"))
        return false;
*/
    return true;
}
