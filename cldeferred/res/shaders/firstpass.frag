#version 330 core

uniform mat4 mvpMatrix;
uniform mat4 modelITMatrix;
uniform int materialId; // Must be >=0 and <256

// Default Qt3D uniforms
uniform sampler2D qt_Texture0;

// Inputs from the vertex shader
in vec2 texCoord;
noperspective in vec3 worldNormal;

// Output buffers
layout (location = 0) out vec4 outDiffuseMat; // COLOR0: Diffuse texture sample + Material Id
layout (location = 1) out vec2 outNormal;     // COLOR1: Normalized normal in world coords
layout (location = 2) out float outDepth;     // COLOR2: Depth

void main()
{    
    vec3 diffuse = texture(qt_Texture0, texCoord).rgb;

    outDiffuseMat.rgb = diffuse;
    outDiffuseMat.a = materialId;

    outNormal = normalize(worldNormal).xy;

    outDepth = gl_FragCoord.z;
}


