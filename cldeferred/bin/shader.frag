#version 330 core

// Default Qt3D uniforms
uniform sampler2D qt_Texture1;

// Inputs from the vertex shader
in vec2 texCoord;

// Output buffers
// COLOR0: Diffuse texture sample + Specular power
layout (location = 0) out vec4 diffuseSpec;

void main()
{
    vec3 diffuse = texture(qt_Texture1, texCoord).rgb;
    float spec = 0.0f;

    diffuseSpec = vec4(diffuse, spec);
}
