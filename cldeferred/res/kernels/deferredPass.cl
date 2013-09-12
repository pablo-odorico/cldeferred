#include "clutils.cl"

#include "cl_camera.h"
#include "cl_spotlight.h"

// Image sampler
const sampler_t sampler= CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;


//
// Kernel
//

__kernel
void deferredPass(
    read_only  image2d_t gbDiffuseSpec,
    read_only  image2d_t gbNormals,
    read_only  image2d_t gbDepth,
    read_only global float* occlusionBuffer,
    write_only image2d_t output,
    constant cl_camera* camera,
    constant cl_spotlight* spotLights
)
{
    // Get global position
    const int2 pos= (int2)(get_global_id(0), get_global_id(1));
    // Size of the G-Buffer, must be <= the size of the output
    const int2 size= get_image_dim(gbDiffuseSpec);

    if(pos.x >= size.x || pos.y >= size.y)
        return;

    // Load data from G-Buffer
    float4 diffuseSpec= read_imagef(gbDiffuseSpec, sampler, pos);

    float3 normal= read_imagef(gbNormals, sampler, pos).xyz;
    normal.z= sqrt(1.0f - normal.x*normal.x - normal.y*normal.y);

/*
    float depth= read_imagef(gbDepth, sampler, pos).x;
    float4 clipPos= getClipPosFromDepth(pos, size, depth, camera->projMatrix);
    float4 viewPos= multMatVec(camera->projMatrixInv, viewPos);
    float4 worldPos= multMatVec(camera->viewMatrixInv, viewPos);
*/
    float occlusion= occlusionBuffer[pos.x + pos.y * size.x];

    // Write output
    const float4 color= diffuseSpec * occlusion;

    //const float4 color= unpackColor(occlusion);

    write_imagef(output, pos, color);
}
