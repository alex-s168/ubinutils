#include "ubu/pe.h"
#include "ubu/elf.h"
#include "ubu/ar.h"
#include "ubu/memfile.h"
#include <string.h>

/*
output format:

text <t> data <t> bss <t> total <t> filename 
123 <t> 12434 <t> 3 <t> 3521 <t> some.file

*/

static size_t elfSectionSize(OpElf* elf, const char * name)
{
  ssize_t s = OpElf_findSection(elf, name);
  size_t size = 0;
  if ( s != -1 ) {
    size = elf->sectionHeaders[s].sh_size;
  }
  return size;
}

static void sizeElf(OpElf* elf, const char * filename)
{
  size_t s_text = elfSectionSize(elf, ".text");

  size_t s_data = elfSectionSize(elf, ".data");
  s_data += elfSectionSize(elf, ".rodata");

  size_t s_bss = elfSectionSize(elf, ".bss");

  size_t total = s_text + s_data + s_bss;

  printf("%zu\t%zu\t%zu\t%zu\t%s\n",
      s_text, s_data, s_bss, total, filename);
}

static void sizePe(OpPe* pe, const char * filename)
{
  size_t secSizes[3] = {0,0,0};
  int numPopulatedSec = 0;
  size_t sum = 0;

  for ( uint16_t i = 0; i < pe->header.numSections; i ++ )
  {
    PeSection sec;
    OpPe_getPeSection(&sec, pe, i);

    int sidx;
    if ( !strcmp(sec.name, ".text") )
      sidx = 0;
    else if ( !strcmp(sec.name, ".bss") )
      sidx = 2;
    else if ( !strcmp(sec.name, ".data") )
      sidx = 1;
    else if ( !strcmp(sec.name, ".rdata") )
      sidx = 1;
    else 
      continue;

    secSizes[sidx] = sec.dataUz;
    sum += sec.dataUz;

    numPopulatedSec ++;
    if ( numPopulatedSec == 4 )
      break;
  }

  printf("%zu\t%zu\t%zu\t%zu\t%s\n",
      secSizes[0], secSizes[1], secSizes[2], sum, filename);
}

static int sizeObjfile(FILE* file, const char * filename)
{
  OpElf elf;
  rewind(file);
  if ( !OpElf_open(&elf, file, NULL) )
  {
    sizeElf(&elf, filename);
    OpElf_close(&elf);
    return 0;
  }

  OpPe pe;
  rewind(file);
  if ( !OpPe_open(&pe, file) )
  {
    sizePe(&pe, filename);
    OpPe_close(&pe);
    return 0;
  }

  return 1;
}

static void sizeAr(SmartArchive* ar)
{
  SmartArchive_rewind(ar);

  char* name;
  while ( (name = SmartArchive_nextFileNameHeap(ar)) )
  {
    void* data; size_t size;
    if ( SmartArchive_continueWithData(&data, &size, ar) )
    {
      fprintf(stderr, "out of memory\n");
      return;
    }

    FILE* file = memFileOpenReadOnly(data, size);

    if ( sizeObjfile(file, name) )
    {
      fprintf(stderr, "%s: unrecognized format\n", name);
    }

    fclose(file);
    free(data);

    free(name);
  }
}

static char supportedFormatsStr[] = "Support file formats: {ELF{32,64},PE,COFF}";

int main(int argc, char const* const* argv)
{

  if ( argc != 2 ) {
    fprintf(stderr, "Usage: %s [file]\n%s\n", argv[0], supportedFormatsStr);
    return 1;
  }

  FILE* f = fopen(argv[1], "rb");
  if ( f == NULL ) {
    fprintf(stderr, "could not open file\n");
    return 1;
  }

  puts("text\tdata\tbss\ttotal\tfilename\n");

  SmartArchive ar;
  rewind(f);
  if ( !SmartArchive_open(&ar, f ) )
  {
    sizeAr(&ar);
    SmartArchive_close(&ar);
    fclose(f);
    return 0;
  }

  rewind(f);
  if ( !sizeObjfile(f, argv[1]) )
  {
    fclose(f);
    return 0;
  }

  fprintf(stderr, "Unsupported file format! %s\n", supportedFormatsStr);
  fclose(f);
  return 1;
}
