#include "clutils.cl"

#include "cl_camera.h"
#include "cl_spotlight.h"
#include "cl_dirlight.h"
#include "cl_material.h"

// Image sampler
const sampler_t sampler= CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

//
// Deferred pass kernel
//

kernel
void deferredPass(
    read_only  image2d_t gbDiffuseMat,
    read_only  image2d_t gbNormals,
    read_only  image2d_t gbDepth,
    read_only global uchar* occlusionBuffer,
    write_only image2d_t output,
    // constant cl_material* materials,
    constant cl_camera* camera,
    int spotLightCount,
    constant cl_spotlight* spotLights,
    int dirLightCount,
    constant cl_dirlight* dirLights,
    int lightsWithShadows
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
    const float3 diffuse= diffuseMat.xyz;

    const uchar matId= clamp((int)(diffuseMat.w * 255.0f), 0, 255);
    cl_material mat;//= materials[matId];
    mat.diffuse= (float3)(1,1,1);
    mat.ambient= (float3)(0,0,0);
    mat.specular= (float3)(1,1,1);
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

        const float3 L= normalize(light.position - worldPos);
        const float angle= acos(max(dot(-light.lookVector, L), 0.0f));
        const float spotEffect= smoothstep(light.cutOffMax, light.cutOffMin, angle);

        const float3 NdotL= max(dot(normal, L), 0.0f);

        float3 V= camera->position - worldPos;
        const float dist= length(V);
        V /= dist;
        const float spec= max(dot(reflect(-L, normal), V), 0.0f);

        const float3 lAmbient= light.ambient * mat.ambient;
        const float3 lDiffuse= light.diffuse * mat.diffuse * diffuse * NdotL;
        const float3 lSpecular= light.specular * mat.specular * pow(spec, mat.shininess);

        color += lAmbient;
        color += (lSpecular + lDiffuse) * spotEffect * visibility; // TODO atenuation

    }

    // Write output
    color= clamp(color, (float3)(0,0,0), (float3)(1,1,1));

    // FXAA:
    // Pre-compute and store the Luma value in the Alpha Channel
    const float fxaaLuma= dot(color, (float3)(0.299f, 0.587f, 0.114f));

    write_imagef(output, pos, (float4)(color, fxaaLuma));
}
