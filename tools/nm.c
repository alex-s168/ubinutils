#include "ubu/elf.h"
#include "ubu/pe.h"
#include "ubu/ar.h"
#include "ubu/memfile.h"
#include "ubu/aof.h"
#include <inttypes.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

static void errclbk(const char * msg) {
  fprintf(stderr, "elf error: %s\n", msg);
}

static void nmElf(OpElf* elf, size_t ptrstrwidth)
{
  ssize_t symtab = OpElf_findSection(elf, ".symtab");
  if ( symtab == -1 )
    symtab = OpElf_findSection(elf, ".dynsym");

  if ( symtab == -1 ) {
    fprintf(stderr, "file has no \".symtab\" or \".dynsym\" section\n");
  }
  else {
    Elf64_SectionHeader sec = elf->sectionHeaders[symtab];

    char * tsstab;
    if ( Elf_getStrTable(&tsstab, NULL, sec.sh_link, &elf->header, elf->file, errclbk) ) {
      fprintf(stderr, "failed to decode string table used by section\n");
    }
    else {
      Elf64_Sym* syms;
      size_t syms_len = 0;
      if ( Elf_getSymTable(&syms, &syms_len, &elf->header, &sec, elf->file, errclbk) )
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
          if ( syms[u].shndx && syms[u].shndx < elf->header.part3.shnum )
            sectionnam = elf->sectionHeaders[syms[u].shndx].sh_name;
          if ( sectionnam ) 
            sname = elf->master_strtab + sectionnam;
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

        printf(" %c ", id);

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
}

static void nmAof(AofObj* o, size_t ptrstrwidth)
{
    for (size_t sy = 0; sy < o->aof.header.num_syms; sy ++)
    {
        AofSym* sym = &o->aof.syms[sy];
        if ( sym->attribs & AofSymAttr_ABS && sym->value ) {
          printf("%016" PRIXPTR, (uintptr_t) sym->value);
        } else {
          for ( size_t i = 0; i < ptrstrwidth; i ++ )
            fputc(' ', stdout);
        }

        char id = '?';
        if ( sym->attribs & AofSymAttr_ABS )
            id = 'A';
        else 
        {
            AofAreaAttrib area = o->aof.areas[sym->ref_area].attributes;
            if ( (area &  AofAreaAttrib_CODE) && (sym->attribs & AofSymAttr_DEFINE) )
                id = ( sym->attribs & AofSymAttr_GLOBAL ) ? 'T' : 't';
            else if ( (area &  AofAreaAttrib_R_ONLY) && (sym->attribs & AofSymAttr_DEFINE) )
                id = ( sym->attribs & AofSymAttr_GLOBAL ) ? 'R' : 'r';
            else if ( sym->attribs & AofSymAttr_DEFINE )
                id = ( sym->attribs & AofSymAttr_GLOBAL ) ? 'D' : 'd';
        }

        char const* name = ChunkFile_getStr(&o->ch, sym->name);

        printf(" %c %s\n", id, name);
    }
}

static void nmPe(OpPe* pe, size_t ptrstrwidth)
{
  OpPe_rewindToSyms(pe);
  for ( size_t i = 0; i < pe->header.numCoffSym; i ++ )
  {
    CoffSym sym;
    OpPe_nextSym(&sym, pe);

    if ( !(sym.sectionId == 0xFFFE) ) // not debug 
    {
      const char * name = CoffSym_name(&sym, pe);

      bool discard = false;

      const char * sname = NULL;
      if ( sym.sectionId && sym.sectionId != 0xFFFF && sym.sectionId < pe->header.numSections )
      {
        PeSection section;
        OpPe_getPeSection(&section, pe, sym.sectionId);

        char sbname[9];
        sbname[8] = '\0';
        memcpy(sbname, section.name, 8);
        sname = sbname;
      }

      if ( sym.storageClass == IMAGE_SYM_CLASS_SECTION )
        discard = true;
      else if ( sym.storageClass == IMAGE_SYM_CLASS_CLR_TOKEN )
        discard = true;
      else if ( sym.storageClass == IMAGE_SYM_CLASS_FILE )
        discard = true;
      else if ( sym.storageClass == IMAGE_SYM_CLASS_STATIC && sym.name[0] == '.' )
        discard = true;

      if ( !discard )
      {
        if ( sym.value ) {
          printf("%016" PRIXPTR, (uintptr_t) sym.value);
        } else {
          for ( size_t i = 0; i < ptrstrwidth; i ++ )
            fputc(' ', stdout);
        }

        bool is_global = sym.storageClass == IMAGE_SYM_CLASS_EXTERNAL;

        char type = '?';
        if ( sym.sectionId == 0xFFFF )
          type = 'A';
        else if ( sym.sectionId == 0 )
          type = 'U';
        else if ( sname && !strcmp(sname, ".text") )
          type = is_global ? 'T' : 't';
        else if ( sname && !strcmp(sname, ".bss") )
          type = is_global ? 'B' : 'b';
        else if ( sname && !strcmp(sname, ".data") )
          type = is_global ? 'D' : 'd';
        else if ( sname && !strcmp(sname, ".rdata") )
          type = is_global ? 'R' : 'r';

        printf(" %c %s\n", type, name);
      }
    }

    // skip following aux sysm
    for ( size_t j = 0; j < sym.numAuxSyms; j ++ )
      OpPe_nextSym(&sym, pe);
    i += sym.numAuxSyms;
  }
}

static int nmObjfile(FILE* file, size_t ptrstrwidth)
{
  OpElf elf;
  rewind(file);
  if ( !OpElf_open(&elf, file, NULL) )
  {
    nmElf(&elf, ptrstrwidth);
    OpElf_close(&elf);
    return 0;
  }

  OpPe pe;
  rewind(file);
  if ( !OpPe_open(&pe, file) )
  {
    nmPe(&pe, ptrstrwidth);
    OpPe_close(&pe);
    return 0;
  }

  AofObj aof;
  rewind(file);
  if ( !AofObj_open(&aof, file) )
  {
    nmAof(&aof, ptrstrwidth);
    AofObj_close(&aof);
    return 0;
  }

  return 1;
}

static void nmAr(SmartArchive* ar, size_t ptrstrwidth)
{
  SmartArchive_rewind(ar);

  char* name;
  while ( (name = SmartArchive_nextFileNameHeap(ar)) )
  {
    printf("%s:\n", name);
    free(name);

    void* data; size_t size;
    if ( SmartArchive_continueWithData(&data, &size, ar) )
    {
      fprintf(stderr, "out of memory\n");
      return;
    }

    FILE* file = memFileOpenReadOnly(data, size);

    if ( nmObjfile(file, ptrstrwidth) )
    {
      printf("unrecognized format\n");
    }

    fclose(file);
    free(data);

    fputc('\n', stdout);
  }
}

static char supportedFormatsStr[] = "Support file formats: {,AR of }{ELF{32,64},PE,COFF}";

int main(int argc, char const* const* argv)
{
  size_t ptrstrwidth;
  {
    char buf[32];
    sprintf(buf, "%016" PRIXPTR, (uintptr_t) argv);
    ptrstrwidth = strlen(buf);
  }

  if ( argc != 2 ) {
    fprintf(stderr, "Usage: %s [file]\n%s\n", argv[0], supportedFormatsStr);
    return 1;
  }

  FILE* f = fopen(argv[1], "rb");
  if ( f == NULL ) {
    fprintf(stderr, "could not open file\n");
    return 1;
  }

  SmartArchive ar;
  rewind(f);
  if ( !SmartArchive_open(&ar, f ) )
  {
    nmAr(&ar, ptrstrwidth);
    SmartArchive_close(&ar);
    fclose(f);
    return 0;
  }

  if ( !nmObjfile(f, ptrstrwidth) )
  {
    fclose(f);
    return 0;
  }

  fprintf(stderr, "Unsupported file format! %s\n", supportedFormatsStr);
  fclose(f);
  return 1;
}
