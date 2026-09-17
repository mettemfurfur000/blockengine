#include "include/block_registry.h"
#include "include/logging.h"
#include "include/scripting.h"
#include "include/sdl2_basics.h"
#include "include/signal_handler.h"

#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct
{
	const char *log_output; // "stdout" or filename (default: "client.log")
	const char *registry; // Registry path (default: "engine")
} ClientConfig;

static void print_usage(const char *program_name)
{
	printf("Usage: %s [OPTIONS]\n\n", program_name);
	printf("Options:\n");
	printf("  -r, --registry <path>     Registry path (default: engine)\n");
	printf("  --help                    Show this help message\n");
}

static ClientConfig parse_arguments(int argc, char *argv[])
{
	ClientConfig config = {
		.log_output = "default",
		.registry = "engine",
	};

	static struct option long_options[] = {
		{"registry", required_argument, 0, 'r'},
		 {	  "help",		  no_argument, 0, 'H'},
		{		 0,					0, 0,	  0}
	};

	int option_index = 0;
	int c;

	while ((c = getopt_long(argc, argv, "r:H", long_options, &option_index)) != -1)
	{
		switch (c)
		{
		case 'r':
			config.registry = optarg;
			break;
		case 'H':
			print_usage(argv[0]);
			exit(0);
		case '?':
			fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
			exit(1);
		default:
			break;
		}
	}

	return config;
}

void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
								const GLchar *message, const void *userParam)
{
	if (type != 0x8251)
		LOG_ERROR("GL: %s type = 0x%x, severity = 0x%x, message = %s",
				  (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, severity, message);
}

int main(int argc, char *argv[])
{
	ClientConfig config = parse_arguments(argc, argv);

	if (strcmp(config.log_output, "default") == 0)
		log_start_new("builder");
	else
		log_start(config.log_output);

	init_backtrace();
	init_signal_handlers();

	if (init_sound() != SUCCESS)
	    return 1;

	// glEnable(GL_DEBUG_OUTPUT);
	// glDebugMessageCallback(MessageCallback, 0);

	scripting_init();

	// signal to traits/scripts that we are compiling the registry headless
	scripting_set_builder_mode(1);

	block_registry r = {};

	read_block_registry(&r, config.registry);
	registry_save(&r);

	scripting_set_builder_mode(0);

	deinit_signal_handlers();
	deinit_backtrace();

	// exit_graphics();
	scripting_close();

	return 0;
}