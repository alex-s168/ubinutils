#include "ar.h"
#include <stdbool.h>

int ArIter_open(ArIter* dest, FILE* consumeFile)
{
  rewind(consumeFile);

  dest->file = consumeFile;

  char magic[8];
  if ( fread(magic, 1, 8, consumeFile) != 8 )
    return 1;

  if ( memcmp(magic, "!<arch>\x0A", 8) )
    return 1;

  return 0;
}

void ArIter_close(ArIter* i)
{
}

void ArIter_rewind(ArIter* i)
{
  fseek(i->file, 8, SEEK_SET);
}

bool ArIter_hasNext(ArIter* i)
{
  int c = fgetc(i->file);
  ungetc(c, i->file);
  return c != EOF;
}

void ArIter_beginIter(ArIterFileHeader* hdest, ArIter* i)
{
  Ar_FileHeader temp;
  fread(&temp, sizeof(Ar_FileHeader), 1, i->file);
  temp.padding[0] = '\0';
  char* space = strchr(temp.decimal_file_size, ' ');
  if ( space )
    *space = '\0';
  memcpy(hdest->filename, temp.filename, 16);
  sscanf(temp.decimal_file_size, "%zu", &hdest->fileSize);
}

void ArIter_rewindBeginIter(ArIter* i) 
{
  fseek(i->file, - (long) sizeof(Ar_FileHeader), SEEK_CUR);
}

void ArIter_readDataAndNext(void* buf, ArIterFileHeader const* hd, ArIter* i)
{
  fread(buf, hd->fileSize, 1, i->file);
}

void ArIter_noDataAndNext(ArIterFileHeader const* hd, ArIter* i)
{
  fseek(i->file, hd->fileSize, SEEK_CUR);
}

int ArIter_findNext(void** heapOut, size_t* sizeOut, ArIter* i, char const searchNam[16])
{
  while ( ArIter_hasNext(i) ) {
    ArIterFileHeader fh;
    ArIter_beginIter(&fh, i);
    if ( memcmp(fh.filename, searchNam, 16) ) {
      ArIter_noDataAndNext(&fh, i);
    } else {
      if ( sizeOut )
        *sizeOut = fh.fileSize;
      *heapOut = malloc(fh.fileSize);
      if ( *heapOut ) {
        ArIter_readDataAndNext(*heapOut, &fh, i);
      } else {
        ArIter_noDataAndNext(&fh, i);
      }

      return 0;
    }
  }

  return 1;
}

int SmartArchive_open(SmartArchive* dest, FILE* consumeFile)
{
  if ( ArIter_open(&dest->_iter, consumeFile) )
    return 1;


  if ( ArIter_findNext(&dest->exFileNames, NULL, &dest->_iter, "//              ") )
    dest->exFileNames = NULL;
  ArIter_rewind(&dest->_iter);

  return 0;
}

void SmartArchive_close(SmartArchive* archv)
{
  ArIter_close(&archv->_iter);
  if ( archv->exFileNames )
    free(archv->exFileNames);
}

void SmartArchive_rewind(SmartArchive* archv)
{
  ArIter_rewind(&archv->_iter);
}

char * SmartArchive_nextFileNameHeap(SmartArchive* archv) 
{
  if ( !ArIter_hasNext(&archv->_iter) )
    return NULL;

  ArIterFileHeader hd;
  ArIter_beginIter(&hd, &archv->_iter);
  ArIter_rewindBeginIter(&archv->_iter);

  if ( archv->exFileNames && hd.filename[0] == '/' && !(hd.filename[1] == '/' || hd.filename[1] == ' ') )
  {
    char temp[16];
    memcpy(temp, hd.filename + 1, 15);
    temp[15] = '\0';
    size_t off;
    sscanf(temp, "%zu", &off);
    const char* old = archv->exFileNames;
    old += off;
    size_t count = strchr(old, '/') - old;
    char * new = malloc(count + 1);
    if ( new ) {
      memcpy(new, old, count);
      new[count] = '\0';
    }
    return new;
  }
  else 
  {
    char* heap = malloc(17);
    if ( !heap )
      return NULL;
    memcpy(heap, hd.filename, 16);
    heap[16] = '\0';
    char * space = strchr(heap, ' ');
    if ( space )
      *space = '\0';
    char * slash = strchr(heap, '/');
    if ( slash )
      *slash = '\0';
    return heap;
  }
}

int SmartArchive_continueWithData(void** heapOut, size_t* sizeOut, SmartArchive* archv) 
{
  ArIterFileHeader hd;
  ArIter_beginIter(&hd, &archv->_iter);
  if ( sizeOut )
    *sizeOut = hd.fileSize;
  *heapOut = malloc(hd.fileSize);
  if ( !*heapOut )
    return 1;
  ArIter_readDataAndNext(*heapOut, &hd, &archv->_iter);
  return 0;
}

void SmartArchive_continueNoData(SmartArchive* archv)
{
  ArIterFileHeader hd;
  ArIter_beginIter(&hd, &archv->_iter);
  ArIter_noDataAndNext(&hd, &archv->_iter);
}

int SmartArchive_findNext(SmartArchive* archv, void** heapOut, size_t* sizeOut, const char * fileName)
{
  char * nam;
  while ( (nam = SmartArchive_nextFileNameHeap(archv) ) )
  {
    bool match = !strcmp(nam, fileName);
    free(nam);

    if ( match ) {
      SmartArchive_continueWithData(heapOut, sizeOut, archv);
      return 0;
    }

    SmartArchive_continueNoData(archv);
  }

  return 1;
}
