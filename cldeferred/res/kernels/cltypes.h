#ifndef CLTYPES_H
#define CLTYPES_H

#ifdef __cplusplus

    // If included from C++, get types from cl.h
    #include <CL/cl_platform.h>
    #define cl_float    float

#else
#ifdef CL_VERSION_1_0

    // If included from OpenCL code, define types to match the native ones
    #define cl_float    float
    #define cl_float2   float2
    #define cl_float3   float3
    #define cl_float4   float4
    #define cl_float8   float8
    #define cl_float16  float16
    // TODO add more
#else

#error This file should be included either from C++ or an OpenCL compiler.

#endif
#endif

#endif // CLTYPES_H
