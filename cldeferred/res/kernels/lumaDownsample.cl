//
// Stores the luma of src on the first channel of dst. dst can have any size and
// aspect ratio (eg. 256x256).
//
// Configuration defines:
// - GAMMA_CORRECT: If src is in linear space, then GAMMA_CORRECT should be set
//   with the gamma value so the gamma-corrected luma value is stored.


#include "clutils.cl"

kernel void lumaDownsample(
    read_only  image2d_t src,
    write_only image2d_t dst
) {
    const int2 dstPos= (int2)(get_global_id(0), get_global_id(1));
    const int2 dstSize= get_image_dim(dst);

    if(dstPos.x >= dstSize.x || dstPos.y >= dstSize.y)
        return;

    const sampler_t sampler= CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_LINEAR;

    const float3 srcColor= read_image3f(src, sampler, normPos(dstPos, dstSize));
    float luma= dot(srcColor, (float3)(0.299f, 0.587f, 0.114f));

#ifdef GAMMA_CORRECT
    luma= native_powr(luma, 1.0f/GAMMA_CORRECT);
#endif

    luma= clamp(luma, 0.0f, 1.0f);

    write_imagef(dst, dstPos, (float4)(luma));
}
