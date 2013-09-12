#include "clutils.cl"

#include "cl_camera.h"
#include "cl_spotlight.h"


#define DEPTH_PARAM_NAME(prefix,N) prefix##N##depth
#define DEF_DEPTH_PARAM(prefix,N)  read_only image2d_t DEPTH_PARAM_NAME(prefix,N)


// http://http.developer.nvidia.com/GPUGems3/gpugems3_ch08.html
//

float linstep(float low, float high, float v)
{
    return clamp((v-low)/(high-low), 0.0f, 1.0f);
}

float varianceShadowMap(const float2 moments, float compare)
{
    const float p = smoothstep(compare-0.001f, compare, moments.x);
    const float variance = max(moments.y - moments.x*moments.x, -0.001);
    const float d = compare - moments.x;
    const float p_max = linstep(0.2f, 1.0f, variance / (variance + d*d));
    return clamp(max(p, p_max), 0.0f, 1.0f);
}


__kernel
void occlusionPass(
    constant cl_camera* camera,
    read_only image2d_t cameraDepth,
    constant cl_spotlight* spotLights,
    /** DEPTH_PARAMS **/ // DEF_DEPTH_PARAM(spotLights, 0),
    write_only global float* occlusionBuffer
)
{
    const int2 pos= (int2)(get_global_id(0), get_global_id(1));
    const int2 size= get_image_dim(cameraDepth);

    if(pos.x >= size.x || pos.y >= size.y)
        return;

    const sampler_t camDepthSampler= CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;
    const float camDepth= read_image1f(cameraDepth, camDepthSampler, pos);

    const float4 clipPos= getClipPosFromDepth(pos, size, camDepth, camera->projMatrix);
    float4 worldPos= multMatVec(camera->vpMatrixInv, clipPos);

    float occlusion= 0;
    float4 lightClipPos;
    float4 lightBiasedNDC;
    float2 lightDepthMoment;

// lights is the pointer to the light structs (eg. spotLights)
// N: light number (resets for each type)
// depthPrefix: used to create depthPrefix##Ndepth

    const sampler_t lightDepthSampler= CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP | CLK_FILTER_LINEAR;

// Light depth moment, sampled with an offset
#define M_BLURRADIUS    1
#define M_BLURCOUNT     ((2*(M_BLURRADIUS)+1)*(2*(M_BLURRADIUS)+1))

#define SET_OCCLUSION(lights,N)                         \
    lightClipPos= multMatVec(lights[N].viewProjMatrix, worldPos);                              \
    lightBiasedNDC= (lightClipPos / lightClipPos.w) * 0.5f + 0.5f;                             \
                                                                                               \
    lightDepthMoment= (float2)(0, 0);                                                          \
    for(int offsetX= -M_BLURRADIUS; offsetX <= M_BLURRADIUS; offsetX++)                        \
        for(int offsetY= -M_BLURRADIUS; offsetY <= M_BLURRADIUS; offsetY++)                    \
            lightDepthMoment += read_image2f(lights##N##depth, lightDepthSampler,              \
                                             lightBiasedNDC.xy + (float2)(offsetX/(float)size.x,offsetY/(float)size.y)); \
    lightDepthMoment /= M_BLURCOUNT;                                                           \
                                                                                               \
    occlusion = varianceShadowMap(lightDepthMoment, lightBiasedNDC.z);

    /** OCCLUSIONS **/ // SET_OCCLUSION(spotLights, 0)

    occlusionBuffer[pos.x + pos.y * size.x]= occlusion;

/*
    //float4 pp= (float4)(camDepth, lightDepth, lightBiasedNDC.z, 0);
    float d= occlusion; // fabs(lightDepth-lightBiasedNDC.z)
    float4 pp= (float4)(pos.x, pos.y, d, 0);
    //float4 pp= (float4)(lightNDCPos.xy, lightNDCPos.z/2.0f + 0.5f, 0);

    if(!camDepth) pp.z= -1;
    occlusionBuffer[pos.x + pos.y * size.x]= pp;
*/
}
