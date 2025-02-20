#include "../include/vars.h"
#include "../include/level.h"
#include "../include/rendering.h"
#include "../include/block_registry.h"
#include "../include/scripting.h"

#include <time.h>

// #include <omp.h>

// void chunk_fill_randomly_from_registry(layer_chunk *c, const block_resources_t *reg, const int seed, const int start_range, const int range)
// {
// 	srand(seed);

// 	int minrange = min(range, reg->length) - start_range;

// 	for (int i = 0; i < c->width; i++)
// 		for (int j = 0; j < c->width; j++)
// 		{
// 			int choosen_block_index = start_range + rand() % minrange;

// 			if (!reg->data[choosen_block_index].block_sample.id)
// 				continue;

// 			if (FLAG_GET(reg->data[choosen_block_index].flags, B_RES_FLAG_IS_FILLER))
// 				continue;

// 			block_copy(BLOCK_FROM_CHUNK(c, i, j), &reg->data[choosen_block_index].block_sample);
// 		}
// }

// void world_layer_fill_randomly(world *w, const int layer_index, const block_resources_t *reg, const int seed, const int start_range, const int range)
// {
// 	// const int TOTAL_CHUNKS = w->layers[layer_index].size_x * w->layers[layer_index].size_y;
// 	const world_layer *layer = LAYER_FROM_WORLD(w, layer_index);
// 	const int size_x = layer->size_x;
// 	const int size_y = layer->size_y;

// 	for (int i = 0; i < size_x; i++)
// 		for (int j = 0; j < size_y; j++)
// 			chunk_fill_randomly_from_registry(CHUNK_FROM_LAYER(layer, i, j), reg, seed ^ (i * size_y + j), start_range, range);
// }

// void debug_print_world(world *w)
// {
// 	for (int i = 0; i < w->layers.length; i++)
// 	{
// 		printf("layer %d\n", i);
// 		const world_layer *layer = LAYER_FROM_WORLD(w, i);

// 		for (int j = 0; j < layer->size_x; j++)
// 		{
// 			for (int k = 0; k < layer->size_y; k++)
// 			{
// 				printf("chunk %d %d\n", j, k);
// 				const layer_chunk *c = CHUNK_FROM_LAYER(layer, j, k);
// 				for (int l = 0; l < layer->chunks[j][k]->width; l++)
// 				{
// 					for (int m = 0; m < layer->chunks[j][k]->width; m++)
// 					{
// 						block *b = BLOCK_FROM_CHUNK(c, l, m);
// 						printf("%c", b->id % 96 + 'A');
// 					}
// 					printf("\n");
// 				}
// 			}
// 		}
// 	}
// }

float lerp(float a, float b, float t)
{
	return a + t * (b - a);
}

u8 set_registry_block(layer *l, u32 x, u32 y, u64 id)
{
	CHECK_PTR(l, set_registry_block);
	CHECK_PTR(l->registry, set_registry_block);

	block_resources_t reg = l->registry->resources;

	block_set_id(l, x, y, id);
	block_set_vars(l, x, y, reg.data[id].vars);

	return SUCCESS;
}

level test_level_init()
{
	g_block_width = 16;

	level lvl = {.name = "client_test_level"};
	init_level(&lvl);

	room r = {.name = "client_test_room", .width = 40, .height = 30};
	init_room(&r, &lvl);

	(void)vec_push(&lvl.rooms, r);

	layer l = {.bytes_per_block = 2};
	FLAG_SET(l.flags, LAYER_FLAG_HAS_VARS, 1);
	init_layer(&l, &r);

	(void)vec_push(&lvl.rooms.data[0].layers, l);

	return lvl;
}

void test_level_generate_test_layout(layer *l)
{
	set_registry_block(l, 2, 2, 5);
	set_registry_block(l, 4, 4, 10);
}

void test_world_exit(level lvl)
{
	free_level(&lvl);
}

client_render_rules prepare_rendering_rules()
{
	client_render_rules rules = {.screen_width = SCREEN_WIDTH, .screen_height = SCREEN_HEIGHT, .cur_frame = 0, {}, {}};

	vec_init(&rules.draw_order);
	vec_init(&rules.slices);

	(void)vec_push(&rules.draw_order, 1);
	(void)vec_push(&rules.draw_order, 0);

	layer_slice t = {.x = 0, .y = 0, .w = rules.screen_width, .h = rules.screen_height, .zoom = 3};

	(void)vec_push(&rules.slices, t);
	(void)vec_push(&rules.slices, t);

	return rules;
}

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)         \
	((byte) & 0x80 ? '1' : '0'),     \
		((byte) & 0x40 ? '1' : '0'), \
		((byte) & 0x20 ? '1' : '0'), \
		((byte) & 0x10 ? '1' : '0'), \
		((byte) & 0x08 ? '1' : '0'), \
		((byte) & 0x04 ? '1' : '0'), \
		((byte) & 0x02 ? '1' : '0'), \
		((byte) & 0x01 ? '1' : '0')

int main(int argc, char *argv[])
{
	log_start("test.log");

	if (init_graphics() == FAIL)
		return 1;

	block_registry reg = {};

	if (read_block_registry("test", &reg) == FAIL)
	{
		LOG_ERROR("failed to read block registry");
		return 1;
	}

	block_resources_t resources = reg.resources;

	for (int i = 0; i < resources.length; i++)
	{
		printf("%lld %s " BYTE_TO_BINARY_PATTERN "\n", resources.data[i].id, resources.data[i].block_texture.filename, BYTE_TO_BINARY(resources.data[i].flags));
	}

	level lvl = test_level_init();

	layer *floor_layer_ref = &lvl.rooms.data[0].layers.data[0];

	floor_layer_ref->registry = &reg;
	test_level_generate_test_layout(floor_layer_ref);

	unsigned long frame = 0;

	const int target_fps = 60;
	const int ms_per_s = 1000 / target_fps;

	const int target_tps = 10;
	const int tick_period = 1000 / target_tps;

	client_render_rules rules = prepare_rendering_rules();

	rules.slices.data[0].ref = floor_layer_ref;

	scripting_init();

	LUA_SET_GLOBAL_OBJECT("g_level", &lvl);
	LUA_SET_GLOBAL_OBJECT("g_reg", &reg);
	LUA_SET_GLOBAL_OBJECT("g_render_rules", &rules);

	scripting_load_scripts(&reg);

	SDL_Event e;

	int latest_logic_tick = SDL_GetTicks();

	// scripting_handle_event(NULL, 2); // tick event
	e.type = ENGINE_INIT;

	call_handlers(e);

	for (;;)
	{
		int frame_begin_tick = SDL_GetTicks();

		while (SDL_PollEvent(&e))
		{
			switch (e.type) // vanilla sdl event handling
			{
			case SDL_KEYDOWN:
			case SDL_KEYUP:
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				break;
			case SDL_QUIT:
				goto logic_exit;
				break;
			case SDL_RENDER_TARGETS_RESET:
				// g_renderer = SDL_CreateRenderer(g_window, -1, 0);
				// printf("erm, reload textures?");
				break;
			}

			// scripting event handling
			call_handlers(e);
		}

		if (latest_logic_tick + tick_period <= frame_begin_tick)
		{
			latest_logic_tick = SDL_GetTicks();
			e.type = ENGINE_TICK;
			call_handlers(e);
		}

		SDL_RenderClear(g_renderer);
		client_render(rules);

		SDL_RenderPresent(g_renderer);

		int loop_took = SDL_GetTicks() - frame_begin_tick;
		int chill_time = ms_per_s - loop_took;

		SDL_Delay(max(1, chill_time));

		frame++;
	}
logic_exit:

	exit_graphics();

	scripting_close();

	test_world_exit(lvl);

	return 0;
}