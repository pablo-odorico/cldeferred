//#include "clutils.cl"

#include "cl_camera.h"

// Image sampler
const sampler_t sampler= CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;


float4 unpackColor(const uint color)
{
    return (float4)(
        clamp(((color >> 16) & 0xFF) / 255.0f, 0.0f, 1.0f),
        clamp(((color >> 8 ) & 0xFF) / 255.0f, 0.0f, 1.0f),
        clamp(((color >> 0 ) & 0xFF) / 255.0f, 0.0f, 1.0f),
        clamp(((color >> 24) & 0xFF) / 255.0f, 0.0f, 1.0f));

}


//
// Kernel
//

__kernel
void deferredPass(
    read_only  image2d_t gbDiffuseSpec,
    read_only  image2d_t gbNormals,
    read_only  image2d_t gbDepth,
    read_only global uint* occlusionBuffer,
    write_only image2d_t output,
    constant cl_camera* camera
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
    uint occlusion= occlusionBuffer[pos.x + pos.y * size.x];

    // Write output
    const float4 color= unpackColor(occlusion);


    write_imagef(output, pos, color);
}
