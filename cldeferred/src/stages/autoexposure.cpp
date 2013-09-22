#include "autoexposure.h"
#include "debug.h"
#include <cassert>

AutoExposure::AutoExposure()
    : QObject(), _initialized(false), _autoExposure(true),
      _updateCounter(0), _updatePeriod(1), _lumaData(0),
      _exposure(1.0f), _adjustSpeed(0.05f)
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
    _downSize= computeSize;

    // Compile kernel and set arguments
    CLUtils::KernelDefines downDefines;
    downDefines["GAMMA_CORRECT"]= "2.2f"; // visibleImage is linear
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
            _downSize.width(), _downSize.height(), 0, 0, &error);
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
    if(_updateCounter % _updatePeriod) {
        // Interpolate here
        return;
    }
    _updateCounter= 0;

    int ai= 0;
    clKernelArg(_downKernel, ai++, image);
    clKernelArg(_downKernel, ai++, _lumaImage);
    if(!clLaunchKernel(_downKernel, queue, _downSize))
        return;

    // Download image data and wait for the execution to be done (sync the queue)
    size_t origin[3]= { 0,0,0 };
    size_t region[3]= { (size_t)_downSize.width(), (size_t)_downSize.height(), 1 };
    cl_int error= clEnqueueReadImage(queue, _lumaImage, CL_TRUE, origin, region,
                                     _downSize.width(),0,_lumaData,0,0,0);
    if(clCheckError(error, "clEnqueueReadImage"))
        return;

    // Enqueue update
    _thread.update(_lumaData, _downSize);

    // When ExposureThread is done, it will emit updateDone signal and
    // the updateExposureData() will be called in a thread-safe manner.
}

void AutoExposure::updateExposureData()
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
