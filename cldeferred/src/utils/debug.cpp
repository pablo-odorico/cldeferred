#include "debug.h"

#ifdef __linux
    #include <unistd.h>
    #include <stdio.h>
#endif

DebugColors::DebugColors()
{
    _colors.resize(colorCount);

#ifdef __linux
    bool useColors= isatty(STDOUT_FILENO);
#else
    bool useColors= false;
#endif

    if(useColors) {
        _colors[RED]    = "\033[0;31m";
        _colors[GREEN]  = "\033[0;32m";
        _colors[VIOLET] = "\033[0;35m";
        _colors[CYAN]   = "\033[0;36m";
        _colors[YELLOW] = "\033[0;33m";
        _colors[DEFAULT]= "\033[0m";
    }
}
