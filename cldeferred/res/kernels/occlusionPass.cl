#include "clutils.cl"

#include "cl_camera.h"
#include "cl_spotlight.h"

// Image sampler for the depth buffers
const sampler_t sampler= CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_LINEAR;

#define DEPTH_PARAM_NAME(prefix,N) prefix##N##depth
#define DEF_DEPTH_PARAM(prefix,N)  read_only image2d_t DEPTH_PARAM_NAME(prefix,N)

__kernel
void occlusionPass(
    constant cl_camera* camera,
    read_only image2d_t cameraDepth,
    constant cl_spotlight* spotLights,
// DEF_DEPTH_PARAM(spotLight, 0),
/** DEPTH_PARAMS **/
    write_only global uint* occlusionBuffer
)
{
    const int2 pos= (int2)(get_global_id(0), get_global_id(1));
    const int2 size= get_image_dim(cameraDepth);

    if(pos.x >= size.x || pos.y >= size.y)
        return;

    const float camDepth= read_imagef(cameraDepth, sampler, pos).x;
    const float4 clipPos= getClipPosFromDepth(pos, size, camDepth, camera->projMatrix);
    const float4 worldPos= multMatVec(camera->vpMatrixInv, clipPos);

    uint occlusion= 0;
    float4 lightClipPos;
    float lightDepth;

// lights is the pointer to the light structs (eg. spotLights)
// N: light number (resets for each type)
// depthPrefix: used to create depthPrefix##Ndepth
#define SET_OCCLUSION(lights,N,depthPrefix) \
    lightClipPos= multMatVec(lights[N].viewProjMatrix, worldPos)/2.0f + 0.5f; \
    lightDepth= read_imagef(depthPrefix##N##depth, sampler, lightClipPos.xy).x; \
    occlusion |= (lightDepth < lightClipPos.z) << N;

    // SET_OCCLUSION(spotLights, 0, spotLight)
/** OCCLUSIONS **/

    occlusionBuffer[pos.x + pos.y * size.x]= occlusion;
}
