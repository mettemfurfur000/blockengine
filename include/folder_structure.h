#ifndef FOLDER_STRUCTURE_H
#define FOLDER_STRUCTURE_H 1

#define FOLDER_SHD "shaders"
#define FOLDER_SHD_FRAG_EXT "frag"
#define FOLDER_SHD_VERT_EXT "vert"
#define FOLDER_SHD_GEOM_EXT "geom"

#define FOLDER_LVL "levels"

#define FOLDER_REG "registries"
#define FOLDER_REG_TEX "textures"
#define FOLDER_REG_SCR "scripts"
#define FOLDER_REG_SND "sounds"

#define PATH_SOUND_MAKE(dest, reg_name, sound_name)                                                                       \
    snprintf(dest, MAX_PATH_LENGTH, FOLDER_REG SEPARATOR_STR "%s" SEPARATOR_STR FOLDER_REG_SND SEPARATOR_STR "%s",     \
             reg_name, sound_name);

#define PATH_SCRIPT_MAKE(dest, reg_name, short_filename)                                                               \
    snprintf(dest, MAX_PATH_LENGTH, FOLDER_REG SEPARATOR_STR "%s" SEPARATOR_STR FOLDER_REG_SCR SEPARATOR_STR "%s",     \
             reg_name, short_filename);

#endif