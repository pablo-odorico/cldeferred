// http://http.developer.nvidia.com/GPUGems3/gpugems3_ch27.html

#include "clutils.cl"
#include "cl_camera.h"

kernel void motionBlur(
    read_only  image2d_t input,
    read_only  image2d_t depth,
    write_only image2d_t output,
    constant cl_camera* camera
) {
    const int2 pos= (int2)(get_global_id(0), get_global_id(1));
    const int2 size= get_image_dim(input);

    if(pos.x >= size.x || pos.y >= size.y)
        return;

    const sampler_t sampler= CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_LINEAR;

    float depth= read_image1f(depth, sampler, pos);

    // Get current and previous clip position
    const float4 clipPosNew= getClipPosFromDepth(pos, size, depth, camera->projMatrix);
    const float4 clipPosOld= multMatVec(camera->motionBlurMatrix, clipPos1);

    // Position delta ("velocity") in normalized coords
    float2 deltaPos= (clipPosNew.xy - clipPosOld.xy) / 2.0f;
    // Position delta in pixels
    deltaPos.x *= size.x;
    deltaPos.y *= size.y;

    const float step= 0.25f;
    const int maxSamples= 5;

    // The number of samples will depend on the displacement magnitude and the
    // step magnitude, but should be at least 1, and no greater that maxSamples
    const float deltaLen= length(deltaPos);
    const int samples= clamp(deltaLen/step, 1, maxSamples);
    const float2 sampleStep= (deltaPos/deltaLen) * step;

    // Sample "backwards" from the current position to the old one
    float2 samplePos= (float2)(pos.x + 0.5f, pos.y + 0.5f);
    float3 color= (float3)(0);
    for(int i=0; i<samples; i++) {
        color += read_image3f(input, sampler, samplePos);
        samplePos -= sampleStep;
    }
    color /= samples;

    write_imagef(output, pos, (float4)(color, 1.0f));
}
