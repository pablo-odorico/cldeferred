#ifndef EXPOSURETHREAD_H
#define EXPOSURETHREAD_H

#include <QtGui>

class ExposureThread : public QThread
{
    Q_OBJECT
public:
    typedef struct {
        float max;
        float min;
        float average;
    } Data;

    explicit ExposureThread(QObject *parent = 0);
    ~ExposureThread();

    // 1. start()
    // 2. Enqueue a computation with update
    void update(uchar* lumas, size_t count);
    // 3. Wait for updateDone signal (or not)
    // 4. Get the exposure data (thread-safe)
    Data exposureData();
    // 5. stop()
    void stop();

    void waitResult(int timeout=-1) { _workDone.tryAcquire(1, timeout); }

signals:
    void updateDone();

protected:
    void run();
    
private:
    size_t bytes() { return _count * sizeof(uchar); }

    uchar* _lumas;
    size_t _count;

    QSemaphore _stop;
    QSemaphore _workPending;
    QSemaphore _working;
    QSemaphore _workDone;

    QMutex _outputDataLock;
    Data _outputData;
};

#endif // EXPOSURETHREAD_H
