#include "analytics.h"

void Analytics::printTimes()
{
    QHashIterator<QString, cl_event> i(_events);
    while (i.hasNext()) {
        i.next();
        QString name= i.key();
        cl_event event= i.value();

        cl_uint error;

        cl_ulong queuedTime;
        cl_ulong startTime;
        cl_ulong endTime;
        error  = clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_QUEUED, sizeof(cl_ulong), &queuedTime, NULL);
        error |= clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &startTime, NULL);
        error |= clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &endTime, NULL);
        if(clCheckError(error, "clGetEventProfilingInfo"))
            continue;

        const float waitTime= elapsedMSecs(queuedTime, startTime);
        const float execTime= elapsedMSecs(startTime, endTime);

        qDebug("Event '%s': %.02f + %.02f = %.02f ms.", qPrintable(name), waitTime, execTime, waitTime+execTime);
    }
}
