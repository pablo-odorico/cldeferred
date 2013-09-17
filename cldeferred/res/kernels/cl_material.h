#ifndef CL_MATERIAL_H
#define CL_MATERIAL_H

#include "cltypes.h"

typedef struct __attribute__ ((packed))
{
    cl_float3 ambient;
    cl_float3 diffuse;
    cl_float3 specular;
    float     shininess;
} cl_material;

#endif // CL_CAMERA
