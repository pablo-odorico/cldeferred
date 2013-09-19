#include "exposurethread.h"
#include "debug.h"
#include <cassert>

ExposureThread::ExposureThread(QObject *parent) :
    QThread(parent), _lumas(0), _count(0)
{
}

ExposureThread::~ExposureThread()
{
    free(_lumas);
}

void ExposureThread::update(uchar* lumas, size_t count)
{
    if(_working.available()) {
        debugWarning("Thread busy, ignoring.");
        return;
    }

    // Resize/alloc local buffer
    if(count != _count) {
        _count= count;
        _lumas= (uchar*)realloc(_lumas, bytes());
        assert(_lumas);
    }
    // Copy data to local buffer
    memcpy(_lumas, lumas, bytes());

    // Set the work pending flag
    _workPending.release();
}

ExposureThread::Data ExposureThread::exposureData()
{
    Data ret;
    _outputDataLock.lock();
    ret= _outputData;
    _outputDataLock.unlock();
    return ret;
}

void ExposureThread::stop()
{
    // Set the stop flag
    _stop.release();
}

void ExposureThread::run()
{
    do {
        // Wait for a job to come in
        if(!_workPending.tryAcquire(1, 250))
            continue;
        _working.release();

        // Reset work done flag
        while(_workDone.tryAcquire(1)) ;

        uchar max= 0;
        uchar min= 255;
        uint sum= 0;
        for(uint i=0; i<_count; i++) {
            uchar& luma= _lumas[i];
            max= qMax(max, luma);
            min= qMin(min, luma);
            sum += luma;
        }
        const float average= (double)sum / _count;

        // Commit result
        _outputDataLock.lock();
        _outputData.max= max / 255.0f;
        _outputData.min= min / 255.0f;
        _outputData.average= average / 255.0f;
        _outputDataLock.unlock();

        emit updateDone();

        // Unset working flag
        _working.acquire();
        // Set work done flag
        _workDone.release();

    } while(!_stop.tryAcquire());
}
