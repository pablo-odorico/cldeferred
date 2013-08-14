#version 330 core

// Default Qt3D uniforms
uniform sampler2D qt_Texture1;

// Inputs from the vertex shader
in vec2 texCoord;
in vec3 normal;

// Output buffers
// COLOR0: Diffuse texture sample + Specular power
layout (location = 0) out vec4 outDiffuseSpec;
// COLOR1: Normals in world coords
layout (location = 1) out vec2 outNormal;
// DEPTH: Filled by OpenGL

void main()
{
    vec3 diffuse = texture(qt_Texture1, texCoord).rgb;
    float spec = 1.0f;

    outDiffuseSpec = vec4(diffuse, spec);
    outNormal = normal.xy;
}
