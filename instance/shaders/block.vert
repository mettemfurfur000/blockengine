#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec4 aInstance; // x, y, frame, type

uniform mat4 uProjection;
uniform vec2 uFrameCount;   // x: frames, y: types
uniform float uBlockWidth;

out vec2 TexCoord;

void main()
{
    // Calculate position based on instance data
    vec2 position = aPos * uBlockWidth + vec2(aInstance.y, aInstance.x);
    gl_Position = uProjection * vec4(position, 0.0, 1.0);
    
    // Calculate texture coordinates based on frame and type
    float frame = aInstance.z;
    float type = aInstance.w;
    
    float frameX = (frame / uFrameCount.x);
    float frameY = type / uFrameCount.y;
    
    // Adjust texture coordinates
    TexCoord = aTexCoord;
    TexCoord.x = frameX + (aTexCoord.x / uFrameCount.x);
    TexCoord.y = frameY + (aTexCoord.y / uFrameCount.y);
}