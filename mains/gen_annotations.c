#include "include/gen_annotations.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(const char *program_name)
{
    printf("Usage: %s -o <output> <input-files...>\n\n", program_name);
    printf("Scans C source files for @lua annotation markers, reads luaL_Reg tables,\n");
    printf("and generates EmmyLua annotation files.\n\n");
    printf("Marker comments:\n");
    printf("  // @lua class: ClassName    — generates ---@class with @field entries\n");
    printf("  // @lua lib: name           — generates function wrappers\n\n");
    printf("Function signature comments (before each table entry):\n");
    printf("  // (x:integer, y:integer) -> boolean\n\n");
    printf("Options:\n");
    printf("  -o <path>   Output file path (required)\n");
    printf("  --help      Show this help message\n");
}

int main(int argc, char *argv[])
{
    const char *output_path = NULL;

    static struct option long_options[] = {
        {"output", required_argument, 0, 'o'},
        {   "help",       no_argument, 0, 'H'},
        {        0,                 0, 0,   0}
    };

    int option_index = 0;
    int c;

    /* collect input files */
    const char **input_files = NULL;
    u32          file_count  = 0;
    u32          file_cap    = 0;

    while ((c = getopt_long(argc, argv, "o:H", long_options, &option_index)) != -1)
    {
        switch (c)
        {
        case 'o':
            output_path = optarg;
            break;
        case 'H':
            print_usage(argv[0]);
            return 0;
        case '?':
            fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
            return 1;
        }
    }

    /* remaining args are input files */
    for (int i = optind; i < argc; i++)
    {
        if (file_count >= file_cap)
        {
            u32 new_cap = file_cap ? file_cap * 2 : 16;
            const char **tmp = (const char **)realloc((void *)input_files, new_cap * sizeof(char *));
            if (!tmp)
            {
                fprintf(stderr, "error: out of memory\n");
                free((void *)input_files);
                return 1;
            }
            input_files = tmp;
            file_cap    = new_cap;
        }
        input_files[file_count++] = argv[i];
    }

    if (!output_path)
    {
        fprintf(stderr, "error: no output path specified (use -o <path>)\n");
        print_usage(argv[0]);
        free((void *)input_files);
        return 1;
    }

    if (file_count == 0)
    {
        fprintf(stderr, "error: no input files specified\n");
        print_usage(argv[0]);
        free((void *)input_files);
        return 1;
    }

    int ret = gen_annotations(input_files, file_count, output_path);

    free((void *)input_files);
    return ret == SUCCESS ? 0 : 1;
}
