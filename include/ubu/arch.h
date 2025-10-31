#ifndef UBU_ARCHS_H
#define UBU_ARCHS_H

#include <stdbool.h>
#include <stddef.h>

typedef struct {
  char const *const *names;

  struct {
    bool supported;
    unsigned arch;
  } capstone;
} ubu_arch_t;

extern ubu_arch_t ubu_archs[];

ubu_arch_t *ubu_findi_arch(char const *name);

#endif
