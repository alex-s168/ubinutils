#include "ubu/utils.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <capstone/capstone.h>

typedef struct {
  char const *const *names;

  struct {
    bool supported;
    cs_arch arch;
  } capstone;
} ubu_arch_t;

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

int main(int argc, char const *const *argv) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <arch> <file>\n", *argv);
    return 1;
  }

  ubu_arch_t *arch = ubu_findi_arch(argv[1]);
  if (!arch) {
    fprintf(stderr, "Arch %s not found\n", argv[1]);
    return 1;
  }

  if (!arch->capstone.supported) {
    fprintf(stderr, "Disassembler for arch %s not implemented\n", argv[1]);
    return 1;
  }

  FILE *fp = fopen(argv[2], "rb");
  if (!fp) {
    fprintf(stderr, "Can't open file\n");
    return 1;
  }

  csh cs;
  cs_err err;
  if (CS_ERR_OK != (err = cs_open(arch->capstone.arch, 0, &cs))) {
    fprintf(stderr, "Can't initialize disassembler: %s\n", cs_strerror(err));
    fclose(fp);
    return 1;
  }

  cs_option(cs, CS_OPT_DETAIL, CS_OPT_ON);

  fseek(fp, 0, SEEK_END);
  size_t size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  uint8_t *buf = malloc(size);
  uint8_t const* bufReader = buf;
  size_t read = 0;
  if (buf) {
    read = fread(buf, 1, size, fp);
  }
  fclose(fp);
  if (!buf || read != size) {
    fprintf(stderr, "not enough memory, or IO error\n");
    cs_close(&cs);
    return 1;
  }

  uint64_t address = 0x8000;

  cs_insn *insn = cs_malloc(cs);
  while (cs_disasm_iter(cs, &bufReader, &size, &address, insn)) {
    printf("0x%" PRIx64 ":\t%s\t\t%s\n", insn->address, insn->mnemonic,
           insn->op_str);
  }
  cs_free(insn, 1);

  free(buf);
  cs_close(&cs);
}
