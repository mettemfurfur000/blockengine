#ifndef ENDIANLESS_H
#define ENDIANLESS_H 1

#include "general.h"

volatile u8 is_big_endian(void);
volatile u8 is_little_endian(void);

int flip_bytes_in_place(u8 *bytes, int size);
int make_endianless(u8 *bytes, int size);

void endianless_copy(u8 *dest, u8 *src, int size);
int endianless_write(u8 *data, int size, FILE *f);
int endianless_read(u8 *data, int size, FILE *f);

// make byte data little endian
// in little endian makes no change
// in big flips bytes

#endif