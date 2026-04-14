#version 330 core
in vec2 TexCoord;

uniform sampler2D uTexture;
uniform vec4 uColor;
uniform int uUseColor;

out vec4 FragColor;

void main() {
    if (uUseColor == 1) {
        FragColor = uColor;
    } else {
        FragColor = texture(uTexture, TexCoord);
    }
}