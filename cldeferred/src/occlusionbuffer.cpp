#include "occlusionbuffer.h"

OcclusionBuffer::OcclusionBuffer()
    : _initialized(false), _spotLightCount(-1)
{
    QFile file(":/kernels/occlusionPass.cl");
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
        qDebug() << "OcclusionKernel::OcclusionKernel: Could not load source.";
    _source= QString(file.readAll());
}

void OcclusionBuffer::init(cl_context context, cl_device_id device)
{
    _context= context;
    _device= device;


    _initialized= true;
}


bool OcclusionBuffer::updateKernel(int spotLightCount)
{
    if(spotLightCount == _spotLightCount)
        return true;

    // If this is not the first time, delete the old kernel
    if(_spotLightCount != -1) {
// TODO
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

    return loadKernel(_context, &_kernel, _device, sourceCopy, "occlusionPass", "-I../res/kernels/");
}

bool OcclusionBuffer::update(
        cl_command_queue queue,
        cl_mem cameraStruct,
        cl_mem cameraDepthImg,
        cl_mem spotLightStructs,
        QVector<cl_mem> spotLightDepthImgs,
        cl_mem occlusionBuffer,
        QSize screenSize)
{
    if(!_initialized) {
        qDebug() << "OcclusionKernel::operator(): Not initialized.";
        return;
    }

    // Update kernel to match the current number of lights
    if(!updateKernel(spotLightDepthImgs.count())) {
        qDebug() << "OcclusionKernel::operator(): Failed to update kernel.";
        return;
    }

    cl_int error;

    error  = clEnqueueAcquireGLObjects(queue, 1, &cameraDepthImg, 0, 0, 0);
    error |= clEnqueueAcquireGLObjects(queue, spotLightDepthImgs.count(), spotLightDepthImgs.data(), 0, 0, 0);
    if(checkCLError(error, "clEnqueueAcquireGLObjects"))
        return;

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
    error |= clSetKernelArg(_kernel, argIndex, sizeof(cl_mem), (void*)&occlusionBuffer);

    // Launch kernel
    error |= clEnqueueNDRangeKernel(queue, _kernel, 2, NULL, ndRangeSize, workGroupSize, 0, NULL, NULL);
    checkCLError(error, "occlusionKernel");

    error  = clEnqueueReleaseGLObjects(queue, 1, &cameraDepthImg, 0, 0, 0);
    error |= clEnqueueReleaseGLObjects(queue, spotLightDepthImgs.count(), spotLightDepthImgs.data(), 0, 0, 0);
    if(checkCLError(error, "clEnqueueReleaseGLObjects"))
        return;
}
