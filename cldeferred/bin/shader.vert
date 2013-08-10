#version 330 core

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;

layout (location = 0) in vec4 position;
layout (location = 3) in vec4 normal;

out vec4 outNormal;

void main()
{
    gl_Position = projMatrix * viewMatrix * modelMatrix * position;
    outNormal= normal;
}

