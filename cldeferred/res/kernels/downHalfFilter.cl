//
// Downsamples src image into dst which should be exactly half the size of src
// Each dst pixel is set to the average of four src texels
//

#include "clutils.cl"

kernel void downHalfFilter(
    read_only  image2d_t src,
    write_only image2d_t dst
) {
    const int2 dstPos= (int2)(get_global_id(0), get_global_id(1));

    const int2 dstSize= get_image_dim(dst);
    const int2 srcSize= get_image_dim(src);

    if(dstPos.x >= dstSize.x || dstPos.y >= dstSize.y)
        return;

    const sampler_t sampler= CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_LINEAR;

    const float2 dstNormPos= normalizePos(dstPos, dstSize);
    float2 srcPos= (float2)(dstNormPos.x * srcSize.x + 1.0f, dstNormPos.y * srcSize.y + 1.0f);
    float3 srcColor= read_image3f(src, sampler, srcPos);

    write_imagef(dst, dstPos, (float4)(srcColor, 1.0f));
}
