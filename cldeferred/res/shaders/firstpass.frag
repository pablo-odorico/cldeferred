#version 330 core

uniform mat4 mvpMatrix;
uniform mat4 modelITMatrix;

// Default Qt3D uniforms
uniform sampler2D qt_Texture1;

// Inputs from the vertex shader
in vec2 texCoord;
in vec3 normal;

// Output buffers
layout (location = 0) out vec4 outDiffuseSpec; // COLOR0: Diffuse texture sample + Specular power
layout (location = 1) out vec2 outNormal;      // COLOR1: Normals in world coords
layout (location = 2) out float outDepth;      // COLOR2: Depth

void main()
{
    vec3 diffuse = texture(qt_Texture1, texCoord).rgb;
    float spec = 1.0f;

    outDiffuseSpec.rgb= diffuse;
    outDiffuseSpec.a= spec;
    outNormal = normal.xy;
    outDepth = gl_FragCoord.z;
}


