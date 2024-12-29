#ifndef ENDIANLESS
#define ENDIANLESS

#include "general.h"

volatile u8 is_big_endian(void);
volatile u8 is_little_endian(void);

int flip_bytes_in_place(u8 *bytes, int size);
int make_endianless(u8 *bytes, int size);

// make byte data little endian
// in little endian makes no change
// in big flips bytes

// for example
/*
value = 0x12345678;

u8* val_arr = (u8*)&value;

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

#endif