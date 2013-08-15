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

    // Create depth texture
    glGenTextures(1, &_depthTextureId);
    glBindTexture(GL_TEXTURE_2D, _depthTextureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, _width, _height, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, 0);


    // Map depth buffer
    /*_depthBuffer= clCreateFromGLRenderbuffer(_clContext, CL_MEM_READ_ONLY,
                                             _depthAttach.bufferId, &error);*/
    _depthBuffer= clCreateFromGLTexture2D(_clContext, CL_MEM_READ_ONLY, GL_TEXTURE_2D,
                                          0, _depthTextureId, &error);
    checkCLError(error, "clCreateFromGLRenderbuffer depth attach");

    return true;
}

void FBOCL::cleanup()
{
    FBO::cleanup();
}

void FBOCL::unbind()
{
    if(_bindedTarget != GL_READ_BUFFER) {
        FBO::unbind();
        bind(GL_READ_BUFFER);
    }
    glBindTexture(GL_TEXTURE_2D, _depthTextureId);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, _width, _height);
    FBO::unbind();
}

bool FBOCL::enqueueAquireBuffers()
{
    cl_int error;

    error= clEnqueueAcquireGLObjects(_clQueue, _colorBuffers.count(), _colorBuffers.data(), 0, 0, 0);
    if(checkCLError(error, "FBOCL::enqueueAquireBuffers: clEnqueueAcquireGLObjects colors"))
        return false;

    error= clEnqueueAcquireGLObjects(_clQueue, 1, &_depthBuffer, 0, 0, 0);
    if(checkCLError(error, "FBOCL::enqueueAquireBuffers: clEnqueueAcquireGLObjects depth"))
        return false;

    return true;
}

bool FBOCL::enqueueReleaseBuffers()
{
    cl_int error;

    error= clEnqueueReleaseGLObjects(_clQueue, _colorBuffers.count(), _colorBuffers.data(), 0, 0, 0);
    if(checkCLError(error, "FBOCL::enqueueReleaseBuffers: clEnqueueReleaseGLObjects color"))
        return false;

    error= clEnqueueReleaseGLObjects(_clQueue, 1, &_depthBuffer, 0, 0, 0);
    if(checkCLError(error, "FBOCL::enqueueReleaseBuffers: clEnqueueReleaseGLObjects depth"))
        return false;

    return true;
}
