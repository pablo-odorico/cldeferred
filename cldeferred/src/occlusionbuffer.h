#ifndef OCCLUSIONBUFFER_H
#define OCCLUSIONBUFFER_H

#include "clutils.h"

class OcclusionBuffer
{
public:
    OcclusionBuffer();

    // Resize MUST be called before using the buffer
    bool resize(cl_context context, cl_device_id device, QSize size);

    // cameraDepthImg and spotLightDepthImgs must be acquired before calling update
    bool update(cl_command_queue queue,
                cl_mem cameraStruct,
                cl_mem cameraDepthImg, int lightsWithShadows,
                cl_mem spotLightStructs,
                QVector<cl_mem> spotLightDepthImgs,
                QSize screenSize);

    cl_mem buffer();

    size_t bufferBytes() const { return _size.width() * _size.height() * _spotLightCount; }

private:
    bool updateKernel(int spotLightCount);
    bool updateBuffer();

    bool _initialized;
    QString _source;

    int _spotLightCount;
    cl_kernel _kernel;

    // CL context and device, used to compile the kernel
    cl_context _context;
    cl_device_id _device;

    // Global memory buffer of uint32_t's
    QSize _size;
    cl_mem _buffer;
    size_t _lastBufferBytes;
};

#endif // OCCLUSIONBUFFER_H
