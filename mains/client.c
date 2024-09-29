#include "../src/include/block_operations.h"
#include "../src/include/block_registry.h"
#include "../src/include/layer_draw_2d.h"
#include "../src/include/data_manipulations.h"
#include "../src/include/world_utils.h"
#include "../src/include/lua_integration.h"

#include <time.h>

// #include <omp.h>

world *g_world = 0;
block_registry_t *g_reg = 0;

void free_world(world *w)
{
	for (int i = 0; i < w->layers.length; i++)
		world_layer_free(LAYER_FROM_WORLD(w, i));
	vec_deinit(&w->layers);
	free(w);
}

void chunk_fill_randomly_from_registry(layer_chunk *c, const block_registry_t *reg, const int seed, const int range)
{
	srand(seed);

	int minrange = min(range, reg->length);

	for (int i = 0; i < c->width; i++)
		for (int j = 0; j < c->width; j++)
		{
			int choosen_block_index = rand() % minrange;

			if (!reg->data[choosen_block_index].block_sample.id)
				continue;

			if (reg->data[choosen_block_index].is_filler)
				continue;

			block_copy(BLOCK_FROM_CHUNK(c, i, j), &reg->data[choosen_block_index].block_sample);
		}
}

void world_layer_fill_randomly(world *w, const int layer_index, const block_registry_t *reg, const int seed, const int range)
{
	// const int TOTAL_CHUNKS = w->layers[layer_index].size_x * w->layers[layer_index].size_y;
	const world_layer *layer = LAYER_FROM_WORLD(w, layer_index);
	const int size_x = layer->size_x;
	const int size_y = layer->size_y;

	for (int i = 0; i < size_x; i++)
		for (int j = 0; j < size_y; j++)
			chunk_fill_randomly_from_registry(CHUNK_FROM_LAYER(layer, i, j), reg, seed ^ (i * size_y + j), range);

	// for (int i = 0; i < TOTAL_CHUNKS; i++)
	// 	chunk_update_neighbors(w->layers[layer_index].chunks[i / size_y][i % size_y]);
}

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

int test_world_init(world **world, block_registry_t *reg)
{
	g_block_size = 16;
	const int floor_layer_id = 1;

	// make a world with one layer and one chunk
	const char *world_name = "test_world";
	*world = world_make(2, world_name);
	if (!*world)
		return FAIL;

	for (int i = 0; i < (*world)->layers.length; i++)
		world_layer_alloc(LAYER_FROM_WORLD((*world), i), 2, 2, 32, i);

	chunk_fill_randomly_from_registry(CHUNK_FROM_LAYER(LAYER_FROM_WORLD((*world), floor_layer_id), 0, 0), (reg), 69, 4);

	world_layer_fill_randomly(*world, floor_layer_id, (reg), time(NULL), 4);

	const block bug_sample = get_block_from_the_registry(reg, 5);
	set_block((*world), floor_layer_id, 5, 5, &bug_sample);

	return SUCCESS;
}

void test_world_exit(world *world, block_registry_t *reg)
{
	for (int i = 0; i < world->layers.length; i++)
		world_layer_free(LAYER_FROM_WORLD(world, i));

	world_free(world);
}

client_render_rules prepare_rendering_rules()
{
	client_render_rules rules = {SCREEN_WIDTH, SCREEN_HEIGHT, {}, {}};

	vec_init(&rules.draw_order);
	vec_init(&rules.slices);

	(void)vec_push(&rules.draw_order, 1);
	(void)vec_push(&rules.draw_order, 0);

	layer_slice t = {0, 0, rules.screen_width, rules.screen_height, 1.0f};

	(void)vec_push(&rules.slices, t);
	(void)vec_push(&rules.slices, t);

	return rules;
}

int main(int argc, char *argv[])
{
	block_registry_t reg;
	vec_init(&reg);

	g_reg = &reg;

	if (!init_graphics())
		return 1;

	if (!read_block_registry("resources/blocks", g_reg))
		return 1;

	if (test_world_init(&g_world, g_reg) == FAIL)
		return 1;

	scripting_init();
	scripting_define_global_variables(g_world);
	scripting_load_scripts(g_reg);

	unsigned long frame = 0;

	const int target_fps = 30;
	const int ms_per_s = 1000 / target_fps;

	const int target_tps = 10;
	const int tick_period = 1000 / target_tps;

	client_render_rules rules = prepare_rendering_rules();

	SDL_Event e;

	int latest_logic_tick = SDL_GetTicks();
	scripting_handle_event(NULL, 2); // tick event

	for (;;)
	{
		int frame_begin_tick = SDL_GetTicks();

		while (SDL_PollEvent(&e))
		{
			switch (e.type)
			{
			case SDL_KEYDOWN:
			case SDL_KEYUP:
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				scripting_handle_event(&e, 0);
				break;
			case SDL_QUIT:
				goto logic_exit;
				break;
			case SDL_RENDER_TARGETS_RESET:
				// g_renderer = SDL_CreateRenderer(g_window, -1, 0);
				// printf("erm, reload textures?");
				break;
			}
		}

		if (latest_logic_tick + tick_period <= frame_begin_tick)
		{
			latest_logic_tick = SDL_GetTicks();
			scripting_handle_event(NULL, 1); // tick event
		}

		SDL_RenderClear(g_renderer);
		client_render(g_world, g_reg, rules, frame / 100);

		SDL_RenderPresent(g_renderer);

		int loop_took = SDL_GetTicks() - frame_begin_tick;
		int chill_time = ms_per_s - loop_took;

		SDL_Delay(max(1, chill_time));

		frame++;
	}
logic_exit:

	exit_graphics();

	scripting_close();

	test_world_exit(g_world, &reg);

	return 0;
}