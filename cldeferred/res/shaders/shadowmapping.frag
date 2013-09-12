#version 330 core

// Output buffers
layout (location = 0) out vec2 outDepth; // COLOR0: Depth

void main()
{
    float depth= gl_FragCoord.z;

    float dx = dFdx(depth);
    float dy = dFdy(depth);

    outDepth = vec2(depth, depth*depth + 0.25f * (dx*dx + dy*dy));
}


