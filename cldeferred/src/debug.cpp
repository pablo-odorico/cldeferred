#include "debug.h"

#ifdef __linux
    #include <unistd.h>
    #include <stdio.h>
#endif

namespace Debug
{

void init()
{
#ifdef __linux
    bool colors= isatty(STDOUT_FILENO);
#else
    bool colors= false;
#endif
/*
    if(!colors) {
        // Colors
        red[0]= 0;
        green[0]= 0;
        violet[0]= 0;
        cyan[0]= 0;
        yellow[0]= 0;
        // Default color
        white[0]= 0;
    }*/
}

}


