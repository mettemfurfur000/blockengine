
#include <unistd.h>
#include "test_utils.h"
#include <signal.h>

#include "../include/file_system.h"

level test_level = {.name = "test_level"};
room test_room = {.name = "test_room", .width = 8, .height = 8};

int test_room_init()
{
	CHECK(init_level(&test_level))
	CHECK(init_room(&test_room, &test_level))

	(void)vec_push(&test_level.rooms, test_room);

	return SUCCESS;
}

int test_layer_init()
{
	layer new_layer = {.bytes_per_block = 2, .width = 16, .height = 16};
	FLAG_SET(new_layer.flags, LAYER_FLAG_HAS_VARS, 1);

	CHECK(init_layer(&new_layer, &test_room))

	(void)vec_push(&test_room.layers, new_layer);

	layer *l = &test_room.layers.data[0];

	u64 id = 0;

	CHECK(block_get_id(l, 7, 7, &id))
	CHECK(block_set_id(l, 7, 7, 100))
	CHECK(block_get_id(l, 7, 7, &id))

	CHECK(id != 100)

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

	free_level(&on_disk_level);

	return SUCCESS;
}

INIT_TESTING(test_file_system)

RUN_TEST(test_room_init)
RUN_TEST(test_layer_init)
RUN_TEST(test_save_level)
RUN_TEST(test_load_level)

FINISH_TESTING()