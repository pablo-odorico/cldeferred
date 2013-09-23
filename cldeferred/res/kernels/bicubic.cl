#ifndef BICUBIC_CL
#define BICUBIC_CL

//
// http://http.developer.nvidia.com/GPUGems2/gpugems2_chapter20.html
//

// w0, w1, w2, and w3 are the four cubic B-spline basis functions
float w0(float a) { return (1.0f/6.0f)*(a*(a*(-a + 3.0f) - 3.0f) + 1.0f); }
float w1(float a) { return (1.0f/6.0f)*(a*a*(3.0f*a - 6.0f) + 4.0f); }
float w2(float a) { return (1.0f/6.0f)*(a*(a*(-3.0f*a + 3.0f) + 3.0f) + 1.0f); }
float w3(float a) { return (1.0f/6.0f)*(a*a*a); }

// g0 and g1 are the two amplitude functions
float g0(float a) { return w0(a) + w1(a); }
float g1(float a) { return w2(a) + w3(a); }

// h0 and h1 are the two offset functions
float h0(float a) { return -1.0f + w1(a) / (w0(a) + w1(a)) + 0.5f; }
float h1(float a) { return  1.0f + w3(a) / (w2(a) + w3(a)) + 0.5f; }

// fast bicubic texture lookup using 4 bilinear lookups
float bicubicSample1(read_only image2d_t image, float2 pos)
{
    pos -= (float2)(0.5f, 0.5f);
    float2 p = floor(pos);
    float2 f = pos - p;

    float g0x = g0(f.x);
    float g1x = g1(f.x);
    float h0x = h0(f.x);
    float h1x = h1(f.x);
    float h0y = h0(f.y);
    float h1y = h1(f.y);

    const sampler_t sampler= CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_LINEAR;
    float f1= read_imagef(image, sampler, p + (float2)(h0x, h0y)).x;
    float f2= read_imagef(image, sampler, p + (float2)(h1x, h0y)).x;
    float f3= read_imagef(image, sampler, p + (float2)(h0x, h1y)).x;
    float f4= read_imagef(image, sampler, p + (float2)(h1x, h1y)).x;

    return g0(f.y) * (g0x * f1 + g1x * f2) + g1(f.y) * (g0x * f3 + g1x * f4);
}

// fast bicubic texture lookup using 4 bilinear lookups
float3 bicubicSample3(read_only image2d_t image, float2 pos)
{
    pos -= (float2)(0.5f, 0.5f);
    float2 p = floor(pos);
    float2 f = pos - p;

    float g0x = g0(f.x);
    float g1x = g1(f.x);
    float h0x = h0(f.x);
    float h1x = h1(f.x);
    float h0y = h0(f.y);
    float h1y = h1(f.y);

    const sampler_t sampler= CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_LINEAR;
    float3 f1= read_imagef(image, sampler, p + (float2)(h0x, h0y)).xyz;
    float3 f2= read_imagef(image, sampler, p + (float2)(h1x, h0y)).xyz;
    float3 f3= read_imagef(image, sampler, p + (float2)(h0x, h1y)).xyz;
    float3 f4= read_imagef(image, sampler, p + (float2)(h1x, h1y)).xyz;

    return g0(f.y) * (g0x * f1 + g1x * f2) + g1(f.y) * (g0x * f3 + g1x * f4);
}

// fast bicubic texture lookup using 4 bilinear lookups
float4 bicubicSample4(read_only image2d_t image, float2 pos)
{
    pos -= (float2)(0.5f, 0.5f);
    float2 p = floor(pos);
    float2 f = pos - p;

    float g0x = g0(f.x);
    float g1x = g1(f.x);
    float h0x = h0(f.x);
    float h1x = h1(f.x);
    float h0y = h0(f.y);
    float h1y = h1(f.y);

    const sampler_t sampler= CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_LINEAR;
    float4 f1= read_imagef(image, sampler, p + (float2)(h0x, h0y));
    float4 f2= read_imagef(image, sampler, p + (float2)(h1x, h0y));
    float4 f3= read_imagef(image, sampler, p + (float2)(h0x, h1y));
    float4 f4= read_imagef(image, sampler, p + (float2)(h1x, h1y));

    return g0(f.y) * (g0x * f1 + g1x * f2) + g1(f.y) * (g0x * f3 + g1x * f4);
}

#endif // BICUBIC_CL
