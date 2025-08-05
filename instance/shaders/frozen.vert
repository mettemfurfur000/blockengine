#version 330 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform vec2 uOffset; // offset in pixels
uniform vec2 uSize;   // size in pixels

void main() {
    // Transform the position - keep the fullscreen quad as is
    gl_Position = vec4(aPos, 0.0, 1.0);
    // Transform texture coordinates to sample the correct portion of the texture
    TexCoord = uOffset + aTexCoord / uSize;
}