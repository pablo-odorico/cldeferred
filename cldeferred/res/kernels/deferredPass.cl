#include "clutils.cl"

#include "cl_camera.h"

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
    float4 clipPos= getClipPosFromDepth(pos, size, depth, camera->projMatrix, camera->projMatrixInv);
    float4 viewPos= multMatVec(camera->projMatrixInv, viewPos);
    float4 worldPos= multMatVec(camera->viewMatrixInv, viewPos);

    // Write output
    const float4 color= diffuseSpec;


    write_imagef(output, pos, color);
}
