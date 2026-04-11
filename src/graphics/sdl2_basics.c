#include "include/sdl2_basics.h"
#include "SDL_video.h"
#include "include/events.h"
#include "include/general.h"
#include "include/opengl_stuff.h"
#include <string.h>

int SCREEN_WIDTH = 800;
int SCREEN_HEIGHT = 600;
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

int init_sound()
{
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

	return SUCCESS;
}

int init_graphics(bool set_fullscreen)
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
	setup_opengl(SCREEN_WIDTH, SCREEN_HEIGHT);

	int result = SDL_RegisterEvents(TOTAL_ENGINE_EVENTS);
	if (result == (Uint32)-1)
	{
		LOG_ERROR("Not able to allocate enough user events");
		LOG_ERROR("SDL_RegisterEvents() error:[%s]", SDL_GetError());
		return FAIL;
	}

	if(init_sound() != SUCCESS)
	{
		LOG_ERROR("Failed to initialize sound");
		return FAIL;
	}

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
