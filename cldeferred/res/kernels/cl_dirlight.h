#ifndef CL_DIRLIGHT_H
#define CL_DIRLIGHT_H

#include "cltypes.h"

typedef struct __attribute__ ((packed))
{
    cl_float16 viewProjMatrix;
    bool hasShadows;
} cl_dirlight;

#endif // CL_DIRLIGHT_H
