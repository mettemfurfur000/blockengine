#include "include/endianless.h"
#include <string.h>

volatile u8 is_big_endian(void)
{
	volatile static union
	{
		unsigned int i;
		char c[4];
	} e = {0x01000000};

	return e.c[0];
}

volatile u8 is_little_endian(void)
{
	volatile static union
	{
		unsigned int i;
		char c[4];
	} e = {0x00000001};

	return e.c[0];
}

void endianless_copy(u8 *dest, u8 *src, int size)
{
	if (is_little_endian())
		memcpy(dest, src, size);
	else
		for (int i = size - 1; i >= 0; i--)
			dest[size - i - 1] = src[i];
}

int endianless_write(u8 *data, int size, FILE *f)
{
	if (is_little_endian()) // write as is if little endian
		return fwrite(data, 1, size, f) == size ? SUCCESS : FAIL;
	for (int i = size - 1; i >= 0; i--) // write in reverse if big endian
		fputc(data[i], f);
	return SUCCESS;
}

int endianless_read(u8 *data, int size, FILE *f)
{
	if (is_little_endian()) // read as is if little endian
		return fread(data, 1, size, f) == size ? SUCCESS : FAIL;
	for (int i = size - 1; i >= 0; i--) // read in reverse if big endian
		data[i] = fgetc(f);
	return SUCCESS;
}

int flip_bytes_in_place(u8 *bytes, int size)
{
	if (size == 1)
		return SUCCESS;
	if (size % 2 != 0)
		return FAIL;
	int sizehalf = size / 2;
	u8 temp;
	int end_index;

	for (u32 i = 0; i < sizehalf; i++)
	{
		end_index = size - i - 1;

		temp = bytes[i];

		bytes[i] = bytes[end_index];

		bytes[end_index] = temp;
	}
	return SUCCESS;
}

int make_endianless(u8 *bytes, int size)
{
	if (is_big_endian())
		return flip_bytes_in_place(bytes, size);
	return SUCCESS;
}

u16 endian_flip_u16(u16 value)
{
	if (is_big_endian())
		return ((value & 0xff00) >> 8) | ((value & 0x00ff) << 8);
	return value;
}

u32 endian_flip_u32(u32 value)
{
	if (is_big_endian())
		return ((value & 0xff000000) >> 24) | //
			   ((value & 0x00ff0000) >> 8) |  //
			   ((value & 0x0000ff00) << 8) |  //
			   ((value & 0x000000ff) << 24);
	return value;
}

u64 endian_flip_u64(u64 value)
{
	if (is_big_endian())
		return ((value & 0xff00000000000000ULL) >> 56) | //
			   ((value & 0x00ff000000000000ULL) >> 40) | //
			   ((value & 0x0000ff0000000000ULL) >> 24) | //
			   ((value & 0x000000ff00000000ULL) >> 8) |	 //
			   ((value & 0x00000000ff000000ULL) << 8) |	 //
			   ((value & 0x0000000000ff0000ULL) << 24) | //
			   ((value & 0x000000000000ff00ULL) << 40) | //
			   ((value & 0x00000000000000ffULL) << 56);
	return value;
}