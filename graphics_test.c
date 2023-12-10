#include "src/sdl2_basics.c"
#include <stdio.h>

int main(int argc, char *argv[])
{
	if (!init_graphics())
		return 1;

	// black_frame();

	texture test = {};
	texture numbers = {};

	texture_load(&test, "resources/textures/test.png");
	texture_load(&numbers, "resources/textures/numbers.png");

	double i = 0;
	int frame = 0;

	while (handle_events())
	{
		SDL_RenderClear(g_renderer);

		for (size_t j = 0; j < 32; j++)
		{
			for (size_t l = 0; l < 32; l++)
			{
				texture_render_anim(&numbers, j * numbers.frame_side_size, l * numbers.frame_side_size, frame / 15, 1.0);
			}
		}

		texture_render(&test, 64 + sin(i) * 64, 64 + cos(i) * 64, 1 + 0.5 * sin(i));

		SDL_RenderPresent(g_renderer);
		SDL_Delay(16);

		i += 0.01;
		frame++;
	}

	exit_graphics();
	return 0;
}