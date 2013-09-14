#version 330 core

in vec2 inPosition;

out vec2 texCoord;

void main()
{
    gl_Position = vec4(inPosition, 0, 1);
    texCoord = inPosition * 0.5f + 0.5f;
}

