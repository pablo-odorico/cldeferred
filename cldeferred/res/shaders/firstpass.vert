#version 330 core

uniform mat4 mvpMatrix;
uniform mat4 modelITMatrix;
uniform int materialId; // Must be >=0 and <256

// Inputs in default Qt3D locations
layout (location = 0) in vec4 inPosition;
layout (location = 1) in vec4 inNormal;
layout (location = 3) in vec2 inTexCoord;

out vec2 texCoord;
out vec3 worldNormal;

void main()
{
    gl_Position = mvpMatrix * inPosition;

    texCoord = inTexCoord;

    worldNormal = (modelITMatrix * inNormal).xyz;
}
