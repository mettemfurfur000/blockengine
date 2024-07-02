#include "../src/include/block_operations.h"
#include "../src/include/block_registry.h"
#include "../src/include/layer_draw_2d.h"
#include "../src/include/data_manipulations.h"
#include "../src/include/world_utils.h"

void free_world(world *w)
{
	for (int i = 0; i < w->depth; i++)
	{
		for (int j = 0; j < w->layers[i].size_x; j++)
			for (int k = 0; k < w->layers[i].size_y; k++)
				chunk_free(w->layers[i].chunks[j][k]);
		free(w->layers[i].chunks);
	}
	free(w->layers);
	free(w);
}

void chunk_fill_randomly_from_registry(layer_chunk *c, const block_registry_t *b_reg, const int seed)
{
	srand(seed);

	for (int i = 0; i < c->width; i++)
		for (int j = 0; j < c->width; j++)
		{
			int choosen_block_index = rand() % b_reg->length;

			if (!b_reg->data[choosen_block_index].block_sample.id)
				continue;

			block_copy(&c->blocks[i][j], &b_reg->data[choosen_block_index].block_sample);
		}
}

void world_layer_fill_randomly(world *w, const int layer_index, const block_registry_t *b_reg, const int seed)
{
	// const int TOTAL_CHUNKS = w->layers[layer_index].size_x * w->layers[layer_index].size_y;
	const int size_x = w->layers[layer_index].size_x;
	const int size_y = w->layers[layer_index].size_y;

	for (int i = 0; i < size_x; i++)
		for (int j = 0; j < size_y; j++)
			chunk_fill_randomly_from_registry(w->layers[layer_index].chunks[i][j], b_reg, seed ^ (i * size_y + j));

	// for (int i = 0; i < TOTAL_CHUNKS; i++)
	// 	chunk_update_neighbors(w->layers[layer_index].chunks[i / size_y][i % size_y]);
}

void debug_print_world(world *w)
{
	for (int i = 0; i < w->depth; i++)
	{
		printf("layer %d\n", i);
		for (int j = 0; j < w->layers[i].size_x; j++)
		{
			for (int k = 0; k < w->layers[i].size_y; k++)
			{
				printf("chunk %d %d\n", j, k);
				for (int l = 0; l < w->layers[i].chunks[j][k]->width; l++)
				{
					for (int m = 0; m < w->layers[i].chunks[j][k]->width; m++)
					{
						block *b = &w->layers[i].chunks[j][k]->blocks[l][m];
						printf("[%c]", b->id % 96 + 'A');
					}
					printf("\n");
				}
			}
		}
	}
}

float lerp(float a, float b, float t)
{
	return a + t * (b - a);
}

int main(int argc, char *argv[])
{
	g_block_size = 16;

	if (!init_graphics())
		return 1;

	block_registry_t b_reg;
	vec_init(&b_reg);

	client_render_rules rules = {SCREEN_WIDTH, SCREEN_HEIGHT, {}, {}};
	vec_init(&rules.draw_order);
	vec_init(&rules.slices);

	(void)vec_push(&rules.draw_order, 1);
	(void)vec_push(&rules.draw_order, 0);

	layer_slice t = {0, 0, rules.screen_width, rules.screen_height, 1.0f};
	(void)vec_push(&rules.slices, t);
	(void)vec_push(&rules.slices, t);

	const int floor_layer_id = 1;
	const int debug_layer_id = 0;

	if (!read_block_registry("resources/blocks", &b_reg))
		goto fire_exit;
	sort_by_id(&b_reg);

	// make a world with one layer and one chunk
	const char *world_name = "test_world";
	world *test_world = world_make(2, world_name);
	if (!test_world)
		goto world_free_exit;

	for (int i = 0; i < test_world->depth; i++)
		world_layer_alloc(&test_world->layers[i], 2, 2, 32, i);

	chunk_fill_randomly_from_registry(test_world->layers[floor_layer_id].chunks[0][0], &b_reg, 69);

	// info about textures
	for (int i = 0; i < b_reg.length; i++)
	{
		block_resources br = b_reg.data[i];
		printf("block id = %d\n", br.block_sample.id);
		printf("texture name = %s\n", br.block_texture.filename);
		printf("texture ptr = %p\n", (void *)br.block_texture.ptr);
		printf("is animated = %d\n", br.is_animated);
		printf("frames per second = %d\n", br.frames_per_second);
		printf("anim controller = %c\n", br.anim_controller);
	}

	// debug_print_world(test_world);

	// main loop?
	unsigned long frame = 0;

	unsigned int last_move_press_time = 0;

	float target_x = 0, target_y = 0;
	SDL_Event e;

	// layer_slice *debug_layer = &rules.slices.data[debug_layer_id];
	layer_slice *floor = &rules.slices.data[floor_layer_id];

	for (;;)
	{
		// handle events
		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_QUIT)
				goto world_free_exit;
			else if (e.type == SDL_KEYDOWN)
			{
				switch (e.key.keysym.sym)
				{
				case SDLK_ESCAPE:
					goto world_free_exit;
					break;
				case SDLK_SPACE:
					world_layer_fill_randomly(test_world, floor_layer_id, &b_reg, frame);
					// bprintf(test_world, &b_reg, 0, 0, 0, 32, "Hello world from the client! 0w0");
					break;
				default:
					break;
				}
			}
			// else if (e.type == SDL_MOUSEMOTION)
			// {
			// 	view.mouse_x = e.motion.x;
			// 	view.mouse_y = e.motion.y;
			// }
		}
		// handling keyboard state
		const Uint8 *keystate = SDL_GetKeyboardState(NULL);

		if (keystate[SDL_SCANCODE_D] || keystate[SDL_SCANCODE_A] || keystate[SDL_SCANCODE_S] || keystate[SDL_SCANCODE_W])
		{
			if (SDL_GetTicks() - last_move_press_time > 17)
			{
				target_x += (keystate[SDL_SCANCODE_D] - keystate[SDL_SCANCODE_A]) * g_block_size;
				target_y += (keystate[SDL_SCANCODE_S] - keystate[SDL_SCANCODE_W]) * g_block_size;

				last_move_press_time = SDL_GetTicks();
			}
		}

		floor->x = lerp(floor->x, target_x, 0.015f);
		floor->y = lerp(floor->y, target_y, 0.015f);

		floor->scale += (keystate[SDL_SCANCODE_LSHIFT] - keystate[SDL_SCANCODE_LCTRL]) * 0.1f;
		floor->scale = MAX(floor->scale, 0.8f);

		SDL_RenderClear(g_renderer);

		// render world
		client_render(test_world, &b_reg, rules, frame / 100);

		SDL_RenderPresent(g_renderer);
		SDL_Delay(16);

		frame++;
	}

world_free_exit:
	world_layer_free(&test_world->layers[0]);
	world_free(test_world);
fire_exit:
	exit_graphics();

	return 0;
}