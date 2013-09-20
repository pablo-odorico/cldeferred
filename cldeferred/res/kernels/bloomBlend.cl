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
    read_only  image2d_t srcBright,
    // Gamma-corrected blended output (can be 8 bits)
    write_only image2d_t dst,
    float brightBlend
) {
    const int2 pos= (int2)(get_global_id(0), get_global_id(1));
    const int2 size= get_image_dim(dst);

    if(pos.x >= size.x || pos.y >= size.y)
        return;

    const sampler_t sampler= CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

    const float3 visibleColor= read_image3f(srcVisible, sampler, pos);
    const float3 brightColor = read_image3f(srcBright , sampler, pos);

    // Blend visible and bright images
    float3 color= visibleColor + brightBlend * brightColor;

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
