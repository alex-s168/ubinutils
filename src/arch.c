#include "ubu/arch.h"
#include "ubu/utils.h"

#include <capstone/capstone.h>

ubu_arch_t ubu_archs[] = {
    {.names = (char const *[]){"etca", 0},
     .capstone = {.supported = true, .arch = CS_ARCH_ETCA}},
    {0}};

ubu_arch_t *ubu_findi_arch(char const *name) {
  for (ubu_arch_t *p = ubu_archs; p->names; p++) {
    for (char const *const *n = p->names; *n; n++) {
      if (strieq(*n, name))
        return p;
    }
  }
  return NULL;
}
