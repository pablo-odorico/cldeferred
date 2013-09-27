#ifndef ANALYTICS_H
#define ANALYTICS_H

#include "clutils.h"
#include "singleton.h"

#define analytics Singleton<Analytics>::instance()

class Analytics
{
public:

    class Event {
    public:
        Event() : _startTime(0), _finishTime(0) { }
        void start() { _startTime= analytics.nowNSecs(); _running= true; }
        void finish() { _finishTime= analytics.nowNSecs(); _running= false; }
        qint64 startTime() const { return _startTime; }
        qint64 finishTime() const { return _running ? _startTime : _finishTime; }
    private:
        bool _running;
        qint64 _startTime;
        qint64 _finishTime;
    };

    Analytics() { _timer.start(); }

    void printTimes();

    bool clEventExists(QString name) const { return _clEvents.contains(name); }
    cl_event& clEvent(QString name) { return _clEvents[name]; }

    bool eventExists(QString name) const { return _events.contains(name); }
    Event& event(QString name) { return _events[name]; }

    qint64 nowNSecs() const { return _timer.nsecsElapsed(); }
    qint64 nowMsecs() const { return nowNSecs()/1e6f; }

private:
    float elapsedMSecs(cl_ulong start, cl_ulong finish)
        { return (double)(finish-start)/1e6; }
    float elapsedMSecs(qint64 start, qint64 finish)
        { return (double)(finish-start)/1e6; }

    QElapsedTimer _timer;

    QHash<QString, cl_event> _clEvents;
    QHash<QString, Event> _events;
};

#endif // ANALYTICS_H
