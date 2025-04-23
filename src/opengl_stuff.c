#include "../include/opengl_stuff.h"
#include <stdlib.h>

void setup_opengl(u16 width, u16 height)
{
    // Setup OpenGL for 2D rendering
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0f, width, height, 0.0f, -1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();
}

u32 load_shader(char *shader_path, GLenum shader_type)
{
    // Load and compile the shader
    FILE *shader_file = fopen(shader_path, "r");
    if (!shader_file)
    {
        LOG_ERROR("Failed to open shader file: %s", shader_path);
        return 0;
    }

    fseek(shader_file, 0, SEEK_END);
    long shader_size = ftell(shader_file);
    fseek(shader_file, 0, SEEK_SET);

    char *shader_source = (char *)malloc(shader_size + 1);
    fread(shader_source, 1, shader_size, shader_file);
    shader_source[shader_size] = '\0';
    fclose(shader_file);

    unsigned int ShaderID = glCreateShader(shader_type);

    glShaderSource(ShaderID, 1, (const GLchar * const*)shader_source, NULL);
    glCompileShader(ShaderID);

    return ShaderID;
}