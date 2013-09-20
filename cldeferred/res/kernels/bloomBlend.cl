//
// Bloom blending of the "visible" and filtered "bright" images
//
// Configuration defines:
// - GAMMA_CORRECT: If set, the final tone-mapped color is gamma corrected to
//   the define's value.
// - LUMA_IN_ALPHA: If set, the luma of the final color is calculated and stored
//   on the alpha channel of the image.

#include "clutils.cl"

kernel void bloomBlend(
    // All images must be the same size
    // LINEAR Visible and filtered bright bloom images
    read_only  image2d_t srcVisible,
    read_only  image2d_t srcBright0,
    read_only  image2d_t srcBright1,
    read_only  image2d_t srcBright2,
    read_only  image2d_t srcBright3,
    // Gamma-corrected blended output (can be 8 bits)
    write_only image2d_t dst,
    float brightBlend
) {
    const int2 pos= (int2)(get_global_id(0), get_global_id(1));
    const int2 size= get_image_dim(dst);

    if(pos.x >= size.x || pos.y >= size.y)
        return;

    const sampler_t sampler= CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;
    const float2 normPos= (float2)((float)pos.x/size.x, (float)pos.y/size.y);

    const float3 visibleColor= read_image3f(srcVisible, sampler, pos);


    const sampler_t sampler2= CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_LINEAR;

    float3 brightColor= (float3)(0);

#define RAD 20
/*
    for(int y=-RAD; y<=RAD; y+=2)
    for(int x=-RAD; x<=RAD; x+=2)
    {
        const float w= 1.0f - sqrt(POW2(x/(float)RAD)+POW2(y/(float)RAD));
        brightColor += max(w,0.0f) * read_image3f(srcBright0, sampler, pos+(int2)(x,y));
    }
    brightColor /= (RAD*2 + 1)*(RAD*2 + 1);
*/
    brightColor += read_image3f(srcBright0, sampler2, normPos);
/*    brightColor += read_image3f(srcBright1, sampler2, normPos);
    brightColor += read_image3f(srcBright2, sampler2, normPos);
    brightColor += read_image3f(srcBright3, sampler2, normPos);
    brightColor /= 4.0f;
*/
    // Blend visible and bright images
    float3 color= visibleColor+ brightBlend * brightColor;

#ifdef GAMMA_CORRECT
    // Gamma-correct color
    color= native_powr(color, 1.0f/GAMMA_CORRECT);
#endif

    // Clamp color
    color= clamp(color, (float3)(0), (float3)(1));

#ifdef LUMA_IN_ALPHA
    // Pre-compute the NON LINEAR luma value in the Alpha Channel (for FXAA)
    const float luma= dot(color, (float3)(0.299f, 0.587f, 0.114f));
#else
    const float luma= 1.0f;
#endif

    write_imagef(dst, pos, (float4)(color, luma));
}
