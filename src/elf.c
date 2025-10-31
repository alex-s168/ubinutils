#include "ubu/elf.h"
#include "ubu/utils.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool Elf_shouldSwapEndianess(Elf_Header const *header) {
  return (header->begin.datat == ELFDATA_BIG) != is_bigendian();
}

/** 0 = ok */
int Elf_decodeElfHeader(Elf_Header *dest, FILE *file,
                        void (*err)(const char *)) {
  int status = 0;

  fread(dest, sizeof(dest->begin) + sizeof(dest->part1), 1, file);

  if (memcmp(dest->begin.magic,
             "\x7f"
             "ELF",
             4)) {
    status = 1;
    if (err)
      err("invalid file magic sequence");
  }

  if (!(dest->begin.clazz == ELFCLASS_32 || dest->begin.clazz == ELFCLASS_64)) {
    status = 1;
    if (err)
      err("invalid elf class");
  }

  if (!(dest->begin.datat == ELFDATA_BIG ||
        dest->begin.datat == ELFDATA_LITTLE)) {
    status = 1;
    if (err)
      err("invalid elf data type");
  }

  if (status)
    return status;

  if (dest->begin.clazz == ELFCLASS_32) {
    fread(&dest->m32, sizeof(dest->m32), 1, file);
  } else {
    fread(&dest->m64, sizeof(dest->m64), 1, file);
  }

  fread(&dest->part3, sizeof(dest->part3), 1, file);

  if (Elf_shouldSwapEndianess(dest)) {
    endianess_swap(dest->part1.type);
    endianess_swap(dest->part1.machine);
    endianess_swap(dest->part1.version);

    if (dest->begin.clazz == ELFCLASS_32) {
      endianess_swap(dest->m32.entry);
      endianess_swap(dest->m32.phoff);
      endianess_swap(dest->m32.shoff);
    } else {
      endianess_swap(dest->m64.entry);
      endianess_swap(dest->m64.phoff);
      endianess_swap(dest->m64.shoff);
    }

    endianess_swap(dest->part3.flags);
    endianess_swap(dest->part3.ehsize);
    endianess_swap(dest->part3.phentsize);
    endianess_swap(dest->part3.phnum);
    endianess_swap(dest->part3.shentsize);
    endianess_swap(dest->part3.shnum);
    endianess_swap(dest->part3.shstrndx);
  }

  if (dest->begin.version_copy != dest->part1.version) {
    status = 1;
    if (err)
      err("mismatched elf version");
  }

  if (dest->part1.version != EV_CURRENT) {
    status = 1;
    if (err)
      err("invalid elf version");
  }

  return status;
}

/** 0 = ok */
int Elf_decodeSectionHeader(Elf64_SectionHeader *dest, size_t id,
                            Elf_Header const *elf, FILE *file,
                            void (*err)(const char *)) {
  fseek(file, Elf_part2(elf, size_t, shoff) + elf->part3.shentsize * id,
        SEEK_SET);

#define GEN_ENDIAN_SWP(dest)                                                   \
  if (Elf_shouldSwapEndianess(elf)) {                                          \
    endianess_swap((dest)->sh_name);                                           \
    endianess_swap((dest)->sh_type);                                           \
    endianess_swap((dest)->sh_flags);                                          \
    endianess_swap((dest)->sh_addr);                                           \
    endianess_swap((dest)->sh_offset);                                         \
    endianess_swap((dest)->sh_size);                                           \
    endianess_swap((dest)->sh_link);                                           \
    endianess_swap((dest)->sh_info);                                           \
    endianess_swap((dest)->sh_addralign);                                      \
    endianess_swap((dest)->sh_entsize);                                        \
  }

  if (elf->begin.clazz == ELFCLASS_32) {
    Elf32_SectionHeader temp;
    fread(&temp, sizeof(Elf32_SectionHeader), 1, file);
    GEN_ENDIAN_SWP(&temp);

    dest->sh_name = (Elf32_Word)temp.sh_name;
    dest->sh_type = (Elf_SectionHeaderType)temp.sh_type;
    dest->sh_flags = (Elf64_SectionHeaderFlags)temp.sh_flags;
    dest->sh_addr = (Elf64_Addr)temp.sh_addr;
    dest->sh_offset = (Elf64_Off)temp.sh_offset;
    dest->sh_size = (uint64_t)temp.sh_size;
    dest->sh_link = (Elf32_Word)temp.sh_link;
    dest->sh_info = (Elf32_Word)temp.sh_info;
    dest->sh_addralign = (uint64_t)temp.sh_addralign;
    dest->sh_entsize = (uint64_t)temp.sh_entsize;
  } else {
    fread(dest, sizeof(Elf64_SectionHeader), 1, file);
    GEN_ENDIAN_SWP(dest);
  }

#undef GEN_ENDIAN_SWP

  return 0;
}

int Elf_readSection(void **heapDest, size_t *sizeDest, Elf_Header const *elf,
                    Elf64_SectionHeader const *section, FILE *file,
                    void (*err)(const char *)) {
  if (sizeDest)
    *sizeDest = section->sh_size;
  *heapDest = malloc(section->sh_size);

  if (!*heapDest) {
    if (err)
      err("out of memory");
    return 1;
  }

  fseek(file, section->sh_offset, SEEK_SET);
  fread(*heapDest, section->sh_size, 1, file);

  return 0;
}

/** 0 = ok */
int Elf_getStrTable(char **heapDest, size_t *sizeDest, size_t id,
                    Elf_Header const *elf, FILE *file,
                    void (*err)(const char *)) {
  Elf64_SectionHeader sh;
  if (Elf_decodeSectionHeader(&sh, id, elf, file, err))
    return 1;

  return Elf_readSection((void **)heapDest, sizeDest, elf, &sh, file, err);
}

int Elf_getSymTable(Elf64_Sym **heapDest, size_t *sizeDest,
                    Elf_Header const *elf, Elf64_SectionHeader const *section,
                    FILE *file, void (*err)(const char *)) {
  if (elf->begin.clazz == ELFCLASS_32) {
    Elf32_Sym *h0;
    size_t h0uz;
    if (Elf_readSection((void **)&h0, &h0uz, elf, section, file, err))
      return 1;
    size_t num = h0uz / sizeof(Elf32_Sym);
    *sizeDest = num;

    *heapDest = malloc(sizeof(Elf64_Sym) * num);
    if (!*heapDest) {
      if (err)
        err("out of memory");
      free(h0);
      return 1;
    }

    for (size_t i = 0; i < num; i++) {
      Elf64_Sym *d = &(*heapDest)[i];
      Elf32_Sym *s = &h0[i];

      if (Elf_shouldSwapEndianess(elf)) {
        endianess_swap(s->name);
        endianess_swap(s->info);
        endianess_swap(s->other);
        endianess_swap(s->shndx);
        endianess_swap(s->value);
        endianess_swap(s->size);
      }

      d->name = s->name;
      d->info = s->info;
      d->other = s->other;
      d->shndx = s->shndx;
      d->value = (Elf64_Addr)s->value;
      d->size = (uint8_t)s->size;
    }

    free(h0);
    return 0;
  } else {
    if (Elf_readSection((void **)heapDest, sizeDest, elf, section, file, err))
      return 1;
    *sizeDest /= sizeof(Elf64_Sym);
    for (size_t i = 0; i < *sizeDest; i++) {
      Elf64_Sym *s = &(*heapDest)[i];

      if (Elf_shouldSwapEndianess(elf)) {
        endianess_swap(s->name);
        endianess_swap(s->info);
        endianess_swap(s->other);
        endianess_swap(s->shndx);
        endianess_swap(s->value);
        endianess_swap(s->size);
      }
    }
    return 0;
  }
}

void OpElf_close(OpElf *elf) {
  free(elf->sectionHeaders);
  free(elf->master_strtab);
}

int OpElf_open(OpElf *dest, FILE *consumeFile, void (*err)(const char *)) {
  dest->file = consumeFile;

  if (Elf_decodeElfHeader(&dest->header, consumeFile, err)) {
    return 1;
  }

  if (Elf_getStrTable(&dest->master_strtab, NULL, dest->header.part3.shstrndx,
                      &dest->header, consumeFile, err)) {
    return 1;
  }

  dest->sectionHeaders =
      malloc(sizeof(Elf64_SectionHeader) * dest->header.part3.shnum);
  if (!dest->sectionHeaders) {
    if (err)
      err("out of memory");
    free(dest->master_strtab);
    return 1;
  }

  for (size_t i = 0; i < dest->header.part3.shnum; i++) {
    if (Elf_decodeSectionHeader(&dest->sectionHeaders[i], i, &dest->header,
                                consumeFile, err)) {
      if (err) {
        char msg[128];
        sprintf(msg, "failed to decode section header %zu", i);
        err(msg);
      }
      OpElf_close(dest);
      return 1;
    }
  }

  return 0;
}

ssize_t OpElf_findSection(OpElf const *elf, const char *want) {
  for (size_t i = 0; i < elf->header.part3.shnum; i++) {
    uint32_t name = elf->sectionHeaders[i].sh_name;
    if (name && !strcmp(elf->master_strtab + name, want))
      return (ssize_t)i;
  }
  return -1;
}
