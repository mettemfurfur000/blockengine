#ifndef ENDIANLESS
#define ENDIANLESS

#include "game_types.h"

volatile byte is_big_endian(void)
{
	volatile union
	{
		unsigned int i;
		char c[4];
	} e = {0x01000000};

	return e.c[0];
}

volatile byte is_little_endian(void)
{
	volatile union
	{
		unsigned int i;
		char c[4];
	} e = {0x00000001};

	return e.c[0];
}

int flip_bytes_in_place(byte *bytes, int size)
{
	if (size == 1)
		return SUCCESS;
	if (size % 2 != 0)
		return FAIL;
	int sizehalf = size / 2;
	byte temp;
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

// make byte data little endian
// in little endian makes no change
// in big flips bytes

// for example
/*
value = 0x12345678;

byte* val_arr = (byte*)&value;

val_arr[i] =

	0		1		2		3
	|		|		|		|
	v		V		V		V

	[ 12 ]	[ 34 ] 	[ 56 ] 	[ 78 ]

no changes in little endian, but in big makes this:

make_endianless(val_arr,4)

val_arr[i] =

	0		1		2		3
	|		|		|		|
	v		V		V		V

	[ 78 ]	[ 56 ] 	[ 34 ] 	[ 12 ]

*/

int make_endianless(byte *bytes, int size)
{
	if (is_big_endian())
		return flip_bytes_in_place(bytes, size);
	return SUCCESS;
}

#endif