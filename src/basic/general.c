#include "include/general.h"

#include <stdlib.h>

u64 generate_uuid()
{
	u64 ret = 0;
	u8 *buh = (u8 *)&ret;

	for (u8 i = 0; i < 8; i++)
		buh[i] = rand();

	ret ^= rand();

	return ret;
}

void *memmem(const void *haystack, size_t haystack_len, const void *const needle, const size_t needle_len)
{
	assert(haystack != NULL);
	assert(needle != NULL);
	if (haystack_len == 0)
		return NULL;
	if (needle_len == 0)
		return NULL;

	for (const u8 *h = haystack; haystack_len >= needle_len; ++h, --haystack_len)
		if (!memcmp(h, needle, needle_len))
			return (void *)h;
	return NULL;
}
