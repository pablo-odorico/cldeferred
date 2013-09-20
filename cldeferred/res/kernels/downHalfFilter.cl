//
// Downsamples src image into dst which should be exactly half the size of src
// Each dst pixel is set to the average of four src texles
//
// Configuration defines:
// - SIZE_ARGS: If set, the size of the images is NOT read from the image2d_t's

kernel void downHalfFilter(
#ifdef SIZE_ARGS
    int2 srcSize,
    int2 dstSize,
#endif
    read_only  image2d_t src,
    write_only image2d_t dst
) {
    const int2 dstPos= (int2)(get_global_id(0), get_global_id(1));

#ifndef SIZE_ARGS
    const int2 dstSize= get_image_dim(dst);
    const int2 srcSize= get_image_dim(src);
#endif

    if(dstPos.x >= dstSize.x || dstPos.y >= dstSize.y)
        return;    

    const sampler_t sampler= CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_LINEAR;

    float4 srcColor= (float4)(0);

#ifndef RAD
#define RAD 0
#endif

//#define RAD 0
    for(int y=-RAD; y<=RAD; y++)
    for(int x=-RAD; x<=RAD; x++)
    {
        const float2 dstNormPos= (float2)((float)(dstPos.x+x)/dstSize.x, (float)(dstPos.y+y)/dstSize.y);
        float2 srcPos= (float2)(dstNormPos.x * srcSize.x, dstNormPos.y * srcSize.y);
        srcPos += (float2)(1.0f, 1.0f);
        srcColor += read_imagef(src, sampler, srcPos);
    }
    srcColor /= (RAD*2 + 1)*(RAD*2 + 1);


    write_imagef(dst, dstPos, srcColor);
}
