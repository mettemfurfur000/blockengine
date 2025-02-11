
#include <unistd.h>
#include "test_utils.h"
#include <signal.h>

#include "../include/file_system.h"

level test_level = {.name = "test_level"};
room test_room = {.name = "test_room", .width = 8, .height = 8};

int test_room_init()
{
	init_level(&test_level);
	init_room(&test_room, &test_level);

	(void)vec_push(&test_level.rooms, test_room);

	return 1;
}

int test_layer_init()
{
	layer new_layer = {.bytes_per_block = 2};
	FLAG_SET(new_layer.flags, LAYER_FLAG_HAS_VARS, 1);

	init_layer(&new_layer, &test_room);

	(void)vec_push(&test_room.layers, new_layer);

	layer *l = &test_room.layers.data[0];

	u64 id = 0;

	if (block_get_id(l, 7, 7, &id) != SUCCESS)
	{
		LOG_ERROR("Failed to get block id");
		return 0;
	}

	if (block_set_id(l, 7, 7, 100) != SUCCESS)
	{
		LOG_ERROR("Failed to set block id");
		return 0;
	}

	if (block_get_id(l, 7, 7, &id) != SUCCESS)
	{
		LOG_ERROR("Block id was not set");
		return 0;
	}

	if (id != 100)
	{
		LOG_ERROR("Block id is not 100");
		return 0;
	}

	return 1;
}

int test_save_level()
{
	if (save_level(test_level) != SUCCESS)
	{
		LOG_ERROR("Failed to save level");
		return 0;
	}

	return 1;
}

int test_load_level()
{
	level on_disk_level = {};
	if (load_level(&on_disk_level, "test_level") != SUCCESS)
	{
		LOG_ERROR("Failed to load level");
		return 0;
	}

	if (on_disk_level.rooms.length != 1)
	{
		LOG_ERROR("Level rooms length is not 1, it is %d", on_disk_level.rooms.length);
		return 0;
	}

	if (on_disk_level.rooms.data[0].width != 8)
	{
		LOG_ERROR("Level room width is not 8, it is %d", on_disk_level.rooms.data[0].width);
		return 0;
	}

	free_level(&on_disk_level);

	return 1;
}

INIT_TESTING(test_file_system)

RUN_TEST(test_room_init)
RUN_TEST(test_layer_init)
RUN_TEST(test_save_level)
RUN_TEST(test_load_level)

FINISH_TESTING()