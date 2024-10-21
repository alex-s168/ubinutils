#include "pe.h"
#include "utils.h"
#include <stdbool.h>

void ToPeSection(PeSection* dst, CoffSection const* src)
{
  memcpy(dst->name, src->name, 8);
  dst->virtualSize = (uint32_t) src->virtualSize;
  dst->virtualAddress = (uint32_t) src->virtualAddress;
  dst->dataUz = (uint32_t) src->dataUz;
  dst->fileDataOffset = (uint32_t) src->fileDataOffset;
  dst->fileRelocsOffset = (uint32_t) src->fileRelocsOffset;
  dst->fileLinenumsOffset = (uint32_t) src->fileLinenumsOffset;
  dst->numRelocs = (uint16_t) src->numRelocs;
  dst->numLinenums = (uint16_t) src->numLinenums;
  dst->characteristics = (CoffSectionCharacteristics) src->characteristics;
}

const char* CoffSym_name(CoffSym const* sym, OpPe* pe)
{
  if ( sym->name[0] == 0 )
  {
    uint32_t strtabidx = * (uint32_t*) (&sym->name[4]);
    if ( is_bigendian() )
      endianess_swap(strtabidx);
    return pe->heapStrTab + strtabidx - 4;
  }
  else
  {
    return sym->name;
  }
}

void OpPe_rewindToSyms(OpPe* pe)
{
  fseek(pe->file, pe->header.fileOffToCoffSymTable, SEEK_SET);
}

void OpPe_nextSym(CoffSym* out, OpPe* pe)
{
  fread(out, sizeof(CoffSym), 1, pe->file);
  if ( is_bigendian() )
  {
    endianess_swap(out->type);
    endianess_swap(out->value);
    endianess_swap(out->sectionId);
    endianess_swap(out->numAuxSyms);
    endianess_swap(out->storageClass);
  }
}

void OpPe_close(OpPe* pe)
{
  free(pe->sections);
  free(pe->heapStrTab);
  fclose(pe->file);
}

/** FIle gets consumed */ 
int OpPe_open(OpPe* dest, FILE* file)
{
  dest->file = file;

  bool isCoff = false;
  unsigned char coffMagic[2] = {0,0};
  fread(coffMagic, 1, 2, file);
  if ( coffMagic[0] == 0x4C && coffMagic[1] == 0x01 )
    isCoff = true;
  else if ( coffMagic[0] == 0x64 && coffMagic[1] == 0x86 )
    isCoff = true;
  else if ( coffMagic[0] == 0x00 && coffMagic[1] == 0x02 )
    isCoff = true;
  dest->isCoff = isCoff;
  rewind(file);

  if ( !isCoff )
  {
    fseek(file, 0x3c, SEEK_SET);
    uint32_t off;
    fread(&off, 4, 1, file);
    if ( is_bigendian() )
      endianess_swap(off);
    fseek(file, off, SEEK_SET);

    char sig[4] = {0};
    fread(sig, 4, 1, file);
    if ( memcmp(sig, "PE\0", 4) ) {
      puts(sig);
      fclose(file);
      return 1;
    }
  }

  fread(&dest->header, sizeof(CoffHeader), 1, file);

  if ( is_bigendian() )
  {
    endianess_swap(dest->header.machine);
    endianess_swap(dest->header.numSections);
    endianess_swap(dest->header.timeDateStamp);
    endianess_swap(dest->header.fileOffToCoffSymTable);
    endianess_swap(dest->header.numCoffSym);
    endianess_swap(dest->header.optHeaderSize);
    endianess_swap(dest->header.characteristics);
  }

  // skip opt header 
  fseek(file,
        dest->header.optHeaderSize + (isCoff ? 2 : 0),
        SEEK_CUR);

  size_t sectionSize = isCoff ? sizeof(CoffSection) : sizeof(PeSection);

  dest->sections = malloc(dest->header.numSections * sectionSize);
  if ( !dest->sections ) {
    fclose(file);
    return 1;
  }

  fread(dest->sections, sectionSize, dest->header.numSections, file);

  if ( is_bigendian() )
  {
    for ( size_t i = 0; i < dest->header.numSections; i ++ )
    {
      if ( isCoff ) {
        CoffSection* s = &((CoffSection*) dest->sections)[i];
        endianess_swap(s->virtualSize);
        endianess_swap(s->virtualAddress);
        endianess_swap(s->dataUz);
        endianess_swap(s->fileDataOffset);
        endianess_swap(s->fileRelocsOffset);
        endianess_swap(s->fileLinenumsOffset);
        endianess_swap(s->numRelocs);
        endianess_swap(s->numLinenums);
        endianess_swap(s->characteristics);
      }
      else {
        PeSection* s = &((PeSection*) dest->sections)[i];
        endianess_swap(s->virtualSize);
        endianess_swap(s->virtualAddress);
        endianess_swap(s->dataUz);
        endianess_swap(s->fileDataOffset);
        endianess_swap(s->fileRelocsOffset);
        endianess_swap(s->fileLinenumsOffset);
        endianess_swap(s->numRelocs);
        endianess_swap(s->numLinenums);
        endianess_swap(s->characteristics);
      }
    }
  }

  size_t coffstrtab = dest->header.fileOffToCoffSymTable + dest->header.numCoffSym * sizeof(CoffSym);
  fseek(file, coffstrtab, SEEK_SET);

  uint32_t strtabuz;
  fread(&strtabuz, 4, 1, file);
  if ( is_bigendian() )
    endianess_swap(strtabuz);
  strtabuz -= 4;

  dest->heapStrTab = malloc(strtabuz);
  if ( !dest->heapStrTab ) {
    free(dest->sections);
    fclose(file);
    return 1;
  }

  fread(dest->heapStrTab, 1, strtabuz, file);

  return 0;
}
