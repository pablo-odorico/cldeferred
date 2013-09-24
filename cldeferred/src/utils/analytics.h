#ifndef ANALYTICS_H
#define ANALYTICS_H

#include "clutils.h"
#include "singleton.h"

#define analytics Singleton<Analytics>::instance()

class Analytics
{
public:
    void printTimes();

    bool eventExists(QString name) { return _events.contains(name); }
    cl_event& event(QString name) { return _events[name]; }

private:
    float elapsedMSecs(cl_ulong start, cl_ulong finish) {
        return (double)(finish-start)/1e6; }

    QHash<QString, cl_event> _events;
};

#endif // ANALYTICS_H
