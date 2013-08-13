
__kernel void outputKernel(
    int width, int height,
    int frameId,
    __write_only image2d_t output)
{
    // Get global position
    int x= get_global_id(0);
    int y= get_global_id(1);

    if(x>=width || y>=height)
        return;

    float4 color= (float4)(((x+frameId) % 256) / 256.0f, ((y+frameId) % 256) / 256.0f, 0, 1);

    write_imagef(output, (int2)(x,y), color);
}
