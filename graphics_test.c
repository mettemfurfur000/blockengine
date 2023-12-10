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

	int number_states[32][32] = {0};

	while (handle_events())
	{
		SDL_RenderClear(g_renderer);

		for (int sel = rand() % 5; sel >= 0; sel--)
		{
			number_states[rand() % 32][rand() % 32] += 1;
		}

		for (size_t j = 0; j < 32; j++)
			for (size_t l = 0; l < 32; l++)
					texture_render_anim(&numbers, j * numbers.frame_side_size, l * numbers.frame_side_size, number_states[l][j], 1.0);

		texture_render(&test, 128 + sin(i) * 32, 16 + cos(i) * 8, 1 + 0.5 * sin(i) - 0.2 * cos(i + 1));

		SDL_RenderPresent(g_renderer);
		SDL_Delay(16);

		i += 0.07;
		frame++;
	}

	exit_graphics();
	return 0;
}