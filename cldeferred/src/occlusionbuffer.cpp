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
        checkCLError(error, "clReleaseKernel");
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

    return CLUtils::loadKernel(_context, &_kernel, _device, sourceCopy,
                               "occlusionPass", "-I../res/kernels/ -Werror");
}

bool OcclusionBuffer::updateBuffer()
{
    if(_lastBufferBytes == bufferBytes())
        return true;

    cl_int error;

    if(_lastBufferBytes) {
        error= clReleaseMemObject(_buffer);
        checkCLError(error, "clReleaseMemObject");
    }

    // Allocate occlusion buffer
    _buffer= clCreateBuffer(_context, CL_MEM_READ_WRITE, bufferBytes(), NULL, &error);
    if(checkCLError(error, "clCreateBuffer"))
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

    cl_int error;

    // Work group and NDRange
    size_t workGroupSize[2] = { 16, 16 };
    size_t ndRangeSize[2];
    ndRangeSize[0]= CLUtils::roundUp(screenSize.width() , workGroupSize[0]);
    ndRangeSize[1]= CLUtils::roundUp(screenSize.height(), workGroupSize[1]);

    // Set kernel parameters
    error  = clSetKernelArg(_kernel, 0, sizeof(cl_mem), (void*)&cameraStruct);
    error |= clSetKernelArg(_kernel, 1, sizeof(cl_mem), (void*)&cameraDepthImg);
    error |= clSetKernelArg(_kernel, 2, sizeof(int)   , (void*)&lightsWithShadows);
    error |= clSetKernelArg(_kernel, 3, sizeof(cl_mem), (void*)&spotLightStructs);
    error |= clSetKernelArg(_kernel, 4, sizeof(cl_mem), (void*)&dirLightStructs);
    uint argIndex= 5;
    for(int i=0; i<_spotLightCount; i++) {
        error |= clSetKernelArg(_kernel, argIndex, sizeof(cl_mem), (void*)&spotLightDepthImgs[i]);
        argIndex++;
    }
    for(int i=0; i<_dirLightCount; i++) {
        error |= clSetKernelArg(_kernel, argIndex, sizeof(cl_mem), (void*)&dirLightDepthImgs[i]);
        argIndex++;
    }
    error |= clSetKernelArg(_kernel, argIndex, sizeof(cl_mem), (void*)&_buffer);

    // Launch kernel
    error |= clEnqueueNDRangeKernel(queue, _kernel, 2, NULL, ndRangeSize, workGroupSize, 0, NULL, NULL);
    if(checkCLError(error, "OcclusionBuffer kernel"))
        return false;

    return true;
}

cl_mem OcclusionBuffer::buffer()
{
    assert(_initialized);
    assert(_lastBufferBytes);
    return _buffer;
}
