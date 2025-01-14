#ifndef _CHUNKFILE_H
#define _CHUNKFILE_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "utils.h"

#ifndef PACKED
# define PACKED __attribute__ ((packed))
#endif 

typedef char ChunkFile_ChunkID[8];

typedef struct
{
    uint32_t magic;
    uint32_t max_chunks;
    uint32_t num_chunks;
} PACKED ChunkFile_Header;

typedef struct
{
    ChunkFile_ChunkID id;
    /** zero if unused */
    uint32_t file_offset;
    uint32_t size;
} PACKED ChunkFile_EntHeader;

typedef struct
{
    bool read_swapped;
    FILE* file;
    uint32_t num_chunks;
    ChunkFile_EntHeader * chunks;

    struct {
        ChunkFile_EntHeader * obj_head;
        ChunkFile_EntHeader * obj_area;
        ChunkFile_EntHeader * obj_identification;
        ChunkFile_EntHeader * obj_symtab;
        ChunkFile_EntHeader * obj_strtab;
    } headers;

    char * lazy_strtab;
} ChunkFile;

void ChunkFile_close(ChunkFile* file);

ChunkFile_EntHeader * ChunkFile_findHeader(ChunkFile const* file, char const * name);

/** 0 = ok; ownership of fp is taken; fp is rewinded() ! */
int ChunkFile_open(ChunkFile* out, FILE* fp);

char * ChunkFile_readChunk(ChunkFile * cf, ChunkFile_EntHeader * hd);

char * ChunkFile_readIdentHeap(ChunkFile * cf);

char const* ChunkFile_getStr(ChunkFile * cf, uint32_t strid);

#endif
