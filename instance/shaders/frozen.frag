#version 330 core

in vec2 TexCoord;
out vec4 fragColor;

uniform sampler2D uTexture;  // The texture sampler

void main()
{
    vec4 color;

    if(gl_PrimitiveID % 2 == 0) 
        color = vec4(1.0, 0.0, 0.0, 0.32); 
    else
        color = vec4(0.0, 0.6, 1.0, 0.32);

    // Sample from the texture using the transformed coordinates
    fragColor = texture(uTexture, TexCoord);
    
    fragColor.rgba *= color; // Apply the color modulation
}