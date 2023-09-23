#ifndef FILE_SYSTEM_FUNCTIONS_H
#define FILE_SYSTEM_FUNCTIONS_H 1

#include "memory_control_functions.c"
#include "endianless.c"
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

struct stat st = {0};

int make_full_chunk_path(char filename[256], world *w, int index, int x, int y)
{
	char working_path[256] = {0};
	char folder_path[512] = {0};
	char buffer[256];

	if (!getcwd(working_path, 256))
		return FAIL;

	memset(filename, 0, 256);

	sprintf(folder_path, "%s/world_%s", working_path, w->worldname);

	if (stat(folder_path, &st) == -1)
		mkdir(folder_path, 0700);

	sprintf(buffer, "/layer_%d", index);
	strcat(folder_path, buffer);

	if (stat(folder_path, &st) == -1)
		mkdir(folder_path, 0700);

	sprintf(buffer, "/chunk_%d_%d.bin", x, y);
	strcat(folder_path, buffer);
	strcpy(filename, folder_path);

	return SUCCESS;
}

int make_full_layer_path(char filename[256], world *w, int index)
{
	char working_path[256] = {0};
	char folder_path[512] = {0};
	char buffer[256];

	if (!getcwd(working_path, 256))
		return FAIL;

	memset(filename, 0, 256);
	sprintf(folder_path, "%s/world_%s", working_path, w->worldname);
	if (stat(folder_path, &st) == -1)
		mkdir(folder_path, 0700);

	sprintf(buffer, "/layer_%d", index);
	strcat(folder_path, buffer);
	if (stat(folder_path, &st) == -1)
		mkdir(folder_path, 0700);

	sprintf(buffer, "/layer_info.bin");
	strcat(folder_path, buffer);
	strcpy(filename, folder_path);

	return SUCCESS;
}

int make_full_world_path(char filename[256], world *w)
{
	char working_path[256] = {0};
	char folder_path[512] = {0};
	char buffer[256];

	if (!getcwd(working_path, 256))
		return FAIL;

	memset(filename, 0, 256);

	sprintf(folder_path, "%s/world_%s", working_path, w->worldname);

	if (stat(folder_path, &st) == -1)
		mkdir(folder_path, 0700);

	sprintf(buffer, "/world_info.bin");
	strcat(folder_path, buffer);

	strcpy(filename, folder_path);
	return SUCCESS;
}

int fwrite_endianless(void *data, const int size, FILE *f)
{
	byte buffer[size];
	memcpy(&buffer, data, size);
	make_endianless(buffer, size);
	return fwrite(buffer, 1, size, f) == size;
}

int fread_endianless(void *data, const int size, FILE *f)
{
	byte buffer[size];
	const int readed = fread(buffer, 1, size, f);
	make_endianless(buffer, size); // it also can undo endianless tricks
	memcpy(data, &buffer, size);
	return readed == size;
}

/* write/read functions */
void write_block(block *b, FILE *f)
{
	int size = 1;

	if (!fwrite_endianless((byte *)&b->id, sizeof(b->id), f))
	{
		fputc(0xEE, f);
		fputc(0xEE, f);
		fputc(0xEE, f);
		fputc(0xEE, f);
	}

	if (b->data)
	{
		size = b->data[0] + 1;
		fwrite(b->data, sizeof(byte), size, f);
		return;
	}

	fputc(0, f);
}

void read_block(block *b, FILE *f)
{
	block_data_free(b);

	if (!fread_endianless((byte *)&b->id, sizeof(b->id), f))
		b->id = 0xEEEEEEEE;

	byte size;
	if (!fread(&size, sizeof(size), 1, f))
	{
		b->data = 0;
		return;
	}

	if (size)
	{
		b->data = (byte *)calloc(size + 1, sizeof(byte));
		b->data[0] = size;
		fread(b->data + 1, sizeof(byte), size, f);
		return;
	}
}

int write_chunk(layer_chunk *c, const char *filename)
{
	if (!c)
		return FAIL;

	FILE *f = fopen(filename, "wb");
	if (!f)
		return FAIL;

	fwrite_endianless(&c->width, sizeof(int), f);

	for (int i = 0; i < c->width; i++)
	{
		for (int j = 0; j < c->width; j++)
		{
			write_block(&c->blocks[i][j], f);
		}
	}

	fclose(f);
	return SUCCESS;
}

int read_chunk(layer_chunk *c, const char *filename)
{
	if (!c)
		return FAIL;

	FILE *f = fopen(filename, "rb");

	if (!f)
		return FAIL;

	chunk_free(c);

	fread_endianless(&c->width, sizeof(int), f);

	chunk_alloc(c, c->width);

	for (int i = 0; i < c->width; i++)
	{
		for (int j = 0; j < c->width; j++)
		{
			read_block(&c->blocks[i][j], f);
		}
	}
	fclose(f);
	return SUCCESS;
}

int write_layer(world_layer *wl, const char *filename)
{
	if (!wl)
		return FAIL;
	FILE *f = fopen(filename, "wb");
	if (!f)
		return FAIL;

	fwrite(&wl->index, sizeof(char), 1, f);
	fwrite_endianless(&wl->chunk_width, sizeof(int), f);
	fwrite_endianless(&wl->size_x, sizeof(int), f);
	fwrite_endianless(&wl->size_y, sizeof(int), f);

	fclose(f);
	return SUCCESS;
}

int read_layer(world_layer *wl, const char *filename)
{
	if (!wl)
		return FAIL;

	FILE *f = fopen(filename, "rb");
	if (!f)
		return FAIL;

	fread(&wl->index, sizeof(char), 1, f);
	fread_endianless(&wl->chunk_width, sizeof(int), f);
	fread_endianless(&wl->size_x, sizeof(int), f);
	fread_endianless(&wl->size_y, sizeof(int), f);

	fclose(f);
	return SUCCESS;
}

int write_world(world *w, const char *filename)
{
	if (!w)
		return FAIL;

	FILE *f = fopen(filename, "wb");

	if (!f)
		return FAIL;

	fwrite_endianless(&w->depth, sizeof(int), f);
	fwrite(&w->worldname, sizeof(char), 64, f);

	fclose(f);
	return SUCCESS;
}

int read_world(world *w, const char *filename)
{
	if (!w)
		return FAIL;

	FILE *f = fopen(filename, "rb");
	if (!f)
		return FAIL;

	fread_endianless(&w->depth, sizeof(int), f);
	fread(&w->worldname, sizeof(char), 64, f);

	fclose(f);
	return SUCCESS;
}
/*load/save fucntions*/
// unload means save to file and then free memory

int chunk_save(world *w, int index, int chunk_x, int chunk_y, layer_chunk *c)
{
	char filename[256];
	make_full_chunk_path(filename, w, index, chunk_x, chunk_y);
	return write_chunk(c, filename);
}

int chunk_load(world *w, int index, int chunk_x, int chunk_y, layer_chunk *c)
{
	char filename[256];
	make_full_chunk_path(filename, w, index, chunk_x, chunk_y);
	return read_chunk(c, filename);
}

int chunk_unload(world *w, int index, int chunk_x, int chunk_y, layer_chunk *c)
{
	int ret;
	char filename[256];

	make_full_chunk_path(filename, w, index, chunk_x, chunk_y);
	ret = write_chunk(c, filename);

	if (ret)
		chunk_free(c);
	return ret;
}

void layer_save(world *w, world_layer *wl)
{
	char filename[256];
	make_full_layer_path(filename, w, wl->index);
	write_layer(wl, filename);

	for (int i = 0; i < wl->size_x; i++)
	{
		for (int j = 0; j < wl->size_y; j++)
		{
			if (wl->chunks[i][j])
				chunk_save(w, wl->index, i, j, wl->chunks[i][j]);
		}
	}
}

void layer_load(world *w, world_layer *wl, int index)
{
	char filename[256];
	make_full_layer_path(filename, w, index);
	read_layer(wl, filename);

	world_layer_alloc(wl, wl->size_x, wl->size_y, wl->chunk_width, index);

	for (int i = 0; i < wl->size_x; i++)
	{
		for (int j = 0; j < wl->size_y; j++)
		{
			chunk_load(w, wl->index, i, j, wl->chunks[i][j]);
		}
	}
}

void layer_unload(world *w, world_layer *wl)
{
	char filename[256];
	make_full_layer_path(filename, w, wl->index);
	write_layer(wl, filename);

	for (int i = 0; i < wl->size_x; i++)
	{
		for (int j = 0; j < wl->size_y; j++)
		{
			if (wl->chunks[i][j])
				chunk_unload(w, wl->index, i, j, wl->chunks[i][j]);
		}
	}

	world_layer_free(wl);
}

void world_save(world *w)
{
	char filename[256];
	make_full_world_path(filename, w);
	write_world(w, filename);

	for (int i = 0; i < w->depth; i++)
	{
		layer_save(w, &w->layers[i]);
	}
}

void world_load(world *w, char *worldname)
{
	char filename[256];
	make_full_world_path(filename, w);
	read_world(w, filename);

	for (int i = 0; i < w->depth; i++)
	{
		layer_load(w, &w->layers[i], i);
	}
}

void world_unload(world *w)
{
	char filename[256];
	make_full_world_path(filename, w);
	write_world(w, filename);

	for (int i = 0; i < w->depth; i++)
	{
		layer_unload(w, &w->layers[i]);
	}
}

#endif