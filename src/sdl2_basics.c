#include "../include/sdl2_basics.h"
#include "../include/events.h"
#include "../include/opengl_stuff.h"
#include <string.h>

int SCREEN_WIDTH = 1024;
int SCREEN_HEIGHT = 640;
char *window_name = "Block Engine";

SDL_Window *g_window = NULL;
// SDL_Renderer *g_renderer = NULL; // do not use
SDL_GLContext *g_gl_context = NULL;

int g_block_width = 16;

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define KEEPINLIMITS(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

int greatest_common_divisor(int a, int b)
{
    int temp;
    while (b != 0)
    {
        temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

char *get_folder_path(char *file_path, int bonus_for_str_size)
{
    // from you.com/chat
    int len = strlen(file_path);
    char *folder_path = (char *)malloc(len + bonus_for_str_size);
    strcpy(folder_path, file_path);
    // find the last occurrence of a separator
    char *last_slash = strrchr(folder_path, SEPARATOR);
    if (last_slash != NULL)
    {
        // if there is a slash, terminate the string after it
        *last_slash = '\0';
    }
    else
    {
        // if there is no slash, return NULL
        free(folder_path);
        return NULL;
    }

    return folder_path;
}

int init_graphics()
{
    const int flags = SDL_INIT_VIDEO | SDL_INIT_AUDIO;

    if (SDL_Init(flags))
    {
        LOG_ERROR("SDL_Init() error:[%s]", SDL_GetError());
        return FAIL;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    g_window = SDL_CreateWindow(window_name, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH,
                                SCREEN_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
    if (!g_window)
    {
        LOG_ERROR("SDL_CreateWindow() error:[%s]", SDL_GetError());
        return FAIL;
    }

    g_gl_context = SDL_GL_CreateContext(g_window);

    // Setup OpenGL for 2D rendering
    setup_opengl(SCREEN_WIDTH, SCREEN_HEIGHT);

    int result = SDL_RegisterEvents(TOTAL_ENGINE_EVENTS);
    if (result == (Uint32)-1)
    {
        LOG_ERROR("Not able to allocate enough user events");
        LOG_ERROR("SDL_RegisterEvents() error:[%s]", SDL_GetError());
        return FAIL;
    }

    SDL_AudioSpec desired;
    desired.freq = 22050;
    desired.format = AUDIO_S16SYS;
    desired.channels = 2;
    desired.samples = 4096;
    desired.callback = NULL;
    SDL_AudioSpec obtained;

    if (SDL_OpenAudio(&desired, &obtained) < 0)
    {
        LOG_ERROR("SDL_OpenAudio Error: %s", SDL_GetError());
        return FAIL;
    }

    if (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 1, 1024) != 0)
    {
        LOG_ERROR("Mix_OpenAudio Error: %s", SDL_GetError());
        return FAIL;
    }

    if (Mix_Init(MIX_INIT_MP3) != MIX_INIT_MP3)
    {
        LOG_ERROR("Mix_Init Error: %s", SDL_GetError());
        return FAIL;
    }

    // Mix_ChannelFinished()

    // SDL_ShowCursor(SDL_DISABLE);

    return SUCCESS;
}

int exit_graphics()
{
    SDL_DestroyWindow(g_window);

    // SDL_DestroyRenderer(g_renderer);
    SDL_GL_DeleteContext(g_gl_context);

    SDL_Quit();

    Mix_Quit();

    return SUCCESS;
}

// int texture_bind(image *img, texture *tex)
// {
//     GLuint texture_id;

//     glGenTextures(1, &texture_id);
//     glBindTexture(GL_TEXTURE_2D, texture_id);

//     glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img->width, img->height, 0,
//                  GL_RGBA, GL_UNSIGNED_BYTE, img->data);

//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

//     // tex
// }

// int texture_load(void *dest, char *path_to_file)
// {
//     // Alot of error checking here.
//     if (!dest)
//     {
//         LOG_ERROR("texture_load Error: No desination texture");
//         return FAIL;
//     }

//     if (!path_to_file)
//     {
//         LOG_ERROR("texture_load Error: No path to file");
//         return FAIL;
//     }

//     // if (strlen(path_to_file) >= MAX_PATH)
//     // {
//     // 	LOG_ERROR("texture_load Error: Path to file too long: %s",
//     // path_to_file); 	return FAIL;
//     // }

//     // Load image

//     u8 *image_data;
//     int channels;

//     if (!(image_data = stbi_load(path_to_file, &dest->width, &dest->height,
//                                  &channels, STBI_rgb_alpha)))
//     {
//         stbi_image_free(image_data);
//         LOG_ERROR("texture_load Error: stbi_load failed, no such file: %s",
//                   path_to_file);
//         return FAIL;
//     }

//     if (dest->width <= 0 || dest->height <= 0)
//     {
//         stbi_image_free(image_data);
//         LOG_ERROR("texture_load Error: stbi_load failed, invalid size: %s",
//                   path_to_file);
//         return FAIL;
//     }

//     GLuint texture_id;

//     // Create and bind texture

//     glGenTextures(1, &texture_id);
//     glBindTexture(GL_TEXTURE_2D, texture_id);

//     glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, dest->width, dest->height, 0,
//                  GL_RGBA, GL_UNSIGNED_BYTE, image_data);

//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

//     // Free image data
//     stbi_image_free(image_data);

//     // Store all the values in the texture struct
//     dest->gl_id = texture_id;

//     dest->frames = dest->width / g_block_width;
//     dest->types = dest->height / g_block_width;
//     dest->total_frames = dest->frames * dest->types;

//     // copy filename for later use
//     char *filename = strrchr(path_to_file, SEPARATOR) + 1;
//     int namelen = strlen(filename);

//     dest->filename = (char *)malloc(namelen + 1);

//     strcpy(dest->filename, filename);

//     return SUCCESS;
// }

// int load_image_alt(image *img_dst, atlas_info *atlas_info,  char
// *path_to_file)
// {
//     CHECK_PTR(img_dst)
//     CHECK_PTR(atlas_info)
//     CHECK_PTR(path_to_file)

//     u8 *image_data;
//     int channels;

//     if (!(image_data = stbi_load(path_to_file, &img_dst->width,
//                                  &img_dst->height, &channels,
//                                  STBI_rgb_alpha)))
//     {
//         stbi_image_free(image_data);
//         LOG_ERROR("texture_load Error: stbi_load failed, no such file: %s",
//                   path_to_file);
//         return FAIL;
//     }

//     if (img_dst->width <= 0 || img_dst->height <= 0)
//     {
//         stbi_image_free(image_data);
//         LOG_ERROR("texture_load Error: stbi_load failed, invalid size: %s",
//                   path_to_file);
//         return FAIL;
//     }

//     img_dst->data = image_data;

//     // partially fill out de texture fields, we will need em later

//     atlas_info->width = atlas_info->width;
//     atlas_info->height = atlas_info->height;

//     atlas_info->frames = atlas_info->width / g_block_width;
//     atlas_info->types = atlas_info->height / g_block_width;

//     atlas_info->total_frames = atlas_info->frames * atlas_info->types;

//     // copy filename for later use
//     char *filename = strrchr(path_to_file, SEPARATOR) + 1;
//     atlas_info->filename = strdup(filename);

//     return SUCCESS;
// }

// void free_texture(texture *t)
// {
//     SAFE_FREE(t->filename);

//     if (t->gl_id)
//         glDeleteTextures(1, &t->gl_id);
// }

int record_atlas_info(atlas_info *atlas_info, image *img)
{
    atlas_info->width = img->width;
    atlas_info->height = img->height;

    atlas_info->frames = img->width / g_block_width;
    atlas_info->types = img->height / g_block_width;

    atlas_info->total_frames = atlas_info->frames * atlas_info->types;

    // copy filename for later use
    // char *filename = strrchr(path_to_file, SEPARATOR) + 1;
    // atlas_info->filename = strdup(filename);

    return SUCCESS;
}

Uint32 getChunkTimeMilliseconds(Mix_Chunk *chunk)
{
    Uint32 points = 0;
    Uint32 frames = 0;
    int freq = 0;
    Uint16 fmt = 0;
    int chans = 0;
    if (!Mix_QuerySpec(&freq, &fmt, &chans))
        return 0;
    points = (chunk->alen / ((fmt & 0xFF) / 8));

    frames = (points / chans);
    return (frames * 1000) / freq;
}

int sound_load(sound *dest, char *path_to_file)
{
    dest->obj = Mix_LoadWAV_RW(SDL_RWFromFile(path_to_file, "rb"), 1);
    if (dest->obj == NULL)
    {
        LOG_ERROR("Couldn't load %s: %s", path_to_file, SDL_GetError());

        return FAIL;
    }

    // copy filename for later use
    char *filename = strrchr(path_to_file, SEPARATOR) + 1;
    int namelen = strlen(filename);

    dest->filename = (char *)malloc(namelen + 1);
    dest->length_ms = dest->obj->alen;

    strcpy(dest->filename, filename);

    return SUCCESS;
}

void free_sound(sound *s)
{
    Mix_FreeChunk(s->obj);
}

int music_load(music *dest, char *filename)
{
    dest->mus = Mix_LoadMUS_RW(SDL_RWFromFile(filename, "rb"), 1);
    if (dest->mus == NULL)
    {
        LOG_ERROR("Couldn't load %s: %s", filename, SDL_GetError());

        return FAIL;
    }

    dest->type = Mix_GetMusicType(dest->mus);
    dest->length_ms = Mix_MusicDuration(dest->mus) * 1000.0;

    return SUCCESS;
}

void free_music(music *s)
{
    Mix_FreeMusic(s->mus);
}

// void play_sound_randomly(vec_sound_t sounds)
// {
// 	u16 selected = rand() % sounds.length;

// 	Mix_PlayChannel(-1, sounds.data[selected].obj, 0);
// }

// it trusts you to pass valid values in, be careful with frames and types...
// int block_render(texture *texture, const int x, const int y, u8 frame, u8 type,
//                  u8 ignore_type, u8 local_block_width, u8 flip,
//                  unsigned short rotation)
// {
//     if (texture->gl_id == 0)
//         return SUCCESS;
//     // frame is an index into one of the frames on a texture
//     frame = frame % texture->total_frames; // wrap frames

//     // get texture coordinates of a frame

//     // int frame_x = (frame % texture->frames) * g_block_width;
//     // int frame_y = 0;

//     // now in floats for opengl, also normalized

//     float frame_x = (frame % texture->frames) / (float)texture->frames;
//     float frame_y = 0.0f;

//     if (ignore_type)
//         frame_y = (u8)(frame / texture->frames) / (float)texture->types;
//     else
//         frame_y = (u8)(type % texture->types) / (float)texture->types;

//     glEnable(GL_TEXTURE_2D);
//     glEnable(GL_BLEND);
//     glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

//     glBindTexture(GL_TEXTURE_2D, texture->gl_id);

//     glBegin(GL_QUADS);
//     {
//         // top left
//         glTexCoord2f(frame_x, frame_y);
//         glVertex2i(x, y);
//         // top right
//         glTexCoord2f(frame_x + 1.0f / texture->frames, frame_y);
//         glVertex2i(x + local_block_width, y);
//         // bottom right
//         glTexCoord2f(frame_x + 1.0f / texture->frames,
//                      frame_y + 1.0f / texture->types);
//         glVertex2i(x + local_block_width, y + local_block_width);
//         // bottom left
//         glTexCoord2f(frame_x, frame_y + 1.0f / texture->types);
//         glVertex2i(x, y + local_block_width);
//     }
//     glEnd();

//     return SUCCESS;
// }
// TODO:

/*
neighbours_mask expected layout, bit by bit:
 0 1 2
 3 X 4
 5 6 7

 X is block being rendered
 if bit is 1, then neighbour have same block id
*/

// int block_render_connected(texture *texture, int x, int y, u8
// neighbours_mask)
// {
// 	const u8 half_w = g_block_width / 2;
// 	const u8 quart_w = g_block_width / 4;
// 	const u8 three_quarts_w = half_w + quart_w;

// 	SDL_Rect src = {0, 0, half_w, half_w};
// 	SDL_Rect dest = {x + quart_w, y + quart_w, half_w, half_w};
// 	SDL_Point rotation_center = {0, 0};

// 	SDL_RenderCopy(g_renderer, texture->ptr, &src, &dest);

// 	for ( u32 i = 1; i < 7; i += 2)
// 	{
// 		if (i == 5)
// 			i--;

// 		double angle;

// 		switch (i)
// 		{
// 		case 1:
// 			angle = 90;
// 			break;
// 		case 3:
// 			angle = 180;
// 			break;
// 		case 4:
// 			angle = 0;
// 			break;
// 		case 6:
// 			angle = 270;
// 			break;
// 		}

// 		src.x = (neighbours_mask & (0b10000000 >> i)) ? half_w : half_w
// + quart_w; 		src.y = 0; 		src.h = half_w;
// src.w = quart_w;

// 		dest.x = x + (i == 1 || i == 6 ? quart_w
// 					  : i == 4		   ?
// three_quarts_w
// : 0); 		dest.y = y + (i == 1   ? 0
// : i == 6 ? three_quarts_w : quart_w); 		dest.h = half_w;
// dest.w = quart_w;

// 		rotation_center.x = 0;
// 		rotation_center.y = 0;

// 		SDL_RenderCopyEx(g_renderer, texture->ptr, &src, &dest, angle,
// &rotation_center, SDL_FLIP_NONE);
// 	}
// }
