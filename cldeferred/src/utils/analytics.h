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
        bool running() { return _running; }
        float elapsedMSecs() {
            return _running ? Analytics::elapsedMSecs(_startTime, analytics.nowNSecs())
                            : Analytics::elapsedMSecs(_startTime, _finishTime);
        }
    private:
        bool _running;
        qint64 _startTime;
        qint64 _finishTime;
    };

    class EventTimer {
    public:
        EventTimer(Event& event) : _event(event) { _event.start(); }
        EventTimer(QString eventName) : _event(analytics.event(eventName)) { _event.start(); }
        ~EventTimer() { finish(); }
        void finish() { if(_event.running()) _event.finish(); }
    private:
        Event& _event;
    };

    Analytics();

    void fpsUpdate();
    void printTimes();

    bool clEventExists(QString name) const { return _clEvents.contains(name); }
    cl_event& clEvent(QString name) { return _clEvents[name]; }

    bool eventExists(QString name) const { return _events.contains(name); }
    Event& event(QString name) { return _events[name]; }

    qint64 nowNSecs() const { return _timer.nsecsElapsed(); }
    float nowMsecs() const { return nowNSecs()/1e6f; }

    static float elapsedMSecs(cl_ulong start, cl_ulong finish)
        { return qMax((double)(finish-start)/1e6, 0.0); }
    static float elapsedMSecs(qint64 start, qint64 finish)
        { return qMax((double)(finish-start)/1e6, 0.0); }

private:
    QElapsedTimer _timer;

    QHash<QString, cl_event> _clEvents;
    QHash<QString, Event> _events;

    int fpsFrameCount;
    qint64 fpsLastTime;
};

#endif // ANALYTICS_H
