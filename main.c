#include "src/block_updates.c"
#include "src/block_registry.c"

int main(int argc, char *argv[])
{
	if (!init_graphics())
		return 1;
	
	block_registry_t b_reg;
	vec_init(&b_reg);

	if(!read_block_registry("resources/blocks", &b_reg))
		goto fire_exit;
	
	sort_by_id(&b_reg);

	unsigned long frame = 0;

	while (handle_events())
	{
		SDL_RenderClear(g_renderer);

		SDL_RenderPresent(g_renderer);
		SDL_Delay(16);

		frame++;
	}
fire_exit:
	exit_graphics();

	return 0;
}