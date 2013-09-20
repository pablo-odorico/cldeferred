#include "exposure.h"
#include "debug.h"
#include <cassert>

Exposure::Exposure()
    : QObject(), _initialized(false),
      _updateCounter(0), _updatePeriod(1), _lumaData(0),
      _exposure(1.0f), _adjustSpeed(0.05f)
{
    connect(&_thread, SIGNAL(updateDone()), this, SLOT(updateExposureData()));
    _thread.start();
}

Exposure::~Exposure()
{
    _thread.stop();
    _thread.wait();
    free(_lumaData);
}

bool Exposure::init(cl_context context, cl_device_id device, QSize downSize, int updatePeriod)
{
    assert(!_initialized);

    _updatePeriod= updatePeriod;
    _downSize= downSize;

    // Compile kernel and set arguments
    bool ok= CLUtils::loadKernel(context, &_downKernel, device, ":/kernels/lumaDownsample.cl",
                                 "lumaDownsample", "-D LUMA_IN_ALPHA -Werror");
    if(!ok) {
        debugWarning("Could not compile kernel.");
        return false;
    }

    _lumaData= (uchar*)malloc(lumaDataBytes());
    if(!_lumaData) {
        debugWarning("Could not allocate data.");
        return false;
    }

    cl_int error;
    _clLumaData= clCreateBuffer(context, CL_MEM_WRITE_ONLY, lumaDataBytes(), NULL, &error);
    if(checkCLError(error, "clCreateBuffer")) {
        free(_lumaData);
        return false;
    }

    _initialized= true;
    return true;
}

void Exposure::update(cl_command_queue queue, cl_mem image)
{
    assert(_initialized);

    // Increse/decrease exposure from the difference between the current luma
    // average with an expected 0.5 frame average.
    _exposure *= 1.0f + qBound(-_adjustSpeed, 0.5f - _exposureData.average, _adjustSpeed);

    _updateCounter++;
    if(_updateCounter % _updatePeriod) {
        // Interpolate here
        return;
    }
    _updateCounter= 0;

    int ai= 0;
    clKernelArg(_downKernel, ai++, image);
    clKernelArg(_downKernel, ai++, _clLumaData);
    clKernelArg(_downKernel, ai++, _downSize.rwidth());
    clKernelArg(_downKernel, ai++, _downSize.rheight());
    if(!clLaunchKernel(_downKernel, queue, _downSize))
        return;

    // Download data and wait for the execution to be done
    // This will sync the queue
    cl_int error= clEnqueueReadBuffer(queue, _clLumaData, CL_TRUE, 0,
                                      lumaDataBytes(), _lumaData, 0, NULL, NULL);
    if(checkCLError(error, "clEnqueueReadBuffer"))
        return;

    // Enqueue update
    _thread.update(_lumaData, _downSize.width() * _downSize.height());

    // When ExposureThread is done, it will emit updateDone signal and
    // the updateExposureData() will be called in a thread-safe manner.
}

void Exposure::updateExposureData()
{
    _exposureData= _thread.exposureData();
}


/*

For some reason, using event callbacks is using 100% of a CPU core...

// exposureCallback will be called after the downsample is calculated
// and downloaded to _lumaData
error= clSetEventCallback(event, CL_COMPLETE, &exposureCallback, (void*)this);
checkCLError(error, "clSetEventCallback");


// This callback will be called in non-blocking mode when lumaData is ready
void exposureCallback(cl_event event, cl_int, void* user_data)
{
    clReleaseEvent(event);
    return;
    Exposure& object= *static_cast<Exposure*>(user_data);
    // Enqueue update
    const int count= object._downSize.width() * object._downSize.height();
    object._thread.update(object._lumaData, count);
    // The updateExposureData will be called after the thread has finished working
}

*/
