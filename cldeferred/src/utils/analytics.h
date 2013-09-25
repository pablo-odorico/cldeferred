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
        Event() : _startTime(analytics.nowNSecs()), _finishTime(0) { }
        void start() { _startTime= analytics.nowNSecs(); }
        void finish() { _finishTime= analytics.nowNSecs(); }
    private:
        qint64 _startTime;
        qint64 _finishTime;
    };

    void printTimes();

    bool clEventExists(QString name) { return _clEvents.contains(name); }
    cl_event& clEvent(QString name) { return _clEvents[name]; }

    bool eventExists(QString name) { return _events.contains(name); }
    Event& event(QString name) { return _events[name]; }

    qint64 nowNSecs() { return _timer.nsecsElapsed(); }
    qint64 nowMsecs() { return nowNSecs()/1e6f; }

private:
    float elapsedMSecs(cl_ulong start, cl_ulong finish)
        { return (double)(finish-start)/1e6; }

    QElapsedTimer _timer;

    QHash<QString, cl_event> _clEvents;
    QHash<QString, Event> _events;

    QMultiHash<QString, float> times;
};

#endif // ANALYTICS_H
