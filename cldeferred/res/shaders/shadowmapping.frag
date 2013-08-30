#version 330 core

uniform mat4 mvpMatrix;
uniform vec2 fboSize;

// Output buffers
layout (location = 0) out float outDepth; // COLOR0: Depth

void main()
{
    // If this pixel is in the border, write 0
    int x= int(gl_FragCoord.x);
    int y= int(gl_FragCoord.y);
    bool notBorder= bool(min(min(x, y), min(fboSize.x-1-x, fboSize.y-1-y)));

    outDepth = float(notBorder) * gl_FragCoord.z;
}


