#include "bloom.h"
#include "debug.h"

Bloom::Bloom() :
    _initialized(false), _brightBlend(0.5f),
    _visibleImage(0), _brightImage(0)
{
}

bool Bloom::init(cl_context context, cl_device_id device)
{
    assert(!_initialized);

    _context= context;

    // Load the bloom blend kernel
    bool ok= CLUtils::loadKernel(_context, &_blendKernel, device,
                   ":/kernels/bloomBlend.cl", "bloomBlend",
                   "-I../res/kernels/ -Werror");

    _initialized= ok;
    return ok;
}

bool Bloom::resize(QSize size)
{
    assert(_initialized);
    _size= size;

    cl_int error;
    cl_image_format bloomFormat;
    CLUtils::gl2clFormat(GL_RGBA16F, bloomFormat);

    if(_visibleImage)
        checkCLError(clReleaseMemObject(_visibleImage), "clReleaseMemObject");
    _visibleImage= clCreateImage2D(_context, CL_MEM_READ_WRITE, &bloomFormat, size.width(), size.height(), 0, 0, &error);
    if(checkCLError(error, "clCreateImage2D"))
        return false;

    if(_brightImage)
        checkCLError(clReleaseMemObject(_brightImage), "clReleaseMemObject");
    _brightImage= clCreateImage2D(_context, CL_MEM_READ_WRITE, &bloomFormat, size.width(), size.height(), 0, 0, &error);
    if(checkCLError(error, "clCreateImage2D"))
        return false;

    return true;
}

bool Bloom::update(cl_command_queue queue, cl_mem outputImage)
{
    assert(_initialized);

    cl_int error;

    // Work group and NDRange
    size_t workGroupSize[2] = { 16, 16 };
    size_t ndRangeSize[2];
    ndRangeSize[0]= CLUtils::roundUp(_size.width() , workGroupSize[0]);
    ndRangeSize[1]= CLUtils::roundUp(_size.height(), workGroupSize[1]);

    // Launch kernel
    error  = clSetKernelArg(_blendKernel, 0, sizeof(cl_mem), (void*)&_visibleImage);
    error |= clSetKernelArg(_blendKernel, 1, sizeof(cl_mem), (void*)&_brightImage);
    error |= clSetKernelArg(_blendKernel, 2, sizeof(cl_mem), (void*)&outputImage);
    error |= clSetKernelArg(_blendKernel, 3, sizeof(float) , (void*)&_brightBlend);
    error |= clEnqueueNDRangeKernel(queue, _blendKernel, 2, NULL,
                                    ndRangeSize, workGroupSize, 0, NULL, NULL);
    if(checkCLError(error, "Blend Kernel"))
        return false;

    return true;
}
