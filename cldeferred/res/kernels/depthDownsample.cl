kernel void depthDownsample(
    read_only  image2d_t src,
    write_only image2d_t dst
) {
    const int2 dstPos= (int2)(get_global_id(0), get_global_id(1));
    const int2 dstSize= get_image_dim(dst);
    const int2 srcSize= get_image_dim(src);

    if(dstPos.x >= dstSize.x || dstPos.y >= dstSize.y)
        return;

    const float2 dstNormPos= (float2)((float)dstPos.x/dstSize.x, (float)dstPos.y/dstSize.y);

    float2 srcPos= (float2)(dstNormPos.x * srcSize.x, dstNormPos.y * srcSize.y);
    // Offset the position to sample and average 4 source texels
    srcPos += (float2)(1.0f, 1.0f);

    const sampler_t sampler= CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_LINEAR;
    const float4 srcColor= read_imagef(src, sampler, srcPos);

    write_imagef(dst, dstPos, srcColor);

}
