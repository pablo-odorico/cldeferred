#ifndef DEBUG_H
#define DEBUG_H

#include <QDebug>

namespace Debug {
/*
    // Colors
    char red[15]    = "\033[0;31m";
    char green[15]  = "\032[0;32m";
    char violet[15] = "\035[0;35m";
    char cyan[15]   = "\036[0;36m";
    char yellow[15] = "\036[0;33m";
    // Default color
    char white[15]  = "\033[0m";
*/
    void init();
}
/*
#define debugMsg(format, ...)   qDebug("%s** %s:%d: %s"format, Debug::cyan, __FILE__, __LINE__, Debug::white, __VA_ARGS__)
#define debugWarn(format, ...)  qDebug("%sWW %s:%d: %s"format, Debug::yellow, __FILE__, __LINE__, Debug::white, __VA_ARGS__)
#define debugErr(format, ...)   qDebug("%sEE %s:%d: %s"format, Debug::red, __FILE__, __LINE__, Debug::white, __VA_ARGS__)
*/

#endif // DEBUG_H
