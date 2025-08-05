#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform vec2 uOffset;  // Offset in normalized coordinates
uniform vec2 uSize;   // Scale factors for the layer

void main()
{
    // Transform the position - keep the fullscreen quad as is
    gl_Position = vec4(aPos, 0.0, 1.0);
    // Adjust texture coordinates based on offset and size
    TexCoord = (aTexCoord + uOffset) / uSize;
}