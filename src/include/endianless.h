#ifndef ENDIANLESS
#define ENDIANLESS

#include "game_types.h"

volatile byte is_big_endian(void);
volatile byte is_little_endian(void);
int flip_bytes_in_place(byte *bytes, int size);

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

int make_endianless(byte *bytes, int size);

#endif