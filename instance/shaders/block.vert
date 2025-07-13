#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec4 aInstance; // x, y, frame, type

uniform mat4 uProjection;
uniform vec2 uResizeRatio;
uniform float uBlockWidth;

out vec2 TexCoord;

void main() {
    vec2 position = aPos * uBlockWidth + vec2(aInstance.x, aInstance.y);
    gl_Position = uProjection * vec4(position, 0.0, 1.0);
    TexCoord = (aTexCoord + aInstance.zw) / uResizeRatio;
}