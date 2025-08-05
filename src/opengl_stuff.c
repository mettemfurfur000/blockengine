#include "../include/opengl_stuff.h"
#include "../include/block_renderer.h"
// #include "SDL_opengl.h"
#include <epoxy/gl_generated.h>
#include <stdlib.h>

void setup_opengl(u16 width, u16 height)
{
    // Setup OpenGL for 2D rendering
    // glViewport(0, 0, width, height);
    // glMatrixMode(GL_PROJECTION);
    // glLoadIdentity();
    // glOrtho(0.0f, width, height, 0.0f, -1.0f, 1.0f);
    // glMatrixMode(GL_MODELVIEW);
    // glLoadIdentity();
    // glClear(GL_COLOR_BUFFER_BIT);
    // glLoadIdentity();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    block_renderer_init(SCREEN_WIDTH, SCREEN_HEIGHT);
}

u32 load_shader(const char *shader_path, GLenum shader_type)
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

    char *shader_source = (char *)calloc(shader_size + 1, 1);
    fread(shader_source, 1, shader_size, shader_file);
    shader_source[shader_size] = '\0';
    fclose(shader_file);

    // replace \r's with \n's
    for (u32 i = 0; i < shader_size; i++)
        if (shader_source[i] == '\r')
            shader_source[i] = '\n';

    u32 ShaderID = glCreateShader(shader_type);

    glShaderSource(ShaderID, 1, (const char *const *)&shader_source, NULL);
    glCompileShader(ShaderID);

    int success;
    char infoLog[512];
    glGetShaderiv(ShaderID, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        glGetShaderInfoLog(ShaderID, 512, NULL, infoLog);
        LOG_ERROR("Failed to load %s shader : %s", shader_path, infoLog);
        LOG_ERROR("Shader source: %s", shader_source);
        free(shader_source);
        return 0;
    }

    free(shader_source);

    return ShaderID;
}

u32 compile_shader_program(u32 *shaders, u8 len)
{
    if (len > 3)
    {
        LOG_ERROR("compile_shader_program: too much boy!");
        return 0;
    }

    u32 shader;
    shader = glCreateProgram();
    for (u8 i = 0; i < len; i++)
        glAttachShader(shader, shaders[i]);

    glLinkProgram(shader);

    for (u8 i = 0; i < len; i++)
        glDeleteShader(shaders[i]);

    int success;
    char infoLog[512];
    glGetProgramiv(shader, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shader, 512, NULL, infoLog);
        LOG_ERROR("TS PMO : %s", infoLog);
        return 0;
    }

    return shader;
}

u32 assemble_shader(const char *shader_name)
{
    u32 shaders[3] = {};
    u8 len = 3;
    char path_buf[MAX_PATH_LENGTH] = {};

    snprintf(path_buf, sizeof(path_buf), FOLDER_SHD SEPARATOR_STR "%s." FOLDER_SHD_VERT_EXT, shader_name);

    if ((shaders[0] = load_shader(path_buf, GL_VERTEX_SHADER)) == 0)
    {
        LOG_ERROR("no shaders found");
        return 0;
    }

    snprintf(path_buf, sizeof(path_buf), FOLDER_SHD SEPARATOR_STR "%s." FOLDER_SHD_FRAG_EXT, shader_name);

    if ((shaders[1] = load_shader(path_buf, GL_FRAGMENT_SHADER)) == 0)
    {
        LOG_ERROR("no fragment shader");
        return 0;
    }

    snprintf(path_buf, sizeof(path_buf), FOLDER_SHD SEPARATOR_STR "%s." FOLDER_SHD_GEOM_EXT, shader_name);

    if ((shaders[2] = load_shader(path_buf, GL_GEOMETRY_SHADER)) == 0)
    {
        // its actually fine we just set size to 2
        len = 2;
        // LOG_ERROR("no shaders found");
        // return 0;
    }

    return compile_shader_program(shaders, len);
}

GLuint gl_bind_texture(image *src)
{
    if (!src)
    {
        LOG_ERROR("texture_load Error: No source image");
        return FAIL;
    }

    GLuint texture_id;

    // Create and bind texture

    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, src->width, src->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, src->data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    return texture_id;
}