#ifndef FBOCL_H
#define FBOCL_H

#include <QtCore>
#include "fbo.h"
#include "clutilfunctions.h"

class FBOCL : public FBO, protected CLUtilFunctions
{
public:
    FBOCL() : FBO() {}
    virtual ~FBOCL() { }

    bool init(cl_context clContext, QSize size,
        QList<GLenum> colorFormats= QList<GLenum>() << GL_RGBA,
        GLenum depthFormat= GL_DEPTH_COMPONENT24);

    bool enqueueAquireBuffers(cl_command_queue clQueue);
    bool enqueueReleaseBuffers(cl_command_queue clQueue);

    cl_mem getColorBuffer(int i) { return _colorBuffers[i]; }
    //cl_mem getDepthBuffer() { return _depthBuffer; }

private:
    // OpenCL-mapped FBO buffers
    QVector<cl_mem> _colorBuffers;
    //cl_mem _depthBuffer;
};

#endif // FBOCL_H
