#ifndef _UTILS_H
#define _UTILS_H

#include <stddef.h>
#include <stdint.h>
#include <ctype.h>

#define PLACE(valty, val) \
        ((valty*)((valty[]) {val}))

#define is_bigendian() ( (* ((char*) PLACE(int, 1)) ) == 0 )

#define endianess_swap(varr) \
  { \
    typeof(varr) _src = varr; \
    typeof(varr) _res; \
    memrevcpy( &_res, &_src, sizeof(_src) ); \
    varr = _res; \
  }

static void memrevcpy(void* dest, void const* src, size_t bytes)
{
  if (bytes == 0) return;
  unsigned char * udst = dest;
  unsigned char const * usrc = src;
  size_t d = bytes - 1;
  for (size_t i = 0; i < bytes; i ++) {
    udst[d--] = usrc[i];
  }
}

#define FNV1A(type, prime, offset, dest, value, valueSize) { \
    type hash = offset;                                      \
    for (size_t i = 0; i < valueSize; i ++) {                \
        uint8_t byte = ((uint8_t *)value)[i];                \
        hash ^= byte;                                        \
        hash *= prime;                                       \
    }                                                        \
    *(type *)dest = hash;                                    \
}

static uint64_t hash(const unsigned char * data, int len)
{
    uint64_t res;
    FNV1A(uint64_t, 0x100000001B3, 0xcbf29ce484222325, &res, data, len);
    return res;
}

static int strieq(char const* a, char const* b)
{
  while (*a && *b)
  {
    if (tolower(*a) != tolower(*b))
      return 0;
    a++;
    b++;
  }
  return *a == *b;
}

#endif
