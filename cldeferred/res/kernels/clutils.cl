#ifndef CLUTILS_CL
#define CLUTILS_CL

#define read_image1f(image,sampler,pos)     read_imagef((image),(sampler),(pos)).x
#define read_image2f(image,sampler,pos)     read_imagef((image),(sampler),(pos)).xy
#define read_image3f(image,sampler,pos)     read_imagef((image),(sampler),(pos)).xyz

#define POW2(x) ((x)*(x))

//
// Float to uchar occlusion packing/unpacking
//

uchar packOcclusion(const float value)
{
    return clamp((int)(value * 255.0f), 0, 255);
}

float unpackOcclusion(const uchar value)
{
    return clamp(value / 255.0f, 0.0f, 1.0f);
}


//
// Float4 to uint32 color packing/unpacking
//

float4 unpackColor(const uint color)
{
    return (float4)(
        clamp(((color >> 16) & 0xFF) / 255.0f, 0.0f, 1.0f),
        clamp(((color >> 8 ) & 0xFF) / 255.0f, 0.0f, 1.0f),
        clamp(((color >> 0 ) & 0xFF) / 255.0f, 0.0f, 1.0f),
        clamp(((color >> 24) & 0xFF) / 255.0f, 0.0f, 1.0f));

}

uint packColor(const float4 color)
{
    return \
        clamp((int)(color.s3 * 255.0f), 0, 255) << 24 |
        clamp((int)(color.s0 * 255.0f), 0, 255) << 16 |
        clamp((int)(color.s1 * 255.0f), 0, 255) << 8  |
        clamp((int)(color.s2 * 255.0f), 0, 255) << 0;
}

//
// Matrix operations
//

float4 multMatVec(const float16 m, const float4 v)
{
    return (float4) (
        dot(m.s0123, v),
        dot(m.s4567, v),
        dot(m.s89AB, v),
        dot(m.sCDEF, v)
    );
}

//
// 3D operations
//

// Get clip coord from 0..1 depth
// http://www.opengl.org/wiki/Compute_eye_space_from_window_space#From_XYZ_of_gl_FragCoord
float4 getClipPosFromDepth(
    const int2 screenPos, const int2 screenSize,
    const float depth,
    const float16 projMatrix)
{
    float3 ndcPos= (float3)(
        (float)screenPos.x/screenSize.x,
        (float)screenPos.y/screenSize.y,
        depth) * 2.0f - 1.0f;

    const float pm32= projMatrix.sB; // Mat. indices  0 1 2 3
    const float pm22= projMatrix.sA; //               4 5 6 7
    const float pm23= projMatrix.sE; //               8 9 A B
                                     //               C D E F

    const float clipW= pm32 / (ndcPos.z - pm22/pm23);
    const float4 clipPos= (float4)(ndcPos * clipW, clipW);

    return clipPos;
}

#endif // CLUTILS_CL
