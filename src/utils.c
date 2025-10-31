#include "ubu/utils.h"
#include <ctype.h>

void memrevcpy(void *dest, void const *src, size_t bytes) {
  if (bytes == 0)
    return;
  unsigned char *udst = dest;
  unsigned char const *usrc = src;
  size_t d = bytes - 1;
  for (size_t i = 0; i < bytes; i++) {
    udst[d--] = usrc[i];
  }
}

int strieq(char const *a, char const *b) {
  while (*a && *b) {
    if (tolower(*a) != tolower(*b))
      return 0;
    a++;
    b++;
  }
  return *a == *b;
}

#define FNV1A(type, prime, offset, dest, value, valueSize)                     \
  {                                                                            \
    type hash = offset;                                                        \
    for (size_t i = 0; i < valueSize; i++) {                                   \
      uint8_t byte = ((uint8_t *)value)[i];                                    \
      hash ^= byte;                                                            \
      hash *= prime;                                                           \
    }                                                                          \
    *(type *)dest = hash;                                                      \
  }

uint64_t hash(const unsigned char *data, int len) {
  uint64_t res;
  FNV1A(uint64_t, 0x100000001B3, 0xcbf29ce484222325, &res, data, len);
  return res;
}
