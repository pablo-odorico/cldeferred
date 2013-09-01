#include "occlusionbuffer.h"
#include <cassert>

OcclusionBuffer::OcclusionBuffer()
    : _initialized(false), _spotLightCount(-1), _bufferSize(0,0)
{
    QFile file(":/kernels/occlusionPass.cl");
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
        qDebug() << "OcclusionBuffer::OcclusionBuffer: Could not load source.";
    _source= QString(file.readAll());
}

bool OcclusionBuffer::resize(cl_context context, cl_device_id device, QSize size)
{
    _context= context;
    _device= device;

    // If the buffer was allocated, release it
    const bool firstTime= _bufferSize == QSize(0,0);
    if(!firstTime) {
        const cl_int error= clReleaseMemObject(_buffer);
        checkCLError(error, "clReleaseMemObject");
    }

    _bufferSize= size;

    // Allocate occlusion buffer
    cl_int error;
    _buffer= clCreateBuffer(_context, CL_MEM_READ_WRITE, bufferBytes(), NULL, &error);
    if(checkCLError(error, "clCreateBuffer"))
        return false;

    _initialized= true;

    return true;
}


bool OcclusionBuffer::updateKernel(int spotLightCount)
{
    assert(_initialized);

    if(spotLightCount == _spotLightCount)
        return true;

    // If this is not the first time, delete the old kernel
    const bool firstTime= _spotLightCount == -1;
    if(!firstTime) {
        const cl_int error= clReleaseKernel(_kernel);
        checkCLError(error, "clReleaseKernel");
    }
    _spotLightCount= spotLightCount;

    QString sourceCopy= _source;

    // Kernel parameters
    QString depthParams= "";
    for(int i=0; i<_spotLightCount; i++)
        depthParams += "DEF_DEPTH_PARAM(spotLight, " + QString::number(i) + "), \n";
    sourceCopy.replace("/** DEPTH_PARAMS **/", depthParams);

    // Occlusion calculation calls
    QString occlusions= "";
    for(int i=0; i<_spotLightCount; i++)
        occlusions += "SET_OCCLUSION(spotLights, " + QString::number(i) + ", spotLight) \n";
    sourceCopy.replace("/** OCCLUSIONS **/", occlusions);

    return loadKernel(_context, &_kernel, _device, sourceCopy, "occlusionPass", "-I../res/kernels/ -Werror");
}

bool OcclusionBuffer::update(
        cl_command_queue queue,
        cl_mem cameraStruct,
        cl_mem cameraDepthImg,
        cl_mem spotLightStructs,
        QVector<cl_mem> spotLightDepthImgs,
        QSize screenSize)
{
    assert(_initialized);

    // Update kernel to match the current number of lights
    if(!updateKernel(spotLightDepthImgs.count())) {
        qDebug() << "OcclusionBuffer::operator(): Failed to update kernel.";
        return false;
    }

    cl_int error;

    // Work group and NDRange
    size_t workGroupSize[2] = { 16, 16 };
    size_t ndRangeSize[2];
    ndRangeSize[0]= roundUp(screenSize.width() , workGroupSize[0]);
    ndRangeSize[1]= roundUp(screenSize.height(), workGroupSize[1]);

    // Set kernel parameters
    error  = clSetKernelArg(_kernel, 0, sizeof(cl_mem), (void*)&cameraStruct);
    error |= clSetKernelArg(_kernel, 1, sizeof(cl_mem), (void*)&cameraDepthImg);
    error |= clSetKernelArg(_kernel, 2, sizeof(cl_mem), (void*)&spotLightStructs);
    uint argIndex= 3;
    for(int i=0; i<spotLightDepthImgs.count(); i++) {
        error |= clSetKernelArg(_kernel, argIndex, sizeof(cl_mem), (void*)&spotLightDepthImgs[i]);
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
    return _buffer;
}
