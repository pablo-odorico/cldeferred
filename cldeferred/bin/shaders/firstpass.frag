#version 330 core

uniform mat4 modelMatrix;
uniform mat4 modelITMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;
// Precomputed projMatrix * viewMatrix * modelMatrix matrix
uniform mat4 mvpMatrix;


// Default Qt3D uniforms
uniform sampler2D qt_Texture1;

// Inputs from the vertex shader
in vec2 texCoord;
in vec3 normal;
in float depth;

// Output buffers
layout (location = 0) out vec4 outDiffuseSpec; // COLOR0: Diffuse texture sample + Specular power
layout (location = 1) out vec2 outNormal;      // COLOR1: Normals in world coords
layout (location = 2) out float outDepth;      // COLOR2: Depth

void main()
{
    vec3 diffuse = texture(qt_Texture1, texCoord).rgb;
    float spec = 1.0f;

    outDiffuseSpec = vec4(diffuse, spec);
    outNormal = normal.xy;

    outDepth = gl_FragCoord.z;

    vec4 ndcPos;
    ndcPos.xy = (2.0 * gl_FragCoord.xy) / vec2(854, 480) - 1;
    ndcPos.z = 2.0 * gl_FragCoord.z - 1;
    ndcPos.w = 1.0;

    //vec4 clipPos = ndcPos / gl_FragCoord.w;

    vec4 clipPos;
    clipPos.w = projMatrix[3][2] / (ndcPos.z - (projMatrix[2][2] / projMatrix[2][3]));
    clipPos.xyz = ndcPos.xyz * clipPos.w;

    vec4 eyePos = inverse(projMatrix * viewMatrix) * clipPos;

    outDiffuseSpec = vec4(eyePos.xyz/8, 1.0f);
}


