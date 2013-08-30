#ifndef FBOCL_H
#define FBOCL_H

#include <QtCore>
#include "fbo.h"
#include "clutilfunctions.h"

class FBOCL : public FBO, protected CLUtilFunctions
{
public:
    FBOCL() : FBO() { }
    virtual ~FBOCL() { }

    bool resize(cl_context context, QSize size,
        QList<GLenum> colorFormats= QList<GLenum>() << GL_RGBA,
        GLenum depthFormat= GL_DEPTH_COMPONENT24);

    QVector<cl_mem> aquireColorBuffers(cl_command_queue queue);
    bool releaseColorBuffers(cl_command_queue queue);

    //cl_mem aquireDepthBuffer(cl_command_queue queue);
    //bool releaseDepthBuffer(cl_command_queue queue);

private:
    // OpenCL-mapped FBO buffers
    QVector<cl_mem> _colorBuffers;
    //cl_mem _depthBuffer;
};

#endif // FBOCL_H
