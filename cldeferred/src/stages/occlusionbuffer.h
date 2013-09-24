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
                cl_mem spotLightStructs, cl_mem dirLightStructs,
                QVector<cl_mem> spotLightDepthImgs, QVector<cl_mem> dirLightDepthImgs,
                QSize screenSize);

    cl_mem& buffer();

    size_t bufferBytes() const {
        return _size.width() * _size.height() * (_spotLightCount + _dirLightCount);
    }

private:
    Q_DISABLE_COPY(OcclusionBuffer)

    bool updateKernel(int spotLightCount, int dirLightCount);
    bool updateBuffer();

    bool _initialized;
    QString _source;

    // Kernel and parameters used to generate the kernel source
    int _spotLightCount;
    int _dirLightCount;
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
