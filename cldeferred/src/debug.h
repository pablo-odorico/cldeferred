#ifndef DEBUG_H
#define DEBUG_H

#include <iostream>
#include <QDebug>

class DebugColors
{
public:
    static const int colorCount= 6;
    enum Color { RED=0, GREEN, VIOLET, CYAN, YELLOW, DEFAULT };

    static char* string(Color color) {
        static DebugColors instance;
        return instance._colors[color].data();
    }

private:
    DebugColors();
    ~DebugColors() { }
    QVector<QByteArray> _colors;
};

#define debugColor(c) DebugColors::string(DebugColors::c)
#define debugPrint(c, msg) std::cerr << debugColor(c) << msg << debugColor(DEFAULT)

// Like qDebug(), these macros can be used as printf or as a stream

#define debugMsg(...)   \
    debugPrint(CYAN, "** " << __FILE__ << ":" << __LINE__ << ": "); \
    qDebug(__VA_ARGS__)

#define debugWarning(...)   \
    debugPrint(YELLOW, "ww " << __FILE__ << ":" << __LINE__ << ": "); \
    qDebug(__VA_ARGS__)

#define debugError(...)   \
    debugPrint(RED, "!! " << __FILE__ << ":" << __LINE__ << ": "); \
    qDebug(__VA_ARGS__)

#endif // DEBUG_H
