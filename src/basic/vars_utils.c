#include "include/vars_utils.h"
// #include "include/block_properties.h"
// #include "include/endianless.h"
#include "include/vars.h"

#include <float.h>
#include <stdlib.h>

u8 get_var_type(char *type_str)
{
    const int len = strlen(type_str);

    if (len <= 1)
        return T_UNKNOWN;

    if (strcmp(type_str, "str") == 0)
        return T_STR;
    const char sign = type_str[0];

    type_str++;

    const int bits = atoi(type_str);

    u8 type_final = T_UNKNOWN;

    switch (bits)
    {
    case 8:
        type_final = T_U8;
        break;
    case 16:
        type_final = T_U16;
        break;
    case 32:
        type_final = T_U32;
        break;
    case 64:
        type_final = T_U64;
        break;
    }

    if (sign == 'I' || sign == 'i')
        type_final += 4;

    if (sign != 'U' && sign != 'u' && sign != 'I' && sign != 'i')
        return T_UNKNOWN;

    return type_final;
}

u8 var_type_length(u8 type)
{
    switch (type)
    {
    case T_U8:
    case T_I8:
        return 1;
    case T_U16:
    case T_I16:
        return 2;
    case T_U32:
    case T_I32:
        return 4;
    case T_U64:
    case T_I64:
        return 8;
    }
    return 0;
}

#define SWITCH_SETTER

u8 var_add_from_string(blob *b, const u8 type, const char letter, const char *token)
{
    u8 length = var_type_length(type);

    if (length == 0)
        return FAIL;

    i64 digit = atoll(token);

    // LOG_DEBUG("got digit %d", digit);

    if (var_add(b, letter, length) != SUCCESS)
        return FAIL;

    void *ptr = var_offset(*b, letter);

    switch (type)
    {
    case T_U8:
        *(u8 *)ptr = digit;
        break;
    case T_I8:
        *(i8 *)ptr = digit;
        break;
    case T_U16:
        *(u16 *)ptr = digit;
        break;
    case T_I16:
        *(i16 *)ptr = digit;
        break;
    case T_U32:
        *(u32 *)ptr = digit;
        break;
    case T_I32:
        *(i32 *)ptr = digit;
        break;
    case T_U64:
        *(u64 *)ptr = digit;
        break;
    case T_I64:
        *(i64 *)ptr = digit;
        break;
    }

    return SUCCESS;
}

/*
format:

{ <character> <type> = <value>
  <character> <type> = <value>
  ...
  <character> <type> = <value> }

one-liners work better

acceptable types: u8, u16, u32, u64
    i8 and such also supported
    str for strings

*/

u8 vars_parse(const char *str_to_cpy, blob *b)
{
    assert(str_to_cpy);
    assert(b);

    vars_free(b);

    char letter;
    u8 length = 0;
    u8 type = 0;

    bool wrote_stuff = false;

    char *str = strdup(str_to_cpy);

    assert(str);
    strcpy(str, str_to_cpy);

    char *token = strtok(str, " \n"); // consume start token
    if (strcmp(token, "{") != 0)
        goto exit;

    while (1)
    {
        token = strtok(NULL, " \n");
    type_is_here:

        if (!token || strcmp(token, "}") == 0)
        {
            if (!wrote_stuff)
                LOG_WARNING("empty vars: %d", str_to_cpy);
            break;
        }

        type = get_var_type(token); // record the type

        // LOG_DEBUG("got type %d - %s", type, token);

        token = strtok(NULL, " \n");

        if (!token)
        {
            LOG_ERROR("Invalid format: expected a letter after type: %s", str_to_cpy);
            goto exit;
        }

        letter = token[0]; // record the letter

        // LOG_DEBUG("got letter %c - %s", letter, token);

        if (type == T_STR || type == T_UNKNOWN)
        {
            token = strtok(NULL, " \n"); // hop to the equal sign

            if (!token || *token != '=')
            {
                LOG_ERROR("No equal sign. Current state: letter %c, type %d", letter, type);
                goto exit;
            }

            char *quote_begin = strchr(token + 2, '\"');
            if (!quote_begin)
            {
                LOG_ERROR("No quote were found for var \'%c\'", letter);
                goto exit;
            }

            quote_begin++;

            char *end_quote = strchr(quote_begin, '\"');
            if (!end_quote)
            {
                LOG_ERROR("No ending quote were found for var \'%c\'", letter);
                goto exit;
            }

            length = end_quote ? (end_quote - quote_begin) : strlen(quote_begin);

            if (length == 0)
            {
                LOG_WARNING("skipping 0 sized variable \'%c\'", letter);
            }
            else
            {
                if (var_add(b, letter, length) != SUCCESS)
                {
                    LOG_ERROR("Failed to add a variable \'%c\'", letter);
                    goto exit;
                }

                char *dest_str = var_offset(*b, letter);
                memcpy(dest_str, quote_begin, length);

                // LOG_DEBUG("got str %.*s", length, dest_str);
            }

            // run strtok until jumped over quote end
            do
            {
                token = strtok(NULL, " \n");
                if (!token)
                {
                    LOG_ERROR("It seems to be some kind of mistake... var \'%c\'", letter);
                    goto exit;
                }
            } while (token <= end_quote);
            goto type_is_here;
        }
        else
        {
            token = strtok(NULL, " =\n"); // value

            if (!token)
                goto exit;

            var_add_from_string(b, type, letter, token);
        }

        wrote_stuff = true;
    }

    free(str);

    return SUCCESS;

exit:

    free(str);

    return FAIL;
}