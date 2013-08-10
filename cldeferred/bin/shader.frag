#version 330 core

out vec4 color;
in vec4 outNormal;

uniform sampler2D qt_Texture1;

void main()
{
    //color = vec4(1, 0, 0, 1);
    //color = outNormal;
    color = texture(qt_Texture1, outNormal.xy);
}
