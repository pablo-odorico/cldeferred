#version 330 core

uniform sampler2D inTexture;

in vec2 texCoord;
out vec4 outColor;

void main()
{
    outColor = texture(inTexture, texCoord);
}
