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

#endif // CLUTILS_CL
