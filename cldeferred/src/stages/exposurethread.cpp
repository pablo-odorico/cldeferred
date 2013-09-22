#include "exposurethread.h"
#include "debug.h"
#include <cassert>

ExposureThread::ExposureThread(QObject *parent) :
    QThread(parent), _lumas(0),
    _meteringWeights(0), _meteringMode(WeightedMetering)
{
}

ExposureThread::~ExposureThread()
{
    free(_lumas);
    free(_meteringWeights);
}

void ExposureThread::update(uchar* lumas, QSize size)
{
    if(_working.available()) {
        debugWarning("Thread busy, ignoring.");
        return;
    }
    bool updateWeights= _currentMeteringMode != _meteringMode;

    // Resize/alloc local buffer
    if(size.width()*size.height() != _size.width()*_size.height()) {
        _size= size;
        _lumas= (uchar*)realloc(_lumas, bytes());
        assert(_lumas);
        _meteringWeights= (float*)realloc(_meteringWeights, meteringBytes());
        assert(_meteringWeights);
        updateWeights= true;
    }
    if(updateWeights)
        updateMeteringWeights();

    // Copy data to local buffer
    memcpy(_lumas, lumas, bytes());

    // Set the work pending flag
    _workPending.release();
}

ExposureThread::LumaData ExposureThread::exposureData()
{
    LumaData ret;
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

void ExposureThread::updateMeteringWeights()
{
    debugMsg("Updating metering weights.");

    // The thread should be suspended
    assert(!_working.available());

    float weightSum= 0;
    //float maxWeight= -1.0f;

    for(uint y=0; y<_size.height(); y++) {
        for(uint x=0; x<_size.width(); x++) {
            const QVector2D vec((float)x/_size.width()-0.5f,(float)y/_size.height()-0.5f);
            const float radius= vec.length();
            float weight= 0;

            switch(_meteringMode) {
            case AverageMetering:  weight= 1.0f; break;
            case SpotMetering:     weight= (radius < 0.05f) ? 1.0f : 0.0f; break;
            case PartialMetering:  weight= (radius < 0.25f) ? 1.0f : 0.0f; break;
            case WeightedMetering: weight= exp(-30.0f * (vec.x()*vec.x() + vec.y()*vec.y())); break;
            default: debugFatal("Metering mode not implemented."); break;
            }
            _meteringWeights[x + y * _size.width()]= weight;
            weightSum += weight;
            //maxWeight= qMax(maxWeight, weight);
        }
    }
/*
    QImage image(_size, QImage::Format_RGB888);
    for(uint y=0; y<_size.height(); y++) {
        for(uint x=0; x<_size.width(); x++) {
            const float value= _meteringWeights[x + y * _size.width()]/maxWeight;
            const uchar color= qMin((int)(255.0f * value), 255);
            image.setPixel(x, y, qRgb(color,color,color));
        }
    }
    image.save("weights.png");
*/
    // Normalize weights so the sum is 1
    const size_t count= _size.width() * _size.height();
    for(uint i=0; i<count; i++)
        _meteringWeights[i] /= weightSum;

    _currentMeteringMode= _meteringMode;
}

void ExposureThread::run()
{
    while(!_stop.tryAcquire()) {
        // Wait for a job to come in, but check every 250 msecs if we need to stop
        if(!_workPending.tryAcquire(1, 250))
            continue;
        _working.release();

        // Reset work done flag
        while(_workDone.tryAcquire(1)) ;

        uchar max= 0;
        uchar min= 255;
        uint sum= 0;
        float meteringAverage= 0;

        const uint count= _size.width() * _size.height();
        for(uint i=0; i<count; i++) {
            const uchar luma= _lumas[i];
            max= qMax(max, luma);
            min= qMin(min, luma);
            sum += luma;
            meteringAverage += luma * _meteringWeights[i];
        }
        const float average= (double)sum / count;

        // Commit results
        _outputDataLock.lock();
        _outputData.max= max / 255.0f;
        _outputData.min= min / 255.0f;
        _outputData.average= average / 255.0f;
        _outputData.meteringAverage= meteringAverage / 255.0f;
        _outputDataLock.unlock();

        emit updateDone();

        // Unset working flag
        _working.acquire();
        // Set work done flag
        _workDone.release();
    }
}
