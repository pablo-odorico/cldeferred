kernel void lumaDownsample(
    read_only image2d_t src,
    global uchar* dst,
    int dstWidth,
    int dstHeight
) {
    const int2 dstPos= (int2)(get_global_id(0), get_global_id(1));

    if(dstPos.x >= dstWidth || dstPos.y >= dstHeight)
        return;

    const float2 srcNormPos= (float2)((float)dstPos.x/dstWidth, (float)dstPos.y/dstHeight);

    const sampler_t sampler= CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_LINEAR;
    const float4 srcColor= read_imagef(src, sampler, srcNormPos);

#ifdef LUMA_IN_ALPHA
    const float luma= srcColor.w;
#else
    const float luma= dot(srcColor.xyz, (float3)(0.299f, 0.587f, 0.114f));
#endif

    dst[dstPos.x + dstPos.y * dstWidth]= clamp((int)(luma * 255.0f), 0, 255);
}
