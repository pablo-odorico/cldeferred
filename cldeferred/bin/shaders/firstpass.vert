#version 330 core

uniform mat4 modelMatrix;
uniform mat4 modelITMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;
// Precomputed projMatrix * viewMatrix * modelMatrix matrix
uniform mat4 mvpMatrix;

// Inputs in default Qt3D locations
layout (location = 0) in vec4 inPosition;
layout (location = 1) in vec4 inNormal;
layout (location = 3) in vec2 inTexCoord;

out vec2 texCoord;
out vec3 normal;

void main()
{
    gl_Position = mvpMatrix * inPosition;
    texCoord = inTexCoord;
    normal = normalize((modelITMatrix * inNormal).xyz);
}
