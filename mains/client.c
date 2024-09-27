#include "../src/include/block_operations.h"
#include "../src/include/block_registry.h"
#include "../src/include/layer_draw_2d.h"
#include "../src/include/data_manipulations.h"
#include "../src/include/world_utils.h"
#include "../src/include/lua_integration.h"

void free_world(world *w)
{
	// for (int i = 0; i < w->layers.length; i++)
	// {
	// for (int j = 0; j < w->layers.data[i].size_x; j++)
	// 	for (int k = 0; k < w->layers.data[i].size_y; k++)
	// 		chunk_free(CHUNK_FROM_LAYER(LAYER_FROM_WORLD(w, i), j, k));
	// free(w->layers[i].chunks);
	// }
	for (int i = 0; i < w->layers.length; i++)
		world_layer_free(LAYER_FROM_WORLD(w, i));
	vec_deinit(&w->layers);
	free(w);
}

void chunk_fill_randomly_from_registry(layer_chunk *c, const block_registry_t *b_reg, const int seed, const int range)
{
	srand(seed);

	int minrange = min(range, b_reg->length);

	for (int i = 0; i < c->width; i++)
		for (int j = 0; j < c->width; j++)
		{
			int choosen_block_index = rand() % minrange;

			if (!b_reg->data[choosen_block_index].block_sample.id)
				continue;

			block_copy(BLOCK_FROM_CHUNK(c, i, j), &b_reg->data[choosen_block_index].block_sample);
		}
}

// void world_layer_fill_randomly(world *w, const int layer_index, const block_registry_t *b_reg, const int seed)
// {
// 	// const int TOTAL_CHUNKS = w->layers[layer_index].size_x * w->layers[layer_index].size_y;
// 	const world_layer *layer = LAYER_FROM_WORLD(w, layer_index);
// 	const int size_x = layer->size_x;
// 	const int size_y = layer->size_y;

// 	for (int i = 0; i < size_x; i++)
// 		for (int j = 0; j < size_y; j++)
// 			chunk_fill_randomly_from_registry(CHUNK_FROM_LAYER(layer, i, j), b_reg, seed ^ (i * size_y + j));

// 	// for (int i = 0; i < TOTAL_CHUNKS; i++)
// 	// 	chunk_update_neighbors(w->layers[layer_index].chunks[i / size_y][i % size_y]);
// }

void debug_print_world(world *w)
{
	for (int i = 0; i < w->layers.length; i++)
	{
		printf("layer %d\n", i);
		const world_layer *layer = LAYER_FROM_WORLD(w, i);

		for (int j = 0; j < layer->size_x; j++)
		{
			for (int k = 0; k < layer->size_y; k++)
			{
				printf("chunk %d %d\n", j, k);
				const layer_chunk *c = CHUNK_FROM_LAYER(layer, j, k);
				for (int l = 0; l < layer->chunks[j][k]->width; l++)
				{
					for (int m = 0; m < layer->chunks[j][k]->width; m++)
					{
						block *b = BLOCK_FROM_CHUNK(c, l, m);
						printf("%c", b->id % 96 + 'A');
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

block get_block_from_the_registry(block_registry_t *reg, int id)
{
	for (int i = 0; i < reg->length; i++)
	{
		if (reg->data[i].block_sample.id == id)
			return reg->data[i].block_sample;
	}
	return (block){};
}

int main(int argc, char *argv[])
{
	g_block_size = 16;

	if (!init_graphics())
		return 1;

	scripting_init();

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
	// const int debug_layer_id = 0;

	if (!read_block_registry("resources/blocks", &b_reg))
		goto fire_exit;

	sort_by_id(&b_reg);

	// make a world with one layer and one chunk
	const char *world_name = "test_world";
	world *test_world = world_make(2, world_name);
	if (!test_world)
		goto world_free_exit;

	for (int i = 0; i < test_world->layers.length; i++)
		world_layer_alloc(LAYER_FROM_WORLD(test_world, i), 2, 2, 32, i);

	scripting_define_global_variables(test_world);

	scripting_load_scripts(&b_reg);

	chunk_fill_randomly_from_registry(CHUNK_FROM_LAYER(LAYER_FROM_WORLD(test_world, floor_layer_id), 0, 0), &b_reg, 69, 4);

	const block bug_sample = get_block_from_the_registry(&b_reg, 5);

	set_block(test_world, floor_layer_id, 5, 5, &bug_sample);

	// info about all loaded blocks
	// for (int i = 0; i < b_reg.length; i++)
	// {
	// 	block_resources br = b_reg.data[i];
	// 	printf("block id = %d\n", br.block_sample.id);
	// 	printf("texture name = %s\n", br.block_texture.filename);
	// 	printf("texture ptr = %p\n", (void *)br.block_texture.ptr);
	// 	printf("is animated = %d\n", br.is_animated);
	// 	printf("frames per second = %d\n", br.frames_per_second);
	// 	printf("anim controller = %c\n", br.anim_controller);
	// }

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
			switch (e.type)
			{
			case SDL_KEYDOWN:
			case SDL_KEYUP:
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				scripting_handle_event(&e);
				break;
			case SDL_QUIT:
				goto world_free_exit;
				break;
			}
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

		// floor->x = lerp(floor->x, target_x, 0.015f);
		// floor->y = lerp(floor->y, target_y, 0.015f);

		floor->scale += (keystate[SDL_SCANCODE_LSHIFT] - keystate[SDL_SCANCODE_LCTRL]) * 0.1f;
		floor->scale = MAX(floor->scale, 0.8f);

		SDL_RenderClear(g_renderer);

		// render world
		client_render(test_world, &b_reg, rules, frame / 100);

		SDL_RenderPresent(g_renderer);
		SDL_Delay(16);

		frame++;
	}

	scripting_close();

world_free_exit:
	for (int i = 0; i < test_world->layers.length; i++)
		world_layer_free(LAYER_FROM_WORLD(test_world, i));
	world_free(test_world);
fire_exit:
	exit_graphics();

	return 0;
}