#include "analytics.h"

void Analytics::printTimes()
{
    float clTimeSum= 0;
    QHashIterator<QString, cl_event> i(_clEvents);
    while(i.hasNext()) {
        i.next();
        QString name= i.key();
        cl_event event= i.value();

        cl_uint error;

        cl_ulong startTime;
        cl_ulong endTime;
        error  = clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &startTime, NULL);
        error |= clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &endTime, NULL);
        if(clCheckError(error, "clGetEventProfilingInfo"))
            continue;

        const float execTime= elapsedMSecs(startTime, endTime);
        clTimeSum += execTime;

        qDebug("Event '%s' time %.02f ms.", qPrintable(name), execTime);
    }    

    float timeSum= 0;
    QHashIterator<QString, Event> j(_events);
    while(j.hasNext()) {
        j.next();
        QString name= j.key();
        const Event& event= j.value();

        const float execTime= elapsedMSecs(event.startTime(), event.finishTime());
        timeSum += execTime;

        qDebug("Event '%s' time %.02f ms.", qPrintable(name), execTime);
    }

    qDebug("clEvent Time: %.2f", clTimeSum);
    qDebug("Event Time: %.2f", timeSum);
    qDebug("Total Time: %.2f", clTimeSum + timeSum);
}
