
__kernel void outputKernel(
    int width, int height,
    __read_only image2d_t gbDiffuseSpec,
    __read_only image2d_t gbNormals,
    __read_only image2d_t gbDepth,
    __write_only image2d_t output)
{
    // Get global position
    int x= get_global_id(0);
    int y= get_global_id(1);

    if(x>=width || y>=height)
        return;

    const sampler_t sampler= CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

    float4 diffuseSpec= read_imagef(gbDiffuseSpec, sampler, (int2)(x,y));

    float3 normal= read_imagef(gbNormals, sampler, (int2)(x,y)).xyz;
    normal.z= sqrt(1.0f - normal.x*normal.x - normal.y*normal.y);

    float depth= read_imagef(gbDepth, sampler, (int2)(x,y)).x;

    //normal *= (depth==0.0f) ? 0.0f : 1.0f;

    float4 color= (float4)(normal, 1.0f);

    write_imagef(output, (int2)(x,y), color);
}
