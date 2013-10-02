#ifndef CL_CAMERA_H
#define CL_CAMERA_H

#include "cltypes.h"

typedef struct //__attribute__ ((packed))
{
    cl_float16 viewMatrix;
    cl_float16 viewMatrixInv;
    cl_float16 projMatrix;
    cl_float16 projMatrixInv;
    cl_float16 vpMatrixInv;      // Inverted View Proj matrix

    cl_float16 motionBlurMatrix; // PrevProj * PrevView * InvView * InvProj

    cl_float3 position;
    cl_float3 lookVector;

    // Depth of Field parameters used to calculate the CoC from the depth buffer's Z-value
    // http://http.developer.nvidia.com/GPUGems/gpugems_ch23.html

    // x: COC value for distances smaller than cocMinMaxDist.x
    // y: COC value for distances greater than cocMinMaxDist.y
    cl_float2 cocNearFar;
    // x,y: slope and bias of the 1st line
    // z,w: slope and bias of the 2nd line
    cl_float4 cocParams;

} cl_camera;

#endif // CL_CAMERA
