//#include "clutils.cl"

#include "cl_camera.h"
#include "cl_spotlight.h"


const sampler_t normSampler= CLK_NORMALIZED_COORDS_TRUE | \
    CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_LINEAR;
const sampler_t sampler= CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_LINEAR;


#define DEPTH_PARAM_NAME(prefix,N) prefix##N##depth
#define DEF_DEPTH_PARAM(prefix,N)  read_only image2d_t DEPTH_PARAM_NAME(prefix,N)


uint packColor(const float4 color)
{
    return \
        clamp((int)(color.s3 * 255.0f), 0, 255) << 24 |
        clamp((int)(color.s0 * 255.0f), 0, 255) << 16 |
        clamp((int)(color.s1 * 255.0f), 0, 255) << 8  |
        clamp((int)(color.s2 * 255.0f), 0, 255) << 0;
}

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
    float3 ndcPos;
    ndcPos.x= (2.0f * screenPos.x) / screenSize.x - 1.0f;
    ndcPos.y= (2.0f * screenPos.y) / screenSize.y - 1.0f;
    ndcPos.z= (2.0f * depth) - 1.0f;

    const float pm23= projMatrix.sB; // Mat. indices  0 1 2 3
    const float pm22= projMatrix.sA; //               4 5 6 7
    const float pm32= projMatrix.sE; //               8 9 A B
                                     //               C D E F

    const float clipW= pm23 / (ndcPos.z - pm22/pm32);
    const float4 clipPos= (float4)(ndcPos * clipW, clipW);

    return clipPos * clipW;
}


__kernel
void occlusionPass(
    constant cl_camera* camera,
    read_only image2d_t cameraDepth,
    constant cl_spotlight* spotLights,
//    DEF_DEPTH_PARAM(spotLight, 0),
    read_only image2d_t spotLight0depth,
    write_only global uint* occlusionBuffer
)
{
    const int2 pos= (int2)(get_global_id(0), get_global_id(1));
    const int2 size= get_image_dim(cameraDepth);

    if(pos.x >= size.x || pos.y >= size.y)
        return;

    const float camDepth= read_imagef(cameraDepth, sampler, pos).x;
    float4 clipPos= getClipPosFromDepth(pos, size, camDepth, camera->projMatrix);
    const float4 worldPos= multMatVec(camera->vpMatrixInv, clipPos);

    //uint occlusion= 0;
    //float4 lightClipPos;
    //float lightDepth;

// lights is the pointer to the light structs (eg. spotLights)
// N: light number (resets for each type)
// depthPrefix: used to create depthPrefix##Ndepth
/*
#define SET_OCCLUSION(lights,N,depthPrefix) \
    lightClipPos= multMatVec(lights[N].viewProjMatrix, worldPos)/2.0f + 0.5f; \
    lightDepth= read_imagef(depthPrefix##N##depth, sampler, lightClipPos.xy).x; \
    occlusion |= (lightDepth < lightClipPos.z) << N;
*/
    // SET_OCCLUSION(spotLights, 0, spotLight)


    //lightClipPos= multMatVec(spotLights[0].viewProjMatrix, worldPos)/2.0f + 0.5f;
    //lightDepth= read_imagef(spotLight0depth, normSampler, lightClipPos.xy).x;
    //bool occ= lightDepth < lightClipPos.z;
/*
    float3 ndcPos;
    ndcPos.x= (2.0f * pos.x) / size.x - 1.0f;
    ndcPos.y= (2.0f * pos.y) / size.y - 1.0f;
    ndcPos.z= (2.0f * camDepth) - 1.0f;

    const float pm23= -20;
    const float pm22= -3;
    const float pm32= -1;

    const float clipW= pm23 / (ndcPos.z - pm22/pm32);
    const float4 clipPos= (float4)(ndcPos * clipW, clipW);
*/
    //float c= camDepth;
    //occlusionBuffer[pos.x + pos.y * size.x]= packColor((float4)(c,c,c, 1.0f));

    //occlusionBuffer[pos.x + pos.y * size.x]= packColor((float4)(worldPos.xyz/2.0f+0.5f, 1.0f));
    occlusionBuffer[pos.x + pos.y * size.x]= packColor((float4)(worldPos.xyz, 1.0f));

    //occlusionBuffer[pos.x + pos.y * size.x]= packColor((float4)(camera->projMatrix.s012, 1.0f));
}
