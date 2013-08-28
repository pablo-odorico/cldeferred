#ifndef OCCLUSIONBUFFER_H
#define OCCLUSIONBUFFER_H

#include "clutilfunctions.h"

class OcclusionBuffer : protected CLUtilFunctions
{
public:
    OcclusionBuffer();

    // TODO pasar width/height y actualizar buffer
    void init(cl_context context, cl_device_id device);

    bool update(cl_command_queue queue,
                cl_mem cameraStruct,
                cl_mem cameraDepthImg,
                cl_mem spotLightStructs,
                QVector<cl_mem> spotLightDepthImgs,
                cl_mem occlusionBuffer,
                QSize screenSize);

    cl_mem getBuffer() { return _buffer; }

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
    cl_mem _buffer;
};

#endif // OCCLUSIONBUFFER_H
