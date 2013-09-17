#include "clutils.cl"

#include "cl_camera.h"
#include "cl_spotlight.h"

// Image sampler
const sampler_t sampler= CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

//
// Deferred pass kernel
//

kernel
void deferredPass(
    read_only  image2d_t gbDiffuseMat,
    read_only  image2d_t gbNormals,
    read_only  image2d_t gbDepth,
    read_only global uchar* occlusionBuffer,
    write_only image2d_t output,
    // constant cl_material* materials,
    constant cl_camera* camera,
    constant cl_spotlight* spotLights,
    int spotLightsCount,
    int lightsWithShadows
)
{
    // Get global position
    const int2 pos= (int2)(get_global_id(0), get_global_id(1));
    // Size of the G-Buffer, must be <= the size of the output
    const int2 size= get_image_dim(gbDiffuseMat);

    if(pos.x >= size.x || pos.y >= size.y)
        return;

    // Load data from G-Buffer
    const float4 diffuseMat= read_imagef(gbDiffuseMat, sampler, pos);

    const uchar matId= clamp((int)(diffuseMat.w * 255.0f), 0, 255);
    //cl_material mat= materials[matId];

    float3 normal= read_imagef(gbNormals, sampler, pos).xyz;
    normal.z= sqrt(1.0f - normal.x*normal.x - normal.y*normal.y);

/*
    float depth= read_imagef(gbDepth, sampler, pos).x;
    float4 clipPos= getClipPosFromDepth(pos, size, depth, camera->projMatrix);
    float4 viewPos= multMatVec(camera->projMatrixInv, viewPos);
    float4 worldPos= multMatVec(camera->viewMatrixInv, viewPos);
*/

    global uchar* occlusionPtr= occlusionBuffer + (pos.x + pos.y * size.x) * lightsWithShadows;
/*
    float occlusion= 0.0f;
    int spotsWithShadows= 0;
    for(int i=0; i<spotLightsCount; i++) {
        if(spotLights[i].hasShadows) {
            occlusion = max(occlusion, unpackOcclusion(occlusionPtr[spotsWithShadows]));
            spotsWithShadows++;
        }
    }
    //occlusion /= spotsWithShadows;
    */

    float occlusion = unpackOcclusion(occlusionPtr[0]);

    // Write output
    float4 color= diffuseMat * (1.0f - occlusion);

    // FXAA:
    // Pre-compute and store the Luma value in the Alpha Channel
    color.w= dot(color.xyz, (float3)(0.299f, 0.587f, 0.114f));

    write_imagef(output, pos, color);
}
