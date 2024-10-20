#include "../lib/elf.h"
#include <inttypes.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

static void errclbk(const char * msg) {
  fprintf(stderr, "elf error: %s\n", msg);
}

int main(int argc, char const* const* argv)
{
  size_t ptrstrwidth;
  {
    char buf[32];
    sprintf(buf, "%016" PRIXPTR, (uintptr_t) argv);
    ptrstrwidth = strlen(buf);
  }

  if ( argc != 2 ) {
    fprintf(stderr, "invalid usage!\nusage: %s [file]\n", argv[0]);
    return 1;
  }

  FILE* f = fopen(argv[1], "r");
  if ( f == NULL ) {
    fprintf(stderr, "could not open file\n");
    return 1;
  }

  OpElf elf;
  if ( OpElf_open(&elf, f, errclbk) )
    return 1;

  ssize_t symtab = OpElf_findSection(&elf, ".symtab");
  if ( symtab == -1 )
    symtab = OpElf_findSection(&elf, ".dynsym");

  if ( symtab == -1 ) {
    fprintf(stderr, "file has no \".symtab\" or \".dynsym\" section\n");
  }
  else {
    Elf64_SectionHeader sec = elf.sectionHeaders[symtab];

    char * tsstab;
    if ( Elf_getStrTable(&tsstab, NULL, sec.sh_link, &elf.header, f, errclbk) ) {
      fprintf(stderr, "failed to decode string table used by section\n");
    }
    else {
      Elf64_Sym* syms;
      size_t syms_len = 0;
      if ( Elf_getSymTable(&syms, &syms_len, &elf.header, &sec, f, errclbk) )
      {
        fprintf(stderr, "failed to decode symbol table\n");
      }

      if ( syms_len ) {
        // first symbol is fake
        syms_len --;
        syms ++;
      }

      for (size_t u = 0; u < syms_len; u++)
      {
        uint8_t bind_attrib = syms[u].info >> 4;
        uint8_t sym_type = syms[u].info & 0xF;

        const char * sname = NULL;
        {
          uint32_t sectionnam = 0;
          if ( syms[u].shndx && syms[u].shndx < elf.header.part3.shnum )
            sectionnam = elf.sectionHeaders[syms[u].shndx].sh_name;
          if ( sectionnam ) 
            sname = elf.master_strtab + sectionnam;
        }

        bool is_global = u >= sec.sh_info;

        if ( syms[u].value ) {
          printf("%016" PRIXPTR, (uintptr_t) syms[u].value);
        } else {
          for ( size_t i = 0; i < ptrstrwidth; i ++ )
            fputc(' ', stdout);
        }

        char id = '?';
        if ( syms[u].shndx == SHN_UNDEF )
          id = 'U';
        else if ( syms[u].shndx == SHN_ABS )
          id = 'A';
        else if ( sname && !strcmp(sname, ".text") )
          id = is_global ? 'T' : 't';
        else if ( sname && !strcmp(sname, ".bss") )
          id = is_global ? 'B' : 'b';
        else if ( sname && !strcmp(sname, ".data") )
          id = is_global ? 'D' : 'd';
        else if ( sname && !strcmp(sname, ".rodata") )
          id = is_global ? 'R' : 'r';

        printf("\t%c\t", id);

        uint32_t name = syms[u].name;
        if ( name && *(tsstab + name) ) {
          printf("%s\n", tsstab + name);
        } else {
          printf("unnamed\n");
        }
      }

      free(tsstab);
    }
  }

  OpElf_close(&elf);
  return 0;
}
