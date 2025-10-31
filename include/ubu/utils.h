#ifndef _UTILS_H
#define _UTILS_H

#include <stddef.h>
#include <stdint.h>

#define PLACE(valty, val) ((valty *)((valty[]){val}))

#define is_bigendian() ((*((char *)PLACE(int, 1))) == 0)

#define endianess_swap(varr)                                                   \
  {                                                                            \
    typeof(varr) _src = varr;                                                  \
    typeof(varr) _res;                                                         \
    memrevcpy(&_res, &_src, sizeof(_src));                                     \
    varr = _res;                                                               \
  }

void memrevcpy(void *dest, void const *src, size_t bytes);

uint64_t hash(const unsigned char *data, int len);

int strieq(char const *a, char const *b);

#endif
