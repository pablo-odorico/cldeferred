#ifndef CL_SPOTLIGHT_H
#define CL_SPOTLIGHT_H

#include "cltypes.h"

typedef struct __attribute__ ((packed))
{
    cl_float16 viewProjMatrix;

    cl_float3 position;

    cl_float3 lookVector;

    cl_float3 ambient;
    cl_float3 diffuse;
    cl_float3 specular;

    float cutOffMin; // Angle when the spots starts to fade in radians
    float cutOffMax; // Cutoff angle in radians

    float exponent;
    float linearAtenuation;

    bool hasShadows;

} cl_spotlight;

#endif // CL_SPOTLIGHT_H
