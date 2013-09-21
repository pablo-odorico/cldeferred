#include "occlusionbuffer.h"
#include "debug.h"
#include <cassert>

OcclusionBuffer::OcclusionBuffer()
    : _initialized(false), _spotLightCount(-1), _dirLightCount(-1),
      _size(0,0), _lastBufferBytes(0)
{
    QFile file(":/kernels/occlusionPass.cl");
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
        debugFatal("Could not load source.");
    _source= QString(file.readAll());
}

bool OcclusionBuffer::resize(cl_context context, cl_device_id device, QSize size)
{
    _context= context;
    _device= device;
    _size= size;

    _initialized= true;
    return true;
}


bool OcclusionBuffer::updateKernel(int spotLightCount, int dirLightCount)
{
    assert(_initialized);

    if(spotLightCount==_spotLightCount and dirLightCount==_dirLightCount)
        return true;

    // If this is not the first time, delete the old kernel
    const bool firstTime= _spotLightCount==-1 or _dirLightCount==-1;
    if(!firstTime) {
        const cl_int error= clReleaseKernel(_kernel);
        clCheckError(error, "clReleaseKernel");
    }
    _spotLightCount= spotLightCount;
    _dirLightCount= dirLightCount;

    QString sourceCopy= _source;

    // Kernel parameters
    QString depthParams= "";
    for(int i=0; i<_spotLightCount; i++)
        depthParams += "DEF_DEPTH_PARAM(spotLights, " + QString::number(i) + "), \n";
    for(int i=0; i<_dirLightCount; i++)
        depthParams += "DEF_DEPTH_PARAM(dirLights, " + QString::number(i) + "), \n";
    sourceCopy.replace("/** DEPTH_PARAMS **/", depthParams);

    // Occlusion calculation calls
    QString occlusions= "";
    for(int i=0; i<_spotLightCount; i++)
        occlusions += "VISIBILITY(spotLights, " + QString::number(i) + ", visibilitySpot) \n";
    for(int i=0; i<_dirLightCount; i++)
        occlusions += "VISIBILITY(dirLights, " + QString::number(i) + ", visibility) \n";
    sourceCopy.replace("/** VISIBILITIES **/", occlusions);

    _kernel= CLUtils::loadKernelText(_context, _device, sourceCopy.toLatin1(), "occlusionPass",
            CLUtils::KernelDefines(), QStringList("../res/kernels/"));

    return _kernel != 0;
}

bool OcclusionBuffer::updateBuffer()
{
    if(_lastBufferBytes == bufferBytes())
        return true;

    cl_int error;

    if(_lastBufferBytes) {
        error= clReleaseMemObject(_buffer);
        clCheckError(error, "clReleaseMemObject");
    }

    // Allocate occlusion buffer
    _buffer= clCreateBuffer(_context, CL_MEM_READ_WRITE, bufferBytes(), NULL, &error);
    if(clCheckError(error, "clCreateBuffer"))
        return false;

    _lastBufferBytes= bufferBytes();
    return true;
}

bool OcclusionBuffer::update(cl_command_queue queue,
        cl_mem cameraStruct,
        cl_mem cameraDepthImg,
        int lightsWithShadows,
        cl_mem spotLightStructs, cl_mem dirLightStructs,
        QVector<cl_mem> spotLightDepthImgs, QVector<cl_mem> dirLightDepthImgs,
        QSize screenSize)
{
    assert(_initialized);

    // Update kernel to match the current number of lights
    const int spotCount= spotLightDepthImgs.count();
    const int dirCount= dirLightDepthImgs.count();
    if(!updateKernel(spotCount, dirCount)) {
        debugFatal("Failed to update kernel.");
        return false;
    }
    // Update occlusion buffer to store the occlusion of all lights
    if(!updateBuffer()) {
        debugFatal("Failed to update buffer.");
        return false;
    }

    // Set kernel args and launch it
    int ai= 0;
    clKernelArg(_kernel, ai++, cameraStruct);
    clKernelArg(_kernel, ai++, cameraDepthImg);
    clKernelArg(_kernel, ai++, lightsWithShadows);
    clKernelArg(_kernel, ai++, spotLightStructs);
    clKernelArg(_kernel, ai++, dirLightStructs);
    for(int i=0; i<_spotLightCount; i++)
        clKernelArg(_kernel, ai++, spotLightDepthImgs[i]);
    for(int i=0; i<_dirLightCount; i++)
        clKernelArg(_kernel, ai++, dirLightDepthImgs[i]);
    clKernelArg(_kernel, ai++, _buffer);

    return clLaunchKernel(_kernel, queue, screenSize);
}

cl_mem &OcclusionBuffer::buffer()
{
    assert(_initialized);
    assert(_lastBufferBytes);
    return _buffer;
}
