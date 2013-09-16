#ifndef FBOCL_H
#define FBOCL_H

#include <QtCore>
#include "fbo.h"
#include "clutils.h"

class FBOCL : public FBO
{
public:
    FBOCL() : FBO() { }
    virtual ~FBOCL() { }

    bool resize(cl_context context, QSize size,
        QList<GLenum> colorFormats= QList<GLenum>() << GL_RGBA,
        GLenum depthFormat= GL_DEPTH_COMPONENT24);

    // Acquire/release buffers
    bool acquireColorBuffers(cl_command_queue queue);
    bool releaseColorBuffers(cl_command_queue queue);
    // Returns mapped OpenCL images for all the color attachments
    // If acquireColorBuffers was not called, clEnqueueAcquireGLObjects must be
    // called before using the buffers
    QVector<cl_mem> colorBuffers();

    //cl_mem acquireDepthBuffer(cl_command_queue queue);
    //bool releaseDepthBuffer(cl_command_queue queue);

private:
    // OpenCL-mapped FBO buffers
    QVector<cl_mem> _colorBuffers;
    //cl_mem _depthBuffer;
};

#endif // FBOCL_H
