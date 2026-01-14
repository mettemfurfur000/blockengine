#include "include/block_registry.h"
#include "include/file_system.h"
#include "include/folder_structure.h"
#include "include/general.h"
#include "include/logging.h"
#include "include/opengl_stuff.h"
#include "include/scripting.h"
#include "include/sdl2_basics.h"

#include <stdlib.h>
#include <sys/stat.h>

void block_resource_write(block_resources res, block_registry *b, stream_t *s)
{
    LOG_DEBUG("Saving block id %llu", res.id);
    WRITE(res.id, s);
    blob_vars_write(res.vars_sample, s);

    WRITE(res.repeat_times, s);
    // write repeat skip
    WRITE_VEC(res.repeat_skip, j, val, { WRITE(val, s); }, s);

    // write repeat increment
    WRITE_VEC(
        res.repeat_increment, j, str,
        {
            blob str_blob = blobify(str);
            blob_write(str_blob, s);
        },
        s);

    // input names
    WRITE_VEC(
        res.input_names, j, str,
        {
            blob str_blob = blobify(str);
            blob_write(str_blob, s);
        },
        s);

    // render info
    WRITE(res.autotile_type, s);
    WRITE(res.interp_takes, s);
    WRITE(res.override_frame, s);
    WRITE(res.frames_per_second, s);
    WRITE(res.flags, s);
    // render controllers

    WRITE(res.anim_controller, s);
    WRITE(res.type_controller, s);
    WRITE(res.flip_controller, s);
    WRITE(res.rotation_controller, s);
    WRITE(res.offset_x_controller, s);
    WRITE(res.offset_y_controller, s);
    WRITE(res.interp_timestamp_controller, s);

    // atlas info
    {
        WRITE(res.info.atlas_offset_x, s);
        WRITE(res.info.atlas_offset_y, s);

        WRITE(res.info.width, s);
        WRITE(res.info.height, s);
        WRITE(res.info.frames, s);
        WRITE(res.info.types, s);
        WRITE(res.info.total_frames, s);
    }

    // image embed
    WRITE_OPTIONAL(
        res.img != NULL,
        {
            blob texture_name = blobify(res.texture_filename);
            blob_write(texture_name, s);
            // write image data
            image *img = res.img;
            u32 width = img->width;
            u32 height = img->height;
            u32 img_size = width * height * CHANNELS;

            WRITE(width, s);
            WRITE(height, s);
            WRITE_N(img->data, s, img_size);
        },
        s)

    // sounds
    WRITE_VEC(
        res.sounds, j, sound,
        {
            LOG_DEBUG("Saving sound %s for block id %llu", sound.filename, res.id);
            blob_write(blobify(sound.filename), s); // name of the sound file
            LOG_DEBUG("Sound length ms: %u", sound.length_ms);
            WRITE(sound.length_ms, s); // length in ms
            // find the sound file path
            char sound_full_path[MAX_PATH_LENGTH] = {};
            PATH_SOUND_MAKE(sound_full_path, b->name, sound.filename);
            stream_embed_file_write(sound_full_path, s);
        },
        s);

    // script file
    WRITE_OPTIONAL(
        res.lua_script_filename != NULL,
        {
            /* write the short script filename so we keep filename info */
            blob script_name_blob = blobify(res.lua_script_filename);
            blob_write(script_name_blob, s);

            /* compile the source file into lua bytecode and embed it */
            unsigned char *bytecode = NULL;
            u32 bytecode_size = 0;
            if (scripting_compile_file_to_bytecode(b->name, res.lua_script_filename, &bytecode, &bytecode_size) !=
                SUCCESS)
            {
                LOG_ERROR("Failed to compile lua script %s for registry %s", res.lua_script_filename, b->name);
                bytecode_size = 0;
            }

            WRITE(bytecode_size, s);
            if (bytecode_size > 0 && bytecode != NULL)
            {
                WRITE_N(bytecode, s, bytecode_size);
                free(bytecode);
            }
        },
        s)

    // all fields written
    write_hashtable(res.all_fields, s);
}

block_resources block_resource_read(block_registry *b, stream_t *s)
{
    block_resources res = {};
    READ(res.id, s);
    LOG_DEBUG("Loading block id %llu", res.id);
    res.vars_sample = blob_vars_read(s);
    // repeat info
    READ(res.repeat_times, s);
    // read repeat skip
    i32 val = 0;
    READ_VEC(res.repeat_skip, j, val, { READ(val, s); }, s);

    // read repeat increment
    char *str = NULL;
    READ_VEC(
        res.repeat_increment, j, str,
        {
            blob str_blob = blob_read(s);
            str = str_blob.str;
        },
        s);

    // input names
    READ_VEC(
        res.input_names, j, str,
        {
            blob str_blob = blob_read(s);
            str = str_blob.str;
        },
        s);

    // render info
    READ(res.autotile_type, s);
    READ(res.interp_takes, s);
    READ(res.override_frame, s);
    READ(res.frames_per_second, s);
    READ(res.flags, s);
    // render controllers

    READ(res.anim_controller, s);
    READ(res.type_controller, s);
    READ(res.flip_controller, s);
    READ(res.rotation_controller, s);
    READ(res.offset_x_controller, s);
    READ(res.offset_y_controller, s);
    READ(res.interp_timestamp_controller, s);

    // atlas info
    {
        READ(res.info.atlas_offset_x, s);
        READ(res.info.atlas_offset_y, s);

        READ(res.info.width, s);
        READ(res.info.height, s);
        READ(res.info.frames, s);
        READ(res.info.types, s);
        READ(res.info.total_frames, s);
    }

    // image embed
    res.img = NULL;

    READ_OPTIONAL(
        {
            blob texture_name = blob_read(s);
            res.texture_filename = texture_name.str;
            // read image data
            u32 width = 0;
            u32 height = 0;
            READ(width, s);
            READ(height, s);
            image *img = image_create(width, height);
            u32 img_size = width * height * CHANNELS;
            READ_N(img->data, s, img_size);
            res.img = img;
        },
        s)

    // sounds
    sound sound;
    READ_VEC(
        res.sounds, j, sound,
        {
            sound.filename = blob_read(s).str; // name of the sound file
            LOG_DEBUG("Read sound %s for block id %llu", sound.filename, res.id);
            READ(sound.length_ms, s); // length in ms
            LOG_DEBUG("Sound length ms: %u", sound.length_ms);

            // read the sound file data from the stream into memory and load via SDL
            u8 *sound_data = NULL;
            u32 sound_size = 0;
            stream_embed_file_read_to_mem(s, &sound_data, &sound_size);
            sound.obj = Mix_LoadWAV_RW(SDL_RWFromMem(sound_data, sound_size), 1);

            if (sound.obj == NULL)
            {
                LOG_ERROR("Couldn't load sound \"%s\" from memory, SDL error: \"%s\"", sound.filename, SDL_GetError());

                abort();
            }

            SAFE_FREE(sound_data);
        },
        s);

    // pointers to scripts
    READ_OPTIONAL(
        {
            blob name_blob = blob_read(s);
            res.lua_script_filename = name_blob.str;

            u32 bytecode_size = 0;
            READ(bytecode_size, s);
            if (bytecode_size > 0)
            {
                res.lua_script_blob = malloc(bytecode_size);
                res.lua_script_blob_size = bytecode_size;
                READ_N(res.lua_script_blob, s, bytecode_size);
            }
        },
        s);
    
    // all fields
    res.all_fields = alloc_table();
    read_hashtable(res.all_fields, s);

    res.parent_registry = b;

    return res;
}

u8 registry_save(block_registry *b)
{
    char path[256] = {};
    sprintf(path, FOLDER_REG SEPARATOR_STR "%s.brg", b->name); // block registry file

    stream_t s;
    if (stream_open_write(path, COMPRESS_LEVEL, &s) != 0)
        return FAIL;

    const char magic[4] = REGISTRY_VERSION_MAGIC;

    WRITE(magic, &s);
    blob_write(blobify(b->name), &s);

    // write the atlas png
    if (b->atlas != NULL)
    {
        image *img = b->atlas;
        u32 img_size = img->width * img->height * CHANNELS;
        // WRITE(img_size, s);
        u32 width = img->width;
        u32 height = img->height;
        WRITE(width, &s);
        WRITE(height, &s);
        WRITE_N(img->data, &s, img_size);
    }
    else
    {
        LOG_WARNING("Atlas is NULL for registry %s, writing empty atlas", b->name);
        u32 img_size = 0;
        WRITE(img_size, &s); // w
        WRITE(img_size, &s); // h
    }

    // compile init script
    const char *init_script_name = "init.lua";

    u8 *bytecode = NULL;
    u32 bytecode_size = 0;
    if (scripting_compile_file_to_bytecode(b->name, init_script_name, &bytecode, &bytecode_size) != SUCCESS)
    {
        LOG_ERROR("Failed to compile init.lua for registry %s", b->name);
        bytecode_size = 0;
    }

    WRITE(bytecode_size, &s);
    if (bytecode_size > 0 && bytecode != NULL)
    {
        WRITE_N(bytecode, &s, bytecode_size);
        free(bytecode);
    }

    // write all block resources
    WRITE_VEC(b->resources, i, res, { block_resource_write(res, b, &s); }, &s);

    stream_close(&s);

    return SUCCESS;
}

// loading tips:
// will have to set parent_registry pointers after loading all registries
// will have to load images separately and set img pointers
// will have to load sound files separately and set sound.obj pointers

block_registry *registry_load(const char *name)
{
    assert(name != NULL);
    assert(strlen(name) > 0 && strlen(name) < 32);

    char path[256] = {};
    sprintf(path, FOLDER_REG SEPARATOR_STR "%s.brg", name); // block registry file

    stream_t s;
    if (stream_open_read(path, COMPRESS_LEVEL, &s) != 0)
    {
        LOG_ERROR("Failed to open registry file: %s", path);
        return NULL;
    }

    char magic[4] = {};
    READ(magic, &s);
    if (memcmp(magic, REGISTRY_VERSION_MAGIC, 4) != 0)
    {
        LOG_ERROR("Registry %s has invalid magic/version", name);
        stream_close(&s);
        return NULL;
    }

    blob reg_name_blob = blob_read(&s);
    block_registry *reg = calloc(1, sizeof(block_registry));
    assert(reg != NULL);
    reg->name = reg_name_blob.str;

    // read atlas png
    u32 width = 0;
    u32 height = 0;
    READ(width, &s);
    READ(height, &s);
    if (width > 0 && height > 0)
    {
        image *img = image_create(width, height);
        u32 img_size = width * height * CHANNELS;
        READ_N(img->data, &s, img_size);
        reg->atlas = img;
    }
    else
    {
        LOG_WARNING("Registry %s has empty atlas", name);
        reg->atlas = NULL;
    }

    // read init script
    u32 bytecode_size = 0;
    READ(bytecode_size, &s);
    if (bytecode_size > 0)
    {
        // we don't store the init script blob in the registry struct,
        // instead we load it directly when needed
        unsigned char *bytecode = malloc(bytecode_size);
        READ_N(bytecode, &s, bytecode_size);

        // load the init script into Lua
        // Also calls it immediately to initialize any global state
        if (scripting_load_compiled_blob(reg->name, "init.lua", bytecode, bytecode_size) != SUCCESS)
        {
            LOG_ERROR("Failed to load init script for registry %s", reg->name);
            abort();
            // free(bytecode);
            // stream_close(&s);
            // free_block_registry(reg);
            // return NULL;
        }

        free(bytecode);
    }

    // read all block resources
    block_resources res = {};
    READ_VEC(
        reg->resources, i, res,
        {
            LOG_DEBUG("Reading block resource %u for registry %s", i, name);
            res = block_resource_read(reg, &s);
        },
        &s)

    LOG_DEBUG("Finished reading %u block resources for registry %s", reg->resources.length, name);

    stream_close(&s);

    sort_by_id(&reg->resources);

    LOG_DEBUG("Loaded registry %s with %u blocks", name, reg->resources.length);

    // run all scripts to initialize any block-specific state, after init has run
    if (scripting_load_scripts(reg) != SUCCESS)
    {
        LOG_ERROR("Failed to load scripts for registry %s", name);
        return NULL;
    }

    reg->atlas_texture_uid = gl_bind_texture(reg->atlas);

    return reg;
}