#ifndef CL_SPOTLIGHT_H
#define CL_SPOTLIGHT_H

#include "cltypes.h"

typedef struct __attribute__ ((packed)) {
    cl_float16 viewProjMatrix;
} cl_spotlight;

#endif // CL_SPOTLIGHT_H