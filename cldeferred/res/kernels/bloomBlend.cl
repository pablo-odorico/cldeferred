//
// Bloom blending of the "visible" and filtered "bright" images.
// Both input image should be floating point and unnormalized. Output image can
// be 8-bit normalized.
//
// Configuration defines:
// - GAMMA_CORRECT: If set, the final tone-mapped color is gamma corrected to
//   the define's value.
// - LUMA_IN_ALPHA: If set, the luma of the final color is calculated and stored
//   on the alpha channel of the image.

#include "clutils.cl"
#include "bicubic.cl"

kernel void bloomBlend(
    // Linear input image
    // "visible" values are min(input, 1)
    read_only  image2d_t input,
    // Filtered and downsampled bloom image
    // "bright" values are max(bloom-1, 0)
    read_only  image2d_t bloom,
    // Gamma-corrected blended output (can be normalized 8 bits)
    write_only image2d_t output,
    float bloomBlend
) {
    const int2 pos= (int2)(get_global_id(0), get_global_id(1));
    const int2 size= get_image_dim(output);

    if(pos.x >= size.x || pos.y >= size.y)
        return;

    const sampler_t sampler= CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_LINEAR;

    // Read input color and extract the "visible" component
    float3 visible= read_image3f(input, sampler, pos);
    visible= min(visible, (float3)(1));

    // Read bloom color and extract the "bright" component
    const float2 normPos= normalizePos(pos, get_image_dim(input));
    const int2 bloomSize= get_image_dim(bloom);
    const float2 bloomPos= (float2)(normPos.x*bloomSize.x + 0.5f, normPos.y*bloomSize.y + 0.5f);
    float3 bright= bicubicSample3(bloom, bloomPos);
    bright= max(bright-1, (float3)(0));

    // Blend visible and bright images
    float3 color= visible + bloomBlend * bright;

/*
    // Read bloom color and extract the "bright" component
    const float2 normPos= normalizePos(pos, get_image_dim(input));
    const int2 bloomSize= get_image_dim(bloom);
    const float2 bloomPos= (float2)(normPos.x*bloomSize.x + 0.5f, normPos.y*bloomSize.y + 0.5f);
    float3 bright;
    if(pos.x > size.x/2)
        bright= read_image3f(bloom, sampler, bloomPos);
    else
        bright= bicubicSample3(bloom, bloomPos);
    bright= max(bright-(float3)(1), (float3)(0));

    // Blend visible and bright images
    float3 color= bloomBlend * bright;
*/

#ifdef GAMMA_CORRECT
    // Gamma-correct output color
    color= native_powr(color, 1.0f/GAMMA_CORRECT);
#endif

    // Clamp output color
    color= clamp(color, (float3)(0), (float3)(1));

#ifdef LUMA_IN_ALPHA
    // Pre-compute the NON LINEAR luma value in the Alpha Channel (for FXAA)
    const float luma= dot(color, (float3)(0.299f, 0.587f, 0.114f));
#else
    const float luma= 1.0f;
#endif

    write_imagef(output, pos, (float4)(color, luma));
}


kernel void bloomBypass(
    // Linear input image
    // "visible" values are min(input, 1)
    read_only  image2d_t input,
    // Gamma-corrected blended output (can be normalized 8 bits)
    write_only image2d_t output
) {
    const int2 pos= (int2)(get_global_id(0), get_global_id(1));
    const int2 size= get_image_dim(output);

    if(pos.x >= size.x || pos.y >= size.y)
        return;

    const sampler_t sampler= CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_LINEAR;

    // Read input color and extract the "visible" component
    float3 color= read_image3f(input, sampler, pos);
    color= min(color, (float3)(1));

#ifdef GAMMA_CORRECT
    // Gamma-correct output color
    color= native_powr(color, 1.0f/GAMMA_CORRECT);
#endif

    // Clamp output color
    color= clamp(color, (float3)(0), (float3)(1));

#ifdef LUMA_IN_ALPHA
    // Pre-compute the NON LINEAR luma value in the Alpha Channel (for FXAA)
    const float luma= dot(color, (float3)(0.299f, 0.587f, 0.114f));
#else
    const float luma= 1.0f;
#endif

    write_imagef(output, pos, (float4)(color, luma));
}
