#ifndef CL_CAMERA_H
#define CL_CAMERA_H

#include "cltypes.h"

typedef struct  __attribute__ ((packed)) {
    cl_float16 viewMatrix;
    cl_float16 viewMatrixInv;
    cl_float16 projMatrix;
    cl_float16 projMatrixInv;
    cl_float16 vpMatrixInv;    // Inverted View Proj matrix
    cl_float3  position;
    cl_float3  lookVector;
} cl_camera;

#endif // CL_CAMERA
