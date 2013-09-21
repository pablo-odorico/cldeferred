#include "fbocl.h"
#include <cassert>
#include "debug.h"

bool FBOCL::resize(cl_context context, QSize size, QList<GLenum> colorFormats,
                   GLenum depthFormat)
{
    if(!FBO::resize(size, colorFormats, depthFormat)) {
        debugFatal("Resize failed");
        return false;
    }
    _initialized= false;

    cl_int error;

    // Map color buffers
    _colorBuffers.resize(_colorAttachs.count());
    for(int i=0; i<_colorAttachs.count(); i++) {
        _colorBuffers[i]= clCreateFromGLRenderbuffer(context, CL_MEM_READ_ONLY,
                                                     _colorAttachs[i].bufferId,
                                                     &error);
        clCheckError(error, "clCreateFromGLRenderbuffer color attach");
    }

    // Map depth buffer
    /*_depthBuffer= clCreateFromGLRenderbuffer(clContext, CL_MEM_READ_ONLY,
                                             _depthAttach.bufferId, &error);
    checkCLError(error, "clCreateFromGLRenderbuffer depth attach");
    */

    _initialized= true;
    return true;
}

bool FBOCL::acquireColorBuffers(cl_command_queue queue)
{
    assert(_initialized);

    cl_int error;
    error= clEnqueueAcquireGLObjects(queue, _colorBuffers.count(), _colorBuffers.data(), 0, 0, 0);
    return !clCheckError(error, "clEnqueueAcquireGLObjects");
}

QVector<cl_mem> FBOCL::colorBuffers()
{
    assert(_initialized);

    return _colorBuffers;
}

bool FBOCL::releaseColorBuffers(cl_command_queue queue)
{
    assert(_initialized);

    cl_int error;
    error= clEnqueueReleaseGLObjects(queue, _colorBuffers.count(), _colorBuffers.data(), 0, 0, 0);
    return !clCheckError(error, "clEnqueueReleaseGLObjects");
}


/*
    error= clEnqueueAcquireGLObjects(clQueue, 1, &_depthBuffer, 0, 0, 0);
    if(checkCLError(error, "clEnqueueAcquireGLObjects depth"))
        return false;
*/
/*
    error= clEnqueueReleaseGLObjects(clQueue, 1, &_depthBuffer, 0, 0, 0);
    if(checkCLError(error, "clEnqueueReleaseGLObjects depth"))
        return false;
*/
