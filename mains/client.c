#include "../src/include/block_operations.h"
#include "../src/include/block_registry.h"
#include "../src/include/layer_draw_2d.h"

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

int main(int argc, char *argv[])
{
	if (!init_graphics())
		return 1;

	block_registry_t b_reg;
	vec_init(&b_reg);

	client_view_point view = {SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, 1.5f, {}};
	vec_init(&view.draw_order);
	(void)vec_push(&view.draw_order, 0);

	if (!read_block_registry("resources/blocks", &b_reg))
		goto fire_exit;
	sort_by_id(&b_reg);

	// make a world with one layer and one chunk
	const char *world_name = "test_world";
	world *test_world = world_make(1, world_name);
	if (!test_world)
		goto world_free_exit;

	world_layer_alloc(&test_world->layers[0], 2, 2, 32, 0);
	chunk_fill_randomly_from_registry(test_world->layers[0].chunks[0][0], &b_reg, 0);

	// info about textures
	for (int i = 0; i < b_reg.length; i++)
	{
		printf("block id = %d\n", b_reg.data[i].block_sample.id);
		printf("texture name = %s\n", b_reg.data[i].block_texture.filename);
		printf("texture ptr = %p\n", (void *)b_reg.data[i].block_texture.ptr);
	}

	// debug_print_world(test_world);

	// main loop?
	unsigned long frame = 0;

	// handling some general events
	SDL_Event e;
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
					world_layer_fill_randomly(test_world, 0, &b_reg, frame);
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

		view.x += (keystate[SDL_SCANCODE_D] - keystate[SDL_SCANCODE_A]) * 4;
		view.x = MAX(view.x, 0);
		view.y += (keystate[SDL_SCANCODE_S] - keystate[SDL_SCANCODE_W]) * 4;
		view.y = MAX(view.y, 0);

		view.scale += (keystate[SDL_SCANCODE_LSHIFT] - keystate[SDL_SCANCODE_LCTRL]) * 0.1f;
		view.scale = MAX(view.scale, 0.8f);

		SDL_RenderClear(g_renderer);

		// render world
		client_render(test_world, b_reg.data, view, frame / 100);

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