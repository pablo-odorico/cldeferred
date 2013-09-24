#include "autoexposure.h"
#include "debug.h"
#include "analytics.h"
#include <cassert>

AutoExposure::AutoExposure()
    : QObject(), _initialized(false), _autoExposure(true),
      _updateCounter(0), _updatePeriod(1), _lumaData(0),
      _exposure(1.0f), _adjustSpeed(0.05f),
      _downsampleEvent(analytics.event("AE/Downsample")),
      _downloadEvent(analytics.event("AE/Download"))
{
    connect(&_thread, SIGNAL(updateDone()), this, SLOT(updateExposureData()));
    _thread.start();
}

AutoExposure::~AutoExposure()
{
    _thread.stop();
    _thread.wait();
    free(_lumaData);
}

bool AutoExposure::init(cl_context context, cl_device_id device, QSize computeSize, int updatePeriod)
{
    assert(!_initialized);

    _updatePeriod= updatePeriod;
    _lumaSize= computeSize;

    // Compile kernel and set arguments
    CLUtils::KernelDefines downDefines;
    downDefines["GAMMA_CORRECT"]= "2.2f";
    _downKernel= CLUtils::loadKernelPath(context, device, ":/kernels/lumaDownsample.cl",
            "lumaDownsample", downDefines, QStringList("../res/kernels/"));
    if(!_downKernel) {
        debugWarning("Could not compile kernel.");
        return false;
    }

    _lumaData= (uchar*)malloc(lumaDataBytes());
    if(!_lumaData) {
        debugWarning("Could not allocate data.");
        return false;
    }

    cl_int error;
    // This image could be WRITE_ONLY
    _lumaImage= clCreateImage2D(context, CL_MEM_READ_WRITE, clFormatGL(GL_R),
            _lumaSize.width(), _lumaSize.height(), 0, 0, &error);
    if(clCheckError(error, "clCreateImage2D")) {
        free(_lumaData);
        return false;
    }

    _initialized= true;
    return true;
}

void AutoExposure::update(cl_command_queue queue, cl_mem image)
{
    assert(_initialized);

    // If auto-exposure is disabled, don't do anything
    if(!_autoExposure)
        return;

    // Increse/decrease exposure from the difference between the current luma
    // average with an expected 0.5 frame average.
    _exposure *= 1.0f + qBound(-_adjustSpeed, 0.5f - _exposureData.meteringAverage, _adjustSpeed);

    _updateCounter++;
    if(_updateCounter % _updatePeriod)
        return;
    _updateCounter= 0;

    int ai= 0;
    clKernelArg(_downKernel, ai++, image);
    clKernelArg(_downKernel, ai++, _lumaImage);
    if(!clLaunchKernelEvent(_downKernel, queue, _lumaSize, _downsampleEvent))
        return;

    // Download image data and wait for the execution to be done (sync the queue)
    size_t origin[3]= { 0,0,0 };
    size_t region[3]= { (size_t)_lumaSize.width(), (size_t)_lumaSize.height(), 1 };
    cl_int error= clEnqueueReadImage(queue, _lumaImage, CL_FALSE, origin, region,
                                     _lumaSize.width(),0,_lumaData,0,0, &_downloadEvent);
    if(clCheckError(error, "clEnqueueReadImage"))
        return;

    error= clSetEventCallback(_downloadEvent, CL_COMPLETE, exposureCallback, (void*)this);
    clCheckError(error, "clSetEventCallback");

    // When the download is done, exposureCallback will be called
}

// This callback will be called in non-blocking mode when lumaData is ready
void CL_CALLBACK exposureCallback(cl_event event, cl_int, void* user_data)
{
    AutoExposure* object= static_cast<AutoExposure*>(user_data);
    // Enqueue update
    // When ExposureThread is done, it will emit updateDone signal and
    // the updateExposureData() will be called in a thread-safe manner.
    object->_thread.update(object->_lumaData, object->_lumaSize);
    // The updateExposureData will be called after the thread has finished working
}

void AutoExposure::updateExposureData()
{
    _exposureData= _thread.exposureData();
}

