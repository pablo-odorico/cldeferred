#include "bloom.h"
#include "debug.h"

Bloom::Bloom() :
    _initialized(false), _enabled(true),
    _bloomBlend(1.0f), _bloomThres(0.5f)
{
}

bool Bloom::init(cl_context context, cl_device_id device)
{
    assert(!_initialized);

    _context= context;

    // Load the downsample kernel
    CLUtils::KernelDefines downDefines;
    downDefines["DST_BLUR_SIZE"]= "3";
    _downKernel= CLUtils::loadKernelPath(_context, device, ":/kernels/bloomDown.cl",
            "bloomDown", downDefines, QStringList("../res/kernels/"));
    if(!_downKernel)
        return false;

    // Load the bloom blend kernel
    CLUtils::KernelDefines blendDefines;
    blendDefines["GAMMA_CORRECT"]= "2.2f";
    blendDefines["LUMA_IN_ALPHA"]= "1";
    _blendKernel= CLUtils::loadKernelPath(_context, device, ":/kernels/bloomBlend.cl",
            "bloomBlend", blendDefines, QStringList("../res/kernels/"));
    if(!_blendKernel)
        return false;

    // Load the bloom bypass kernel (uses the same defines as _blendKernel)
    _bypassKernel= CLUtils::loadKernelPath(_context, device, ":/kernels/bloomBlend.cl",
            "bloomBypass", blendDefines, QStringList("../res/kernels/"));
    if(!_bypassKernel)
        return false;

    _initialized= true;
    return true;
}

bool Bloom::resize(QSize inputSize)
{
    assert(_initialized);

    if(_inputSize == inputSize)
        return true;
    _inputSize= inputSize;
    // Choose between 3 and 4 levels, depending on the resolution: 4 for 1080p, 3 for 720p
    _levels= qMax(3, inputSize.width()/450);

    // Release previous images
    foreach(cl_mem mem, _images)
        clCheckError(clReleaseMemObject(mem), "clReleaseMemObject");

    cl_int error;

    // Create/resize the input image and it's downsamples
    _images.resize(_levels);

    QString bloomSizes= "";
    for(int i=0; i<_levels; i++) {
        const QSize size= imageSize(i);
        _images[i]= clCreateImage2D(_context, CL_MEM_READ_WRITE, clFormatGL(GL_RGBA16F),
                size.width(), size.height(), 0, 0, &error);
        if(clCheckError(error, "clCreateImage2D"))
            return false;

        bloomSizes += QString(" (%1 x %2)").arg(size.width()).arg(size.height());
    }
    debugMsg("Bloom level sizes:%s", qPrintable(bloomSizes));

    return true;
}

bool Bloom::update(cl_command_queue queue, cl_mem outputImage)
{
    assert(_initialized);

    if(!_enabled) {
        // If blur is not enabled, call the bypass kernel and return
        int ai= 0;
        clKernelArg(_bypassKernel, ai++, _images[0]);
        clKernelArg(_bypassKernel, ai++, outputImage);
        return clLaunchKernel(_bypassKernel, queue, _inputSize);
    }

    // Downsample _level-1 times
    for(int i=0; i<_levels-1; i++) {
        const QSize size= imageSize(i);
        cl_int2 srcSize= { size.width(), size.height() };
        cl_mem blurWeights= CLUtils::gaussianKernel(clInfo.context, clInfo.queue, QSize(3,3));

        int ai= 0;
        clKernelArg(_downKernel, ai++, _images[i]);
        clKernelArg(_downKernel, ai++, _images[i+1]);
        clKernelArg(_downKernel, ai++, srcSize);
        clKernelArg(_downKernel, ai++, blurWeights);
        if(!clLaunchKernel(_downKernel, queue, imageSize(i+1)))
            return false;
    }

    // Call bloomBlend using _image[0] as the input image, and the last downsample
    // as the bloom image.
    int ai= 0;
    clKernelArg(_blendKernel, ai++, _images[0]);
    clKernelArg(_blendKernel, ai++, _enabled ? _images[_levels-1] : _images[0]);
    clKernelArg(_blendKernel, ai++, outputImage);
    clKernelArg(_blendKernel, ai++, _bloomBlend);
    return clLaunchKernel(_blendKernel, queue, _inputSize);
}

QSize Bloom::imageSize(int level)
{
    assert(level>=0 and level<=_levels);

    // The first level will have the same size as the visibleImage
    if(!level)
        return _inputSize;

    const QSize baseSize= CLUtils::roundUp(_inputSize, 1 << (_levels-1));
    return baseSize / (1 << level);
}
