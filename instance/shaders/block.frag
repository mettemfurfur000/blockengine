#version 330 core
in vec2 TexCoord;

uniform sampler2D uTexture;

out vec4 FragColor;

void main() {
    vec4 color;

    // small adition to see actual triangles for debug only
    // if(gl_PrimitiveID % 2 == 0) 
    //     color = vec4(1.0, 0.0, 0.0, 0.32); 
    // else
    //     color = vec4(0.0, 0.6, 1.0, 0.32);

    // FragColor = (texture(uTexture, TexCoord) + color) / 2;
    FragColor = texture(uTexture, TexCoord);
}