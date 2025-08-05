#version 330 core

in vec2 TexCoord;
out vec4 fragColor;

uniform sampler2D uTexture;  // The texture sampler

void main()
{
    // Sample from the texture using the transformed coordinates
    fragColor = texture(uTexture, TexCoord);
    
    // Optionally add any static layer effects here
    // For example, you could tint background layers:
    // fragColor.rgb *= vec3(0.8, 0.8, 0.9); // Slight blue tint
}