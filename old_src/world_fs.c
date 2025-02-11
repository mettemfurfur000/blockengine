#include "include/world_fs.h"

#ifdef _WIN64 // why not
const char *separator = "\\";
#else
const char *separator = "/";
#endif

const char *worlds_folder = "worlds";
const char *layer_folder_template = "layer_%d";
const char *chunk_folder_template = "chunk_%d_%d.bin";
const char *world_info_str = "world_info.bin";
const char *layer_info_str = "layer_info.bin";

int make_full_world_path(char *filename, const world *w)
{
	struct stat st = {0};
	char working_path[256] = {0};
	char folder_path[512] = {0};

	if (!getcwd(working_path, 256))
		return FAIL;

	strcat(folder_path, working_path); // assemble worlds folder path
	strcat(folder_path, separator);
	strcat(folder_path, worlds_folder);

	if (stat(folder_path, &st) == -1)
		X_P_mkdir(folder_path);

	strcat(folder_path, separator);
	strcat(folder_path, w->worldname);

	if (stat(folder_path, &st) == -1)
		X_P_mkdir(folder_path);

	strcat(folder_path, separator);
	strcat(folder_path, world_info_str);

	strcpy(filename, folder_path);
	return SUCCESS;
}

int make_full_layer_path(char *filename, const world *w, int index)
{
	struct stat st = {0};
	char working_path[256] = {0};
	char folder_path[512] = {0};
	char buffer[256];

	if (!getcwd(working_path, 256))
		return FAIL;

	strcat(folder_path, working_path); // assemble worlds folder path
	strcat(folder_path, separator);
	strcat(folder_path, worlds_folder);

	if (stat(folder_path, &st) == -1)
		X_P_mkdir(folder_path);

	strcat(folder_path, separator);
	strcat(folder_path, w->worldname);

	if (stat(folder_path, &st) == -1)
		X_P_mkdir(folder_path);

	strcat(folder_path, separator);
	sprintf(buffer, layer_folder_template, index); // create layer folder path
	strcat(folder_path, buffer);

	if (stat(folder_path, &st) == -1)
		X_P_mkdir(folder_path);

	strcat(folder_path, separator);
	strcat(folder_path, layer_info_str);
	strcpy(filename, folder_path);

	return SUCCESS;
}

int make_full_chunk_path(char *filename, const world *w, int index, int x, int y)
{
	struct stat st = {0};
	char working_path[256] = {0};
	char folder_path[512] = {0};
	char buffer[256];

	if (!getcwd(working_path, 256))
		return FAIL;

	strcat(folder_path, working_path); // assemble worlds folder path
	strcat(folder_path, separator);
	strcat(folder_path, worlds_folder);

	if (stat(folder_path, &st) == -1) // if not created yet, create
		X_P_mkdir(folder_path);

	strcat(folder_path, separator); // assemble world folder path
	strcat(folder_path, w->worldname);

	if (stat(folder_path, &st) == -1) // if not created yet, create
		X_P_mkdir(folder_path);

	strcat(folder_path, separator);
	sprintf(buffer, layer_folder_template, index); // create layer folder path
	strcat(folder_path, buffer);

	if (stat(folder_path, &st) == -1) // create if not exist
		X_P_mkdir(folder_path);

	strcat(folder_path, separator);
	sprintf(buffer, chunk_folder_template, x, y); // make chunk path
	strcat(folder_path, buffer);

	strcpy(filename, folder_path);

	return SUCCESS;
}

int fwrite_endianless(const void *data, const int size, FILE *f)
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

	if (b->data) // write all data of the block
	{
		size = b->data[0] + 1;
		fwrite(b->data, sizeof(byte), size, f);
		return;
	}
	// or just 0 to mark that there is no data
	fputc(0, f);
}

void read_block(block *b, FILE *f)
{
	block_data_free(b);

	if (!fread_endianless((byte *)&b->id, sizeof(b->id), f))
		b->id = 0xEEEEEEEE;

	byte size;
	fread(&size, sizeof(size), 1, f); // read size byte

	if (!size)
		b->data = 0;
	else
	{
		b->data = (byte *)calloc(size + 1, sizeof(byte));
		b->data[0] = size;
		fread(b->data + 1, sizeof(byte), size, f);
		return;
	}
}

int write_chunk(const layer_chunk *c, const char *filename)
{
	if (!c)
		return FAIL;

	FILE *f = fopen(filename, "wb");
	if (!f)
		return FAIL;

	fwrite_endianless(&c->width, sizeof(c->width), f);

	FOREACH_BLOCK_FROM_CHUNK(c, blk, {
		write_block(blk, f);
	})

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

	fread_endianless(&c->width, sizeof(c->width), f);

	chunk_alloc(c, c->width);

	FOREACH_BLOCK_FROM_CHUNK(c, blk, {
		read_block(blk, f);
	})

	fclose(f);
	return SUCCESS;
}

int write_layer(const world_layer *wl, const char *filename)
{
	if (!wl)
		return FAIL;
	FILE *f = fopen(filename, "wb");
	if (!f)
		return FAIL;

	fwrite_endianless(&wl->index, sizeof(wl->index), f);
	fwrite_endianless(&wl->chunk_width, sizeof(wl->chunk_width), f);
	fwrite_endianless(&wl->size_x, sizeof(wl->size_x), f);
	fwrite_endianless(&wl->size_y, sizeof(wl->size_y), f);

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

	fread_endianless(&wl->index, sizeof(wl->index), f);
	fread_endianless(&wl->chunk_width, sizeof(wl->chunk_width), f);
	fread_endianless(&wl->size_x, sizeof(wl->size_x), f);
	fread_endianless(&wl->size_y, sizeof(wl->size_y), f);

	fclose(f);
	return SUCCESS;
}

int write_world(const world *w, const char *filename)
{
	if (!w)
		return FAIL;

	FILE *f = fopen(filename, "wb");

	if (!f)
		return FAIL;

	fwrite_endianless(&w->layers.length, sizeof(w->layers.length), f);
	fwrite(w->worldname, sizeof(char), sizeof(w->worldname), f);

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

	fread_endianless(&w->layers.length, sizeof(w->layers.length), f);
	fread(w->worldname, sizeof(char), sizeof(w->worldname), f);

	fclose(f);
	return SUCCESS;
}
/*load/save fucntions*/
// unload means save to file and then free memory

int chunk_save(const world *w, int index, int chunk_x, int chunk_y, const layer_chunk *c)
{
	char filename[256];
	make_full_chunk_path(filename, w, index, chunk_x, chunk_y);
	return write_chunk(c, filename);
}

int chunk_load(const world *w, int index, int chunk_x, int chunk_y, layer_chunk *c)
{
	char filename[256];
	make_full_chunk_path(filename, w, index, chunk_x, chunk_y);
	return read_chunk(c, filename);
}

int chunk_unload(const world *w, int index, int chunk_x, int chunk_y, layer_chunk *c)
{
	int ret;
	char filename[256];

	make_full_chunk_path(filename, w, index, chunk_x, chunk_y);
	ret = write_chunk(c, filename);

	if (ret)
		chunk_free(c);
	return ret;
}

void layer_save(const world *w, const world_layer *wl)
{
	char filename[256];
	make_full_layer_path(filename, w, wl->index);
	write_layer(wl, filename);

	for (int i = 0; i < wl->size_x; i++)
		for (int j = 0; j < wl->size_y; j++)
			if (wl->chunks[i][j])
				chunk_save(w, wl->index, i, j, wl->chunks[i][j]);
}

void layer_load(const world *w, world_layer *wl, int index)
{
	char filename[256];
	make_full_layer_path(filename, w, index);
	read_layer(wl, filename);

	world_layer_alloc(wl, wl->size_x, wl->size_y, wl->chunk_width, index);

	for (int i = 0; i < wl->size_x; i++)
		for (int j = 0; j < wl->size_y; j++)
			chunk_load(w, wl->index, i, j, wl->chunks[i][j]);
}

void layer_unload(world *w, world_layer *wl)
{
	char filename[256];
	make_full_layer_path(filename, w, wl->index);
	write_layer(wl, filename);

	for (int i = 0; i < wl->size_x; i++)
		for (int j = 0; j < wl->size_y; j++)
			if (wl->chunks[i][j])
				chunk_unload(w, wl->index, i, j, wl->chunks[i][j]);

	world_layer_free(wl);
}

void world_save(world *w)
{
	char filename[256];
	make_full_world_path(filename, w);
	write_world(w, filename);

	for (int i = 0; i < w->layers.length; i++)
		layer_save(w, &w->layers.data[i]);
}

void world_load(world *w)
{
	char filename[256];
	make_full_world_path(filename, w);
	read_world(w, filename);

	for (int i = 0; i < w->layers.length; i++)
		layer_load(w, &w->layers.data[i], i);
}

void world_unload(world *w)
{
	char filename[256];
	make_full_world_path(filename, w);
	write_world(w, filename);

	for (int i = 0; i < w->layers.length; i++)
		layer_unload(w, &w->layers.data[i]);
}
