#ifndef _AR_H 
#define _AR_H

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef PACKED
# define PACKED __attribute__ ((packed))
#endif 

typedef struct {
  char filename[16];
  char decimal_file_modific_timestamp[12];
  char decimal_ownder_id[6];
  char decimal_group_id[6];
  char octal_file_mode[8];
  char decimal_file_size[10];
  char padding[2]; // must be 0x60, 0x0A
} PACKED Ar_FileHeader;

typedef struct {
  char filename[16];
  size_t fileSize;
} ArIterFileHeader;

typedef struct {
  FILE* file;
} ArIter;

typedef struct {
  ArIter _iter;
  void* exFileNames;
} SmartArchive;

int ArIter_open(ArIter* dest, FILE* file /** will not close */);
void ArIter_close(ArIter* i);
void ArIter_rewind(ArIter* i);
bool ArIter_hasNext(ArIter* i);
void ArIter_beginIter(ArIterFileHeader* hdest, ArIter* i);
void ArIter_rewindBeginIter(ArIter* i);
void ArIter_readDataAndNext(void* buf, ArIterFileHeader const* hd, ArIter* i);
void ArIter_noDataAndNext(ArIterFileHeader const* hd, ArIter* i);
int ArIter_findNext(void** heapOut, size_t* sizeOut, ArIter* i, char const searchNam[16]);

int SmartArchive_open(SmartArchive* dest, FILE* file /** will not close */);
void SmartArchive_close(SmartArchive* archv);
void SmartArchive_rewind(SmartArchive* archv);
char * SmartArchive_nextFileNameHeap(SmartArchive* archv);
int SmartArchive_continueWithData(void** heapOut, size_t* sizeOut, SmartArchive* archv);
void SmartArchive_continueNoData(SmartArchive* archv);

#endif


