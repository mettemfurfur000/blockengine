#include "include/sdl2_extras.h"
#include "include/block_renderer_v2.h"
#include "include/sdl2_basics.h"

void set_fullscreen(bool set_full)
{
	SDL_SetWindowFullscreen(g_window, set_full ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
	SDL_GetWindowSize(g_window, &SCREEN_WIDTH, &SCREEN_HEIGHT);

	printf("Got window size %d x %d for fullscreen\n", SCREEN_WIDTH, SCREEN_HEIGHT);

	renderer_v2_resize(SCREEN_WIDTH, SCREEN_HEIGHT);
}