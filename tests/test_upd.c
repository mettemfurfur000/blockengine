#include "../src/include/block_operations.h"
#include "utils.h"

const char test_world_name[] = "test_world";
world *t_w = 0;

world *make_test_world()
{
	world *w = world_make(1, test_world_name);

	world_layer_alloc(&w->layers[0], 2, 2, 32, 0);

	return w;
}

void free_test_world(world *w)
{
	world_layer_free(&w->layers[0]);

	world_free(w);
}

void print_layer(world *w, int layer, int x, int y, int s_x, int s_y)
{
	for (int j = y; j < s_y; j++)
	{
		for (int i = x; i < s_x; i++)
			printf("[%c]", get_block_access(w, layer, i, j)->id % 96 + ' ');
		printf("\n");
	}
}

int test_block_set()
{
	int status = SUCCESS;

	block test_block = {0};
	block_set_random(&test_block);

	status &= set_block(t_w, 0, 0, 0, &test_block);

	block that_block = *get_block_access(t_w, 0, 0, 0);

	status &= is_block_equal(&that_block, &test_block);

	block_data_free(&test_block);

	return status;
}

int test_block_clean()
{
	int status = SUCCESS;

	const block void_block = {0, 0};

	world *t_w = make_test_world();

	block test_block = {0};
	block_set_random(&test_block);

	status &= set_block(t_w, 0, 0, 0, &test_block);

	status &= clean_block(t_w, 0, 0, 0);

	block that_block = *get_block_access(t_w, 0, 0, 0);

	status &= is_block_equal(&that_block, &void_block);

	block_data_free(&test_block);

	return status;
}

int test_block_move_gently()
{
	int status = SUCCESS;

	world *t_w = make_test_world();

	const block void_block = {0, 0};

	block test_block = {0};
	block other_block = {0};
	block_set_random(&test_block);
	block_set_random(&other_block);

	// this looks like shit, but its ok
	// imagine this configuration of blocks like this
	/*
		  x - >
		y [#][#][ ]
		| [ ][%][ ]
		v [ ][ ][ ]
	*/
	// at the end of the test they must be at this positions
	/*
		  x - >
		y [ ][ ][ ]
		| [#][#][ ]
		v [ ][%][ ]
	*/

	status &= set_block(t_w, 0, 0, 0, &test_block);
	status &= set_block(t_w, 0, 1, 0, &test_block);

	status &= set_block(t_w, 0, 1, 1, &other_block);

	// lets try gently move blocks

	status &= move_block_gently(t_w, 0, 0, 0, 0, 1);		 // must be ok, no block on the way == move
	status &= move_block_gently(t_w, 0, 1, 0, 0, 1) == FAIL; // must fail, block on the way == no move

	status &= move_block_gently(t_w, 0, 1, 1, 0, 1); // move blocking block and then our test block
	status &= move_block_gently(t_w, 0, 1, 0, 0, 1);

	block block00 = *get_block_access(t_w, 0, 0, 0);
	block block01 = *get_block_access(t_w, 0, 0, 1);

	block block10 = *get_block_access(t_w, 0, 1, 0);
	block block11 = *get_block_access(t_w, 0, 1, 1);
	block block12 = *get_block_access(t_w, 0, 1, 2);

	status &= is_block_equal(&block00, &void_block);
	status &= is_block_equal(&block01, &test_block);

	status &= is_block_equal(&block10, &void_block);
	status &= is_block_equal(&block11, &test_block);
	status &= is_block_equal(&block12, &other_block);

	// print_layer(t_w, 0, 0, 0, 4, 4);

	block_data_free(&test_block);
	block_data_free(&other_block);

	return status;
}

int test_block_move_rough()
{
	int status = SUCCESS;

	world *t_w = make_test_world();

	const block void_block = {0, 0};

	block test_block = {0};
	block other_block = {0};
	block_set_random(&test_block);
	block_set_random(&other_block);

	// imagine this configuration of blocks like this
	/*
		  x - >
		y [#][#][ ]
		| [ ][%][ ]
		v [ ][ ][ ]
	*/
	// at the end of the test they must be at this positions
	/*
		  x - >
		y [ ][ ][ ]
		| [#][#][ ]
		v [ ][ ][ ]
	*/

	status &= set_block(t_w, 0, 0, 0, &test_block);
	status &= set_block(t_w, 0, 1, 0, &test_block);

	status &= set_block(t_w, 0, 1, 1, &other_block);

	status &= move_block_rough(t_w, 0, 0, 0, 0, 1); // must be ok anyway
	status &= move_block_rough(t_w, 0, 1, 0, 0, 1);

	block block00 = *get_block_access(t_w, 0, 0, 0);
	block block01 = *get_block_access(t_w, 0, 0, 1);

	block block10 = *get_block_access(t_w, 0, 1, 0);
	block block11 = *get_block_access(t_w, 0, 1, 1);

	status &= is_block_equal(&block00, &void_block);
	status &= is_block_equal(&block01, &test_block);

	status &= is_block_equal(&block10, &void_block);
	status &= is_block_equal(&block11, &test_block);

	// print_layer(t_w, 0, 0, 0, 4, 4);

	block_data_free(&test_block);
	block_data_free(&other_block);

	return status;
}

int test_block_move_recursive()
{
	int status = SUCCESS;

	world *t_w = make_test_world();

	const block void_block = {0, 0};

	block test_block = {0};
	block other_block = {0};
	block_set_random(&test_block);
	block_set_random(&other_block);

	// imagine this configuration of blocks like this
	/*
		  x - >
		y [#][#][#][%]
		| [ ][%][%][%]
		v [ ][ ][#][%]
		  [ ][ ][ ][%]
	*/
	// at the end of the test they must be at this positions
	/*
		  x - >
		y [ ][ ][ ][%]
		| [#][#][#][%]
		v [ ][%][%][%]
		  [ ][ ][#][%]
	*/

	status &= set_block(t_w, 0, 0, 0, &test_block);

	status &= set_block(t_w, 0, 1, 0, &test_block);
	status &= set_block(t_w, 0, 1, 1, &other_block);

	status &= set_block(t_w, 0, 2, 0, &test_block);
	status &= set_block(t_w, 0, 2, 1, &other_block);
	status &= set_block(t_w, 0, 2, 2, &test_block);

	for (int i = 0; i < 4; i++)
		status &= set_block(t_w, 0, 3, i, &other_block);

	// printf("before pushing:\n");
	// print_layer(t_w, 0, 0, 0, 4, 4);

	status &= move_block_recursive(t_w, 0, 0, 0, 0, 1, 3);
	status &= move_block_recursive(t_w, 0, 1, 0, 0, 1, 3);
	status &= move_block_recursive(t_w, 0, 2, 0, 0, 1, 3);
	status &= move_block_recursive(t_w, 0, 3, 0, 0, 1, 3) == FAIL; // must fail, max 2 blocks ahead to push

	for (int i = 0; i < 3; i++)
		status &= is_block_equal(get_block_access(t_w, 0, i, 0), &void_block);
	status &= is_block_equal(get_block_access(t_w, 0, 3, 0), &other_block);

	// printf("after:\n");
	// print_layer(t_w, 0, 0, 0, 4, 4);

	block_data_free(&test_block);
	block_data_free(&other_block);

	return status;
}

int test_block_operations_all()
{
	printf("test_block_operations_all:\n");
	t_w = make_test_world();

	RUN_TEST(test_block_set)
	RUN_TEST(test_block_clean)
	RUN_TEST(test_block_move_gently)
	RUN_TEST(test_block_move_rough)
	RUN_TEST(test_block_move_recursive)

	free_test_world(t_w);
	return SUCCESS;
}
