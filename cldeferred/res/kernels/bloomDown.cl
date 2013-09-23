//
// Downsamples src image into dst which should be half the size of src
// Each dst pixel is set to the average of four src texels
//
// Configuration defines:
// - SIZE_ARGS: If set, the size of the images is NOT read from the image2d_t's
// - DST_BLUR_SIZE: If set, the dst image is applied a DST_BLUR_SIZE x DST_BLUR_SIZE
//   blur filter

#include "clutils.cl"

#ifndef DST_BLUR_SIZE
    #error DST_BLUR_SIZE should be defined.
#endif

#if (DST_BLUR_SIZE & 0x1 == 0)
    #error DST_BLUR_SIZE should be odd.
#endif

#define BLUR_RADIUS  (DST_BLUR_SIZE/2)

kernel void bloomDown(
    read_only  image2d_t src,
    write_only image2d_t dst,
    int2 srcSize,
    constant float* blurWeights
) {
    const int2 dstPos= (int2)(get_global_id(0), get_global_id(1));
    const int2 dstSize= get_image_dim(dst);

    if(dstPos.x >= dstSize.x || dstPos.y >= dstSize.y)
        return;

    const sampler_t sampler= CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_LINEAR;

    // Start color with an offset of 1.0f
    float3 color= (float3)(1);

    for(int y=-BLUR_RADIUS; y<=BLUR_RADIUS; y++) {
        for(int x=-BLUR_RADIUS; x<=BLUR_RADIUS; x++) {
            const float2 dstNormPos= normalizePos(dstPos+(int2)(x,y), dstSize);
            float2 srcPos= (float2)(dstNormPos.x * srcSize.x + 1.0f, dstNormPos.y * srcSize.y + 1.0f);

            // Input color will be above 1.0f
            float3 srcColor= read_image3f(src, sampler, srcPos);
            // Extract the "bright" part of the color
            srcColor= max(srcColor-(float3)(1), (float3)(0));

            const float weight= blurWeights[(x+BLUR_RADIUS) + (y+BLUR_RADIUS) * DST_BLUR_SIZE];

            color += srcColor * weight;
        }
    }

    // Write color, the value will be over 1.0f
    write_imagef(dst, dstPos, (float4)(color, 1.0f));
}
