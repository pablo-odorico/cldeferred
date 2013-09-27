// http://http.developer.nvidia.com/GPUGems3/gpugems3_ch27.html

#include "clutils.cl"
#include "cl_camera.h"

kernel void motionBlur(
    read_only  image2d_t input,
    read_only  image2d_t depths,
    write_only image2d_t output,
    constant cl_camera* camera
) {
    const int2 pos= (int2)(get_global_id(0), get_global_id(1));
    const int2 size= get_image_dim(input);

    if(pos.x >= size.x || pos.y >= size.y)
        return;

    const sampler_t sampler= CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_LINEAR;

    float depth= read_image1f(depths, sampler, pos);
    if(depth == 0.0f) {
        // Pass-through if the depth is 0 (no fragments)
        write_imagef(output, pos, read_imagef(input, sampler, pos));
        return;
    }

    // Get current and previous clip position
    float4 clipPosNew= getClipPosFromDepth(pos, size, depth, camera->projMatrix);
    float4 clipPosOld= multMatVec(camera->motionBlurMatrix, clipPosNew);

    const float2 ndcPosNew= clipPosNew.xy / clipPosNew.w;
    const float2 ndcPosOld= clipPosOld.xy / clipPosOld.w;

    // Position delta ("velocity") in normalized coords
    float2 deltaPos= (ndcPosNew - ndcPosOld) / 2.0f;
    // Position delta in pixels
    deltaPos.x *= size.x;
    deltaPos.y *= size.y;

    const float minStep= 0.5f;
    const int maxSamples= 10;

    // The number of samples will depend on the displacement magnitude and the
    // step magnitude, but should be at least 1, and no greater that maxSamples
    const float deltaLen= length(deltaPos);
    //deltaPos= (deltaPos/deltaLen) * min(deltaLen, maxDelta);
    int samples= clamp((int)(deltaLen/minStep), 1, maxSamples);
    const float2 sampleStep= deltaPos/samples;

    // Sample "backwards" from the current position to the old one
    float2 samplePos= (float2)(pos.x + 0.5f, pos.y + 0.5f);
    float3 color= (float3)(0);
    for(int i=0; i<samples; i++) {
        color += read_image3f(input, sampler, samplePos);
        samplePos -= sampleStep;
    }
    color /= samples;

    write_imagef(output, pos, (float4)(color, 1));
}
