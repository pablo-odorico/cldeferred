#version 330 core

uniform mat4 mvpMatrix;

layout (location = 0) in vec4 inPosition;

void main()
{    
    gl_Position = mvpMatrix * inPosition;
}
