
#include <unistd.h>
#include "test_utils.h"
#include <signal.h>

#include "../include/file_system.h"

level test_level = {.name = "test_level"};
room test_room = {.name = "test_room", .width = 8, .height = 8};
room* t_room_ref = NULL;

int test_room_init()
{
	CHECK(init_level(&test_level))
	CHECK(init_room(&test_room, &test_level))

	(void)vec_push(&test_level.rooms, test_room);

	t_room_ref = &test_level.rooms.data[0];

	return SUCCESS;
}

int test_layer_init()
{
	layer new_layer = {
		.block_size = 2,
		.var_index_size = 2,
		.width = t_room_ref->width,
		.height = t_room_ref->height

	};
	FLAG_SET(new_layer.flags, LAYER_FLAG_HAS_VARS, 1);

	CHECK(init_layer(&new_layer, t_room_ref))

	(void)vec_push(&t_room_ref->layers, new_layer);

	layer *l = &t_room_ref->layers.data[0];

	u64 id = 0;

	for(int i = 0; i < l->width * l->height; i++)
	{
		CHECK(block_set_id(l, i % l->width, i / l->width, i))
	}

	for(int i = 0; i < l->width * l->height; i++)
	{
		CHECK(block_get_id(l, i % l->width, i / l->width, &id))
		CHECK(id != i)
	}

	return SUCCESS;
}

int test_save_level()
{
	CHECK(save_level(test_level))

	return SUCCESS;
}

int test_load_level()
{
	level on_disk_level = {};

	CHECK(load_level(&on_disk_level, "test_level"))
	CHECK(on_disk_level.rooms.length != 1)
	CHECK(on_disk_level.rooms.data[0].width != 8)

	layer* l = &on_disk_level.rooms.data[0].layers.data[0];

	u64 id = 0;

	for(int i = 0; i < l->width * l->height; i++)
	{
		CHECK(block_get_id(l, i % l->width, i / l->width, &id))
		CHECK(id != i)
	}

	free_level(&on_disk_level);

	return SUCCESS;
}

INIT_TESTING(test_file_system)

RUN_TEST(test_room_init)
RUN_TEST(test_layer_init)
RUN_TEST(test_save_level)
RUN_TEST(test_load_level)

FINISH_TESTING()