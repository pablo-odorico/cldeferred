#include "clutils.cl"

#include "cl_camera.h"

// Image sampler
const sampler_t sampler= CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

// Get view-space coord from 0..1 depth
// http://www.opengl.org/wiki/Compute_eye_space_from_window_space#From_XYZ_of_gl_FragCoord
float4 getViewPosFromDepth(
    const int2 pos, const int2 size,
    const float depth,
    const float16 projMatrix, const float16 projMatrixInv)
{
    const float3 ndcPos= 2.0f * (float3)(convert_float2(pos)/size, depth) - 1.0f;

    const float pm23= projMatrix.sE; // Mat. indices  0 1 2 3
    const float pm22= projMatrix.sA; //               4 5 6 7
    const float pm32= projMatrix.sB; //               8 9 A B
                                     //               C D E F
    const float clipW= pm23 / (ndcPos.z - pm22/pm32);
    const float4 clipPos= (float4)(ndcPos * clipW, clipW);

    return multMatVec(projMatrixInv, clipPos);
}

//
// Kernel
//

__kernel
void deferredPass(
    read_only  image2d_t gbDiffuseSpec,
    read_only  image2d_t gbNormals,
    read_only  image2d_t gbDepth,
    write_only image2d_t output,
    constant cl_camera* camera
)
{
    // Get global position and size
    const int2 pos= (int2)(get_global_id(0), get_global_id(1));
    const int2 size= get_image_dim(output);

    if(pos.x >= size.x || pos.y >= size.y)
        return;

    // Load data from G-Buffer
    float4 diffuseSpec= read_imagef(gbDiffuseSpec, sampler, pos);

    float3 normal= read_imagef(gbNormals, sampler, pos).xyz;
    normal.z= sqrt(1.0f - normal.x*normal.x - normal.y*normal.y);

    float depth= read_imagef(gbDepth, sampler, pos).x;
    float4 viewPos= getViewPosFromDepth(pos, size, depth, camera->projMatrix, camera->projMatrixInv);
    float4 worldPos= multMatVec(camera->viewMatrixInv, viewPos);

    // Write output
    const float4 color= diffuseSpec;
    write_imagef(output, pos, color);
}
