#include "../include/endianless.h"

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

	for (int i = 0; i < sizehalf; i++)
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
