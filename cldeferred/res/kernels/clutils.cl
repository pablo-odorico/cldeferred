#ifndef CLUTILS_CL
#define CLUTILS_CL

inline
float4 multMatVec(const float16 m, const float4 v)
{
    return (float4) (
        dot(m.s0123, v),
        dot(m.s4567, v),
        dot(m.s89AB, v),
        dot(m.sCDEF, v)
    );
}

// Get clip coord from 0..1 depth
// http://www.opengl.org/wiki/Compute_eye_space_from_window_space#From_XYZ_of_gl_FragCoord
float4 getClipPosFromDepth(
    const int2 screenPos, const int2 screenSize,
    const float depth,
    const float16 projMatrix)
{
    const float3 ndcPos= 2.0f * (float3)(convert_float2(screenPos)/screenSize, depth) - 1.0f;

    const float pm23= projMatrix.sE; // Mat. indices  0 1 2 3
    const float pm22= projMatrix.sA; //               4 5 6 7
    const float pm32= projMatrix.sB; //               8 9 A B
                                     //               C D E F
    const float clipW= pm23 / (ndcPos.z - pm22/pm32);
    const float4 clipPos= (float4)(ndcPos * clipW, clipW);

    return clipPos;
}


#endif // CLUTILS_CL
