#ifndef AUTOEXPOSURE_H
#define AUTOEXPOSURE_H

#include <QtCore>
#include "utils/clutils.h"
#include "autoexposurethread.h"

class AutoExposure : public QObject
{
Q_OBJECT
public:
    typedef AutoExposureThread::MeteringMode MeteringMode;
    typedef AutoExposureThread::LumaData     LumaData;

    AutoExposure();
    ~AutoExposure();

    // Init MUST be called before calling update()
    bool init(cl_context context, cl_device_id device,
              QSize computeSize= QSize(256,256), int updatePeriod=1);

    // Update will block until the luma image is computed and downloaded, but
    // then it will return after enqueing the work to ExposureThread
    // image should be LINEAR
    void update(cl_command_queue queue, cl_mem image);

    bool enabled() { return _autoExposure; }
    void setEnabled(bool value) { _autoExposure= value; }
    void toggleEnabled() { _autoExposure= !_autoExposure; }

    MeteringMode meteringMode() { return _thread.meteringMode(); }
    void setMeteringMode(MeteringMode mode) { _thread.setMeteringMode(mode); }

    void setExposure(float value) { _exposure= value; }
    float exposure() const { return _exposure; }

    int updatePeriod() const { return _updatePeriod; }
    void setUpdatePeriod(int value) { _updatePeriod= value; }

    float adjustSpeed() const { return _adjustSpeed; }
    void setAdjustSpeed(float value) { _adjustSpeed= value; }

    // Current luma information
    float minLuma() const { return _exposureData.min; }
    float maxLuma() const { return _exposureData.max; }
    float averageLuma() const { return _exposureData.average; }
    float meteringAverageLuma() const { return _exposureData.meteringAverage; }

    QSize computeSize() const { return _lumaSize; }

    // For debugging
    cl_mem lumaImage() { return _lumaImage; }

private:
    size_t lumaDataBytes() { return _lumaSize.width() * _lumaSize.height(); }
    bool _initialized;

    float _exposure;

    bool _autoExposure;
    float _adjustSpeed;
    int _updatePeriod;
    int _updateCounter;

    QSize _lumaSize;
    uchar* _lumaData;
    cl_mem _lumaImage;
    cl_kernel _downKernel;

    LumaData _exposureData;
    AutoExposureThread _thread;

private slots:
    void updateExposureData();
};

#endif // AUTOEXPOSURE_H
