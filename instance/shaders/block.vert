#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;
// Instance attributes
layout(location = 2) in vec2 aInstancePos;      // x, y
layout(location = 3) in vec2 aInstanceScale;    // scaleX, scaleY
layout(location = 4) in float aInstanceRotation; // rotation
layout(location = 5) in uint aInstanceFrame;     // frame number
layout(location = 6) in uint aInstanceType;      // block type
layout(location = 7) in uint aInstanceFlags;     // flags

uniform mat4 uProjection;
uniform vec2 uResizeRatio;
uniform float uBlockWidth;

out vec2 TexCoord;

void main() {
    // Apply scaling
    vec2 scaledPos = aPos * aInstanceScale;
    
    // Apply rotation
    float cosR = cos(aInstanceRotation);
    float sinR = sin(aInstanceRotation);
    vec2 rotatedPos = vec2(
        scaledPos.x * cosR - scaledPos.y * sinR,
        scaledPos.x * sinR + scaledPos.y * cosR
    );

    // Apply position and block width
    vec2 finalPos = rotatedPos * uBlockWidth + aInstancePos;
    
    // Set final position
    gl_Position = uProjection * vec4(finalPos, 0.0, 1.0);
    
    // Start with the input texture coordinates
    vec2 finalTexCoord = aTexCoord;
    
    // Handle horizontal flip if bit 0 of flags is set
    if ((aInstanceFlags & uint(1)) != uint(0)) {
        finalTexCoord.x = 1.0 - finalTexCoord.x;
    }
    
    // Handle vertical flip if bit 1 of flags is set
    if ((aInstanceFlags & uint(2)) != uint(0)) {
        finalTexCoord.y = 1.0 - finalTexCoord.y;
    }
    
    // Apply frame and type offsets
    vec2 texCoordOffset = vec2(float(aInstanceFrame), float(aInstanceType));
    TexCoord = (finalTexCoord + texCoordOffset) / uResizeRatio;
}