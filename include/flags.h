#ifndef FLAGS_H
#define FLAGS_H 1

#define FLAG_IS_VALID_POS(f, i) (i >= 0 && i < sizeof(f) ? 1 : 0)
#define FLAG_GET(f, mask) ((f) & (mask))

#define FLAG_FLIP(f, mask) ((f) ^= (mask));

#define FLAG_ON(f, mask) ((f) |= (mask));
#define FLAG_OFF(f, mask) ((f) &= ~(mask));

#define FLAG_SET(f, mask, val)                                                                                         \
    if (FLAG_GET(f, mask) != (val))                                                                                    \
    FLAG_FLIP(f, mask)

#define FLAG_CONFIGURE(f, mask, v, condition)                                                                          \
    if (condition)                                                                                                     \
        FLAG_OFF(f, mask)                                                                                              \
    else                                                                                                               \
        FLAG_SET(f, mask, v)

#endif