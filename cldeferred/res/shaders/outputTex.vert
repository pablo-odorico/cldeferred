#version 330 core

in vec4 inPosition;

out vec2 texCoord;

void main()
{
    gl_Position = vec4(inPosition.xy, 0, 1);
    texCoord = inPosition.zw;
}

