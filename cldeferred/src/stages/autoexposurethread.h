#ifndef AUTOEXPOSURETHREAD_H
#define AUTOEXPOSURETHREAD_H

#include <QtGui>

class AutoExposureThread : public QThread
{
    Q_OBJECT
public:
    // http://en.wikipedia.org/wiki/Metering_mode
    enum MeteringMode {
        AverageMetering=0, SpotMetering, PartialMetering, WeightedMetering
    };

    typedef struct {
        // All values are between 0 and 1
        float max;
        float min;
        float average;
        float meteringAverage;
    } LumaData;

    explicit AutoExposureThread(QObject *parent = 0);
    ~AutoExposureThread();

    // 1. start()
    // 2. Enqueue a computation with update
    void update(uchar* lumas, QSize size);
    // 3. Wait for updateDone signal (or not)
    // 4. Get the exposure data (thread-safe)
    LumaData exposureData();
    // 5. stop()
    void stop();

    void waitResult(int timeout=-1) { _workDone.tryAcquire(1, timeout); }

    // The default metering mode is WeightedMetering
    MeteringMode meteringMode() const { return _meteringMode; }
    void setMeteringMode(MeteringMode value) { _meteringMode= value; }

signals:
    void updateDone();

protected:
    void run();

private:
    size_t bytes() { return _size.width() * _size.height() * sizeof(uchar); }
    size_t meteringBytes() { return _size.width() * _size.height() * sizeof(float); }

    // This function should ONLY be called from update, when we are sure the thread
    // is not working
    void updateMeteringWeights();

    uchar* _lumas;
    float* _meteringWeights;
    QSize _size;

    MeteringMode _meteringMode;
    MeteringMode _currentMeteringMode;

    QSemaphore _stop;
    QSemaphore _workPending;
    QSemaphore _working;
    QSemaphore _workDone;

    QMutex _outputDataLock;
    LumaData _outputData;
};

#endif // EXPOSURETHREAD_H
