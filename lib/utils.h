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

static void memrevcpy(void* restrict dest, void const* restrict src, size_t bytes)
{
  if (bytes == 0) return;
  unsigned char * udst = dest;
  unsigned char const * usrc = src;
  size_t d = bytes - 1;
  for (size_t i = 0; i < bytes; i ++) {
    udst[d--] = usrc[i];
  }
}
