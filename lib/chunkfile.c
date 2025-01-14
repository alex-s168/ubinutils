#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "utils.h"
#include "chunkfile.h"

static uint32_t CHUNK_FILE_MAGIC     = 0xC3CBC6C5;
static uint32_t CHUNK_FILE_REV_MAGIC = 0xC5C6CBC3;

void ChunkFile_close(ChunkFile* file)
{
    if (file->lazy_strtab)
        free(file->lazy_strtab);
    free(file->chunks);
}

ChunkFile_EntHeader * ChunkFile_findHeader(ChunkFile const* file, char const * name)
{
    for (size_t i = 0; i < file->num_chunks; i ++)
    {
        ChunkFile_EntHeader* hdr = &file->chunks[i];

        char s[9];
        memcpy(s, hdr->id, 8);
        s[8] = '\0';

        if ( !strcmp(s, name) ) {
            return hdr;
        }
    }

    return NULL;
}

/** 0 = ok; ownership of fp is taken; fp is rewinded() ! */
int ChunkFile_open(ChunkFile* out, FILE* fp)
{
    out->file = fp;
    out->lazy_strtab = NULL;

    ChunkFile_Header header;
    rewind(fp);
    if ( fread(&header, sizeof(ChunkFile_Header), 1, fp) != 1 ) {
        return 1;
}

    bool swap;
    if ( header.magic == CHUNK_FILE_MAGIC ) {
        swap = false;
    } else if ( header.magic == CHUNK_FILE_REV_MAGIC ) {
        swap = true;
    } else {
        return 1;
    }
    out->read_swapped = swap;

    if ( swap ) {
        endianess_swap(header.max_chunks);
        endianess_swap(header.num_chunks);
    }

    out->num_chunks = header.num_chunks;
    out->chunks = malloc(sizeof(ChunkFile_EntHeader) * out->num_chunks);
    if (!out->chunks) {
        return 1;
    }

    if ( fread(out->chunks, sizeof(ChunkFile_EntHeader), out->num_chunks, fp) != out->num_chunks ) {
        free(out->chunks);
        return 1;
    }

    if ( swap ) {
        for (size_t i = 0; i < out->num_chunks; i ++) {
            ChunkFile_EntHeader* p = &out->chunks[i];
            endianess_swap(p->size);
            endianess_swap(p->file_offset);
        }
    }

    out->headers.obj_head = ChunkFile_findHeader(out, "OBJ_HEAD");
    out->headers.obj_area = ChunkFile_findHeader(out, "OBJ_AREA");
    out->headers.obj_identification = ChunkFile_findHeader(out, "OBJ_IDFN");
    out->headers.obj_symtab = ChunkFile_findHeader(out, "OBJ_SYMT");
    out->headers.obj_strtab = ChunkFile_findHeader(out, "OBJ_STRT");

    return 0;
}

char * ChunkFile_readChunk(ChunkFile * cf, ChunkFile_EntHeader * hd)
{
    char * out = malloc(hd->size);
    if ( !out )
        return NULL;

    fseek(cf->file, hd->file_offset, SEEK_SET);
    if ( fread(out, 1, hd->size, cf->file) != hd->size )
    {
        free(out);
        return NULL;
    }

    return out;
}

char * ChunkFile_readIdentHeap(ChunkFile * cf)
{
    if ( !cf->headers.obj_identification )
        return NULL;

    return ChunkFile_readChunk(cf, cf->headers.obj_identification);
}

char const* ChunkFile_getStr(ChunkFile * cf, uint32_t strid)
{
    if ( !cf->headers.obj_strtab )
        return NULL;

    if ( !cf->lazy_strtab )
    {
        cf->lazy_strtab = ChunkFile_readChunk(cf, cf->headers.obj_strtab);

        if ( !cf->lazy_strtab )
            return NULL;
    }

    if ( strid >= cf->headers.obj_strtab->size )
        return NULL;

    return cf->lazy_strtab + strid;
}
