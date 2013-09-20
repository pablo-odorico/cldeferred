#include "bloom.h"
#include "debug.h"

Bloom::Bloom() :
    _initialized(false), _visibleImage(0),
    _brightBlend(1.0f), _brightThres(0.5f)
{
}

bool Bloom::init(cl_context context, cl_device_id device)
{
    assert(!_initialized);

    _context= context;
    bool ok;

    // Load the bloom blend kernel    
    ok= CLUtils::loadKernel(_context, &_blendKernel, device,
           ":/kernels/bloomBlend.cl", "bloomBlend",
            "-D GAMMA_CORRECT=2.2f -D LUMA_IN_ALPHA -I../res/kernels/ -Werror");
    if(!ok)
        return false;

    // Load the downsample kernel
    ok= CLUtils::loadKernel(_context, &_downKernel, device,
            ":/kernels/downHalfFilter.cl", "downHalfFilter",
            "-D RAD=2 -Werror");
    if(!ok)
        return false;

    ok= CLUtils::loadKernel(_context, &_upKernel, device,
            ":/kernels/downHalfFilter.cl", "downHalfFilter",
            "-D RAD=0 -Werror");


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
    _visibleImage= clCreateImage2D(_context, CL_MEM_READ_WRITE, &bloomFormat,
                                   size.width(), size.height(), 0, 0, &error);
    if(checkCLError(error, "clCreateImage2D"))
        return false;


    foreach(cl_mem mem, _brightImages)
        checkCLError(clReleaseMemObject(mem), "clReleaseMemObject");
    _brightImages.resize(_brightLevels);

    for(int i=0; i<_brightLevels; i++) {
        _brightImages[i]= clCreateImage2D(_context, CL_MEM_READ_WRITE, &bloomFormat,
            brightSize(i).width(), brightSize(i).height(), 0, 0, &error);
        if(checkCLError(error, "clCreateImage2D"))
            return false;
    }

    return true;
}

bool Bloom::update(cl_command_queue queue, cl_mem outputImage)
{
    assert(_initialized);

    // Downsample the brightImage
    for(int i=0; i<_brightLevels-1; i++) {
        const QSize size1= brightSize(i);
        const QSize size2= brightSize(i+1);
        cl_int2 srcSize= { size1.width(), size1.height() };
        cl_int2 dstSize= { size2.width(), size2.height() };
        int ai= 0;
        //clKernelArg(_downKernel, ai++, srcSize);
        //clKernelArg(_downKernel, ai++, dstSize);
        clKernelArg(_downKernel, ai++, _brightImages[i]);
        clKernelArg(_downKernel, ai++, _brightImages[i+1]);
        if(!clLaunchKernel(_downKernel, queue, size2))
            return false;
    }

    for(int i=_brightLevels-1; i>0; i--) {
        const QSize size1= brightSize(i);
        const QSize size2= brightSize(i-1);
        cl_int2 srcSize= { size1.width(), size1.height() };
        cl_int2 dstSize= { size2.width(), size2.height() };
        int ai= 0;
        //clKernelArg(_downKernel, ai++, srcSize);
        //clKernelArg(_downKernel, ai++, dstSize);
        clKernelArg(_upKernel, ai++, _brightImages[i]);
        clKernelArg(_upKernel, ai++, _brightImages[i-1]);
        if(!clLaunchKernel(_upKernel, queue, size2))
            return false;
    }

    //assert(_brightLevels == 4);

    int ai= 0;
    clKernelArg(_blendKernel, ai++, _visibleImage);
    clKernelArg(_blendKernel, ai++, _brightImages[0]);
    clKernelArg(_blendKernel, ai++, _brightImages[1]);
    clKernelArg(_blendKernel, ai++, _brightImages[2]);
    clKernelArg(_blendKernel, ai++, _brightImages[2]);
    clKernelArg(_blendKernel, ai++, outputImage);
    clKernelArg(_blendKernel, ai++, _brightBlend);
    return clLaunchKernel(_blendKernel, queue, _size);

}

QSize Bloom::brightSize(int level)
{
    assert(level <= _brightLevels);

    // The first level will have the same size as the visibleImage
    if(!level)
        return _size;

    const QSize baseSize= CLUtils::roundUp(_size, 1 << (_brightLevels-1));
    return baseSize / (1 << level);
}
