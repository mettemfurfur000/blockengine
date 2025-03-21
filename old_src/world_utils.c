#include "include/world_utils.h"

// turns string into formatted block chain
void bprintf(const world *w, block_resources_t *reg, int layer, int orig_x, int orig_y, int length_limit, const char *format, ...)
{
    char buffer[1024] = {};
    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);

    char *ptr = buffer;
    int x = orig_x;
    int y = orig_y;

    while (*ptr != 0)
    {
        block b = {.id = 4};
        block *dest = get_block_access(w, layer, x, y);

        block_copy(dest, &reg->data[b.id].block_sample);
        data_create_element(&dest->data, 'v', 1);
        data_set_b(dest, 'v', *ptr);
        switch (*ptr)
        {
        case '\n':
            x = orig_x;
            y++;
            break;
        case '\t':
            x += 4;
            break;
        default:
            x++;
        }

        if (x > length_limit)
        {
            x = orig_x;
            y++;
        }

        ptr++;
    }

    va_end(args);
}