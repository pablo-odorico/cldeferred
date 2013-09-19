#ifndef EXPOSURE_H
#define EXPOSURE_H

#include <QtCore>
#include "utils/clutils.h"
#include "exposurethread.h"

class Exposure : public QObject
{    
Q_OBJECT
public:
    Exposure();
    ~Exposure();

    // Init MUST be called before calling update()
    bool init(cl_context context, cl_device_id device, QSize downSize= QSize(320,240), int updatePeriod=1);

    void update(cl_command_queue queue, cl_mem image);

    int updatePeriod() const { return _updatePeriod; }
    void setUpdatePeriod(int value) { _updatePeriod= value; }

    float exposure() const { return _exposure; }

    float adjustSpeed() const { return _adjustSpeed; }
    void setAdjustSpeed(float value) { _adjustSpeed= value; }

    float minLight() const { return _exposureData.min; }
    float maxLight() const { return _exposureData.max; }
    float averageLight() const { return _exposureData.average; }

    QSize downSize() const { return _downSize; }

private slots:
    void updateExposureData();

private:
    size_t lumaDataBytes() { return _downSize.width() * _downSize.height(); }

    bool _initialized;

    uint _updatePeriod;
    uint _updateCounter;

    cl_kernel _downKernel;

    QSize _downSize;
    uchar* _lumaData;
    cl_mem _clLumaData; // OpenCL buffer of uchars

    ExposureThread _thread;
    ExposureThread::Data _exposureData;

    float _exposure;
    float _adjustSpeed;
};

#endif // EXPOSURE_H
