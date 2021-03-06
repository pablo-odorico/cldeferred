#ifndef DEBUG_H
#define DEBUG_H

#include <iostream>
#include <QDebug>
#include <typeinfo>

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


#define debugMsg(...) { \
    debugPrint(CYAN, "** " << __FILE__ << ":" << __LINE__ << ": "); \
    qDebug(__VA_ARGS__); }

#define debugWarning(...) { \
    debugPrint(YELLOW, "ww " << __FILE__ << ":" << __LINE__ << ": "); \
    qDebug(__VA_ARGS__); }

#define debugFatal(...) { \
    debugPrint(RED, "!! " << __FILE__ << ":" << __LINE__ << ": "); \
    qDebug(__VA_ARGS__); exit(EXIT_FAILURE); }

#endif // DEBUG_H
