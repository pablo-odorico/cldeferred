#include "clutils.cl"

#include "cl_camera.h"
#include "cl_spotlight.h"

// Variance Shadow Map calculation:
// http://http.developer.nvidia.com/GPUGems3/gpugems3_ch08.html

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

// Samples the depth moments image multiple times and averages the result
// depths must be a float2 image
// coord is the normalized centroid
float2 depthBlurSample(read_only image2d_t depths, int radius, float2 coord)
{
    const sampler_t sampler= CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP | CLK_FILTER_LINEAR;
    const float2 size= (float2)(get_image_width(depths), get_image_height(depths));

    float2 moment= (float2)(0, 0);

    for(int offsetY= -radius; offsetY <= radius; offsetY++)
        for(int offsetX= -radius; offsetX <= radius; offsetX++)
            moment += read_image2f(depths, sampler, coord + (float2)(offsetX/size.x,offsetY/size.y));
    moment /= (2*radius+1) * (2*radius+1);

    return moment;
}

float calcVisibility(read_only image2d_t lightDepths, const float4 worldPos, const float16 lightVPMatrix)
{
    const float4 lightClipPos= multMatVec(lightVPMatrix, worldPos);
    const float4 lightBiasedNDC= (lightClipPos / lightClipPos.w) * 0.5f + 0.5f;
    const float2 lightDepthMoment= depthBlurSample(lightDepths, 1, lightBiasedNDC.xy);
    return varianceShadowMap(lightDepthMoment, lightBiasedNDC.z);
}

//
// Occlusions Pass Kernel
//

#define DEPTH_PARAM_NAME(prefix,N) prefix##N##depth
#define DEF_DEPTH_PARAM(prefix,N)  read_only image2d_t DEPTH_PARAM_NAME(prefix,N)

kernel
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
    const float4 worldPos= multMatVec(camera->vpMatrixInv, clipPos);

#define VISIBILITY(lights,N) \
    calcVisibility(lights##N##depth, worldPos, lights[N].viewProjMatrix)

    /** ISIBILITIES **/ // VISIBILITY(spotLights, 0)
    float visibility= VISIBILITY(spotLights, 0);

    occlusionBuffer[pos.x + pos.y * size.x]= visibility;
}
