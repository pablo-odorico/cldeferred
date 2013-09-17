#ifndef CL_DIRLIGHT_H
#define CL_DIRLIGHT_H

#include "cltypes.h"

typedef struct __attribute__ ((packed))
{
    cl_float16 viewProjMatrix;
    cl_float3 position;
    cl_float3 lookVector;

    cl_float3 ambient;
    cl_float3 diffuse;
    cl_float3 specular;

    bool hasShadows;

} cl_dirlight;

#endif // CL_DIRLIGHT_H
