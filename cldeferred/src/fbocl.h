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

    // SetupCL must be called before calling init/cleanup
    void setupCL(cl_context context, cl_command_queue queue);

    bool init(QSize size, QList<GLenum> colorFormats, GLenum depthFormat);

    void unbind();

    bool enqueueAquireBuffers();
    bool enqueueReleaseBuffers();

    cl_mem getColorBuffer(int i) { return _colorBuffers[i]; }
    cl_mem getDepthBuffer() { return _depthBuffer; }

private:
    void cleanup();

    cl_context _clContext;
    cl_command_queue _clQueue;

    // OpenCL-mapped FBO buffers
    QVector<cl_mem> _colorBuffers;
    cl_mem _depthBuffer;

    // Texture used to copy the FBO's depth buffer
    GLuint _depthTextureId;
};

#endif // FBOCL_H
