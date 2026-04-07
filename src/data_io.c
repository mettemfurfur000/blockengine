#include "include/data_io.h"
#include "include/vars.h"

u8 flip_buf[8] = {};

void blob_write(blob b, stream_t *f)
{
	WRITE(b.length, f);
	WRITE_N(b.str, f, b.length);
}

blob blob_read(stream_t *f)
{
	blob b;
	READ(b.length, f);
	b.str = malloc(b.length);
	stream_read(b.str, b.length, f);
	return b;
}

void blob_vars_write(blob b, stream_t *f)
{
	WRITE(b.length, f);

	// stream_write(b.ptr, b.length, f);

	VAR_FOREACH(b, {
		char letter = b.ptr[i];
		u8 size = b.ptr[i + 1];
		u8 *val = b.ptr + i + 2;

		WRITE(letter, f);
		WRITE(size, f);
		WRITE_N(val, f, size);
	});
}

blob blob_vars_read(stream_t *f)
{
	blob b = {};
	READ(b.length, f);
	if (b.length == 0)
		return b;

	b.ptr = calloc(b.length, 1);

	assert(b.ptr != NULL);

	VAR_FOREACH(b, {
		char letter;
		u8 size;

		READ(letter, f);
		READ(size, f);
		b.ptr[i] = (u8)letter;
		b.ptr[i + 1] = size;
		READ_N(b.ptr + i + 2, f, size);
	});

	return b;
}

void write_hashtable(hash_node **t, stream_t *f)
{
	const u32 size = TABLE_SIZE;
	const u32 elements = table_elements(t);

	WRITE(size, f);		// write table size
	WRITE(elements, f); // write number of elements

	if (elements == 0)
		return;

	for (u32 i = 0; i < TABLE_SIZE; i++)
	{
		hash_node *n = t[i];
		while (n != NULL)
		{
			blob_write(n->key, f);
			blob_write(n->value, f);

			n = n->next;
		}
	}
}

void read_hashtable(hash_node **t, stream_t *f)
{
	u32 size = TABLE_SIZE;
	u32 elements = 0;

	READ(size, f);
	READ(elements, f);

	if (elements == 0)
		return;

	if (size != TABLE_SIZE)
		LOG_WARNING("Table size mismatch, expected %d, got %d", TABLE_SIZE, size);

	for (u32 i = 0; i < elements; i++)
	{
		blob key = blob_read(f);
		blob value = blob_read(f);

		LOG_DEBUG("Read table entry: \'%s\' - \'%s\'", key.str, value.str);

		// i could just put it in place but im not
		// sure about the order of them being called
		put_entry(t, key, value);
	}
}

void stream_embed_file_write(const char *file_path, stream_t *s)
{
	// open the file
	FILE *file = fopen(file_path, "rb");
	// get file size
	fseek(file, 0, SEEK_END);
	u32 file_size = ftell(file);
	fseek(file, 0, SEEK_SET);
	// write file size
	WRITE(file_size, s);

	// read and write file data in chunks
	u8 buffer[BUFFER_SIZE];
	u32 bytes_remaining = file_size;
	while (bytes_remaining > 0)
	{
		u32 bytes_to_read = bytes_remaining < sizeof(buffer) ? bytes_remaining : sizeof(buffer);
		size_t read_bytes = fread(buffer, 1, bytes_to_read, file);
		if (read_bytes == 0)
		{
			LOG_ERROR("Failed to read from file during embedding: %s", file_path);
			break;
		}
		WRITE_N(buffer, s, read_bytes);
		bytes_remaining -= (u32)read_bytes;
	}

	fclose(file);
}

void stream_embed_file_read_to_mem(stream_t *s, u8 **out_data, u32 *out_size)
{
	// read file size
	u32 file_size = 0;
	READ(file_size, s);
	*out_size = file_size;

	// allocate memory
	u8 *data = malloc(file_size);
	assert(data != NULL);

	// read file data in chunks
	u32 bytes_remaining = file_size;
	u8 *data_ptr = data;
	while (bytes_remaining > 0)
	{
		u32 bytes_to_read = bytes_remaining < BUFFER_SIZE ? bytes_remaining : BUFFER_SIZE;
		READ_N(data_ptr, s, bytes_to_read);
		data_ptr += bytes_to_read;
		bytes_remaining -= bytes_to_read;
	}

	*out_data = data;
}
