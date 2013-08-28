#version 330 core

uniform mat4 mvpMatrix;
uniform ivec2 fboSize;

// Output buffers
layout (location = 0) out float outDepth; // COLOR0: Depth

void main()
{
    // If this pixel is in the border, write 0
    const int x= gl_FragCoord.x;
    const int y= gl_FragCoord.y;
    const bool notBorder= min(min(x, y), min(w-1-x, h-1-y));

    outDepth = notBorder * gl_FragCoord.z;
}


