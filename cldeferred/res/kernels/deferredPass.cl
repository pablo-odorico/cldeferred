#include "clutils.cl"

#include "cl_camera.h"
#include "cl_spotlight.h"
#include "cl_dirlight.h"
#include "cl_material.h"

#define GAMMA 2.2f

// Image sampler
const sampler_t sampler= CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

float3 toneMap(float3 inCol, float exposure, float maxLight)
{
    float3 outCol;

    //outCol = inCol / (inCol + 1.0f);
    //outCol=inCol;

    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    inCol *= exposure;
    outCol= ((inCol*(A*inCol+C*B)+D*E)/(inCol*(A*inCol+B)+D*F))-E/F;

    outCol /= ((maxLight*(A*maxLight+C*B)+D*E)/(maxLight*(A*maxLight+B)+D*F))-E/F;

    return clamp(outCol, (float3)(0), (float3)(1));
}

//
// Deferred pass kernel
//

kernel
void deferredPass(
    // G-Buffer and output buffer
    read_only  image2d_t gbDiffuseMat,
    read_only  image2d_t gbNormals,
    read_only  image2d_t gbDepth,
    read_only global uchar* occlusionBuffer,
    write_only image2d_t output,
    // Scene: Camera, Lights and Materials
    // constant cl_material* materials,
    constant cl_camera* camera,
    int spotLightCount,
    constant cl_spotlight* spotLights,
    int dirLightCount,
    constant cl_dirlight* dirLights,
    int lightsWithShadows,
    // HDR
    float exposure,
    float maxLight
)
{
    // Get global position
    const int2 pos= (int2)(get_global_id(0), get_global_id(1));
    // Size of the G-Buffer, must be <= the size of the output
    const int2 size= get_image_dim(gbDiffuseMat);

    if(pos.x >= size.x || pos.y >= size.y)
        return;

    // Load data from G-Buffer
    const float4 diffuseMat= read_imagef(gbDiffuseMat, sampler, pos);
    const float3 diffuse= native_powr(diffuseMat.xyz, GAMMA);

    const uchar matId= clamp((int)(diffuseMat.w * 255.0f), 0, 255);
    cl_material mat;//= materials[matId];
    mat.diffuse= (float3)(1);
    mat.ambient= (float3)(1);
    mat.specular= (float3)(10);
    mat.shininess= 200;

    float3 normal= read_imagef(gbNormals, sampler, pos).xyz;
    normal.z= sqrt(1.0f - POW2(normal.x) - POW2(normal.y));
    //normal= (float3)(0,1,0);

    float depth= read_image1f(gbDepth, sampler, pos);
    float4 clipPos= getClipPosFromDepth(pos, size, depth, camera->projMatrix);
    float3 worldPos= multMatVec(camera->vpMatrixInv, clipPos).xyz;

    global uchar* occlusionPtr= occlusionBuffer + (pos.x + pos.y * size.x) * lightsWithShadows;

    float3 color= (float3)(0,0,0);

    for(int i=0; i<spotLightCount; i++) {
        cl_spotlight light= spotLights[i];

        float visibility= 1.0f;
        if(light.hasShadows) {
            visibility = 1.0f - unpackOcclusion(*occlusionPtr);
            occlusionPtr++;
        }

        float3 L= light.position - worldPos;
        const float dist= fast_length(L);
        L /= dist;

        const float angle= acos(max(dot(-light.lookVector, L), 0.0f));
        const float spotEffect= smoothstep(light.cutOffMax, light.cutOffMin, angle);

        const float3 NdotL= max(dot(normal, L), 0.0f);

        float3 V= fast_normalize(camera->position - worldPos);
        const float spec= max(dot(reflect(-L, normal), V), 0.0f);

        const float3 lAmbient= light.ambient * mat.ambient;
        const float3 lDiffuse= light.diffuse * mat.diffuse * NdotL;
        const float3 lSpecular= light.specular * mat.specular * native_powr(spec, mat.shininess);

        const float attenuation= 1.0f / (light.attenuation * POW2(dist));

        float3 lightColor= lAmbient;
        lightColor += (lSpecular + lDiffuse) * spotEffect * visibility * attenuation;
        color += lightColor * diffuse;

    }

    for(int i=0; i<dirLightCount; i++) {
        cl_dirlight light= dirLights[i];

        float visibility= 1.0f;
        if(light.hasShadows) {
            visibility = 1.0f - unpackOcclusion(*occlusionPtr);
            occlusionPtr++;
        }

        float3 L= -light.lookVector;
        const float3 NdotL= max(dot(normal, L), 0.0f);

        float3 V= fast_normalize(camera->position - worldPos);
        const float spec= max(dot(reflect(-L, normal), V), 0.0f);

        const float3 lAmbient= light.ambient * mat.ambient;
        const float3 lDiffuse= light.diffuse * mat.diffuse * NdotL;
        const float3 lSpecular= light.specular * mat.specular * native_powr(spec, mat.shininess);

        const float3 lightColor= lAmbient + (lSpecular + lDiffuse) * visibility;
        color += lightColor * diffuse;
    }

    // Tone mapping for HDR
    color= toneMap(color, exposure, maxLight);

    // Gamma-correct color
    color= native_powr(color, 1.0f/GAMMA);

    // Pre-compute the NON LINEAR luma value in the Alpha Channel (for FXAA)
    const float fxaaLuma= dot(color, (float3)(0.299f, 0.587f, 0.114f));

    write_imagef(output, pos, (float4)(color, fxaaLuma));

}


