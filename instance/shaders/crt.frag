#version 330 core
in vec2 TexCoords;

out vec4 FragColor;

uniform sampler2D screenTexture;

const int scanlineCount = 128;

void main() {
    float centerOffset = -0.5;

    vec2 uvCentered = TexCoords + centerOffset;

    vec2 uvNew = uvCentered * (1.0 + 0.1 * vec2(4.0 / 1.0, 1.0) * length(uvCentered)) - centerOffset; //bar - centerOffset;

    float brightness = (int(uvNew.y * scanlineCount) % 2) == 0 ? 1.0 : 0.75;

    if(uvNew.x >= 0.0 && uvNew.x <= 1.0 && uvNew.y >= 0.0 && uvNew.y <= 1.0)
        FragColor = vec4(vec3(brightness * texture(screenTexture, uvNew).xyz), 1);
    else
        FragColor = vec4(0.0);
}