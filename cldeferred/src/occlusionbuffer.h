#ifndef OCCLUSIONBUFFER_H
#define OCCLUSIONBUFFER_H

#include "clutilfunctions.h"

class OcclusionBuffer : protected CLUtilFunctions
{
public:
    OcclusionBuffer();

    // Resize MUST be called before using the buffer
    bool resize(cl_context context, cl_device_id device, QSize size);

    // cameraDepthImg and spotLightDepthImgs must be aquired before calling update
    bool update(cl_command_queue queue,
                cl_mem cameraStruct,
                cl_mem cameraDepthImg,
                cl_mem spotLightStructs,
                QVector<cl_mem> spotLightDepthImgs,
                QSize screenSize);

    cl_mem buffer();

    int bufferBytes() { return _bufferSize.width() * _bufferSize.height() * sizeof(uint); } // TODO ver si pasar a float

private:
    bool updateKernel(int spotLightCount);

    bool _initialized;
    QString _source;

    int _spotLightCount;
    cl_kernel _kernel;

    // CL context and device, used to compile the kernel
    cl_context _context;
    cl_device_id _device;

    // Global memory buffer of uint32_t's
    QSize _bufferSize;
    cl_mem _buffer;
};

#endif // OCCLUSIONBUFFER_H
