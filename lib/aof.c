#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "utils.h"

#ifndef PACKED
# define PACKED __attribute__ ((packed))
#endif 

static uint32_t CHUNK_FILE_MAGIC     = 0xC3CBC6C5;
static uint32_t CHUNK_FILE_REV_MAGIC = 0xC5C6CBC3;

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

void ChunkFile_close(ChunkFile* file)
{
    if (file->lazy_strtab)
        free(file->lazy_strtab);

    free(file->file);
    fclose(file->file);
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
        fclose(fp);
        return 1;
    }

    bool swap;
    if ( header.magic == CHUNK_FILE_MAGIC ) {
        swap = false;
    } else if ( header.magic == CHUNK_FILE_REV_MAGIC ) {
        swap = true;
    } else {
        fclose(fp);
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
        fclose(fp);
        return 1;
    }

    if ( fread(out->chunks, sizeof(ChunkFile_EntHeader), out->num_chunks, fp) != out->num_chunks ) {
        fclose(fp);
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

typedef struct {
    uint32_t file_type;
    uint32_t version_id;
    uint32_t num_areas;
    uint32_t num_syms;
    /** 1-based ! */
    uint32_t entry_area;
    /** relatvie to entry_area */
    uint32_t entry_offset;
} PACKED AofHeader;

typedef struct {
    uint32_t name;
    uint32_t attributes; // see section 15.3
    uint32_t size;
    uint32_t num_relocs;
    uint32_t base_addr;
} PACKED AofAreaHeader;

typedef struct {
    AofHeader header;
    AofAreaHeader * areas;
} Aof;

void Aof_free(Aof* aof)
{
    free(aof->areas);
}

int Aof_read(Aof* out, ChunkFile const* ch)
{
    if ( !ch->headers.obj_head )
        return 1;

    fseek(ch->file, ch->headers.obj_head->file_offset, SEEK_SET);
    if ( fread(&out->header, sizeof(AofHeader), 1, ch->file) != 1 )
        return 1;

    if ( ch->read_swapped ) 
    {
        endianess_swap(out->header.file_type);
        endianess_swap(out->header.version_id);
        endianess_swap(out->header.num_areas);
        endianess_swap(out->header.num_syms);
        endianess_swap(out->header.entry_area);
        endianess_swap(out->header.entry_offset);
    }

    out->areas = malloc(sizeof(AofAreaHeader) * out->header.num_areas);
    if ( !out->areas )
        return 1;

    if ( fread(out->areas, sizeof(AofAreaHeader), out->header.num_areas, ch->file) != out->header.num_areas )
    {
        free(out->areas);
        return 1;
    }

    if ( ch->read_swapped ) {
        for (size_t i = 0; i < out->header.num_areas; i ++) {
            AofAreaHeader* h = &out->areas[i];
            endianess_swap(h->name);
            endianess_swap(h->attributes);
            endianess_swap(h->size);
            endianess_swap(h->num_relocs);
            endianess_swap(h->base_addr);
        }
    }

    return 0;
}

static char const* ChunkFile_getStr(ChunkFile * cf, uint32_t strid)
{
    if ( !cf->headers.obj_strtab )
        return NULL;

    if ( !cf->lazy_strtab )
    {
        cf->lazy_strtab = (char*) malloc(cf->headers.obj_strtab->size);
        fseek(cf->file, cf->headers.obj_strtab->file_offset, SEEK_SET);
        if ( fread(cf->lazy_strtab, 1, cf->headers.obj_strtab->size, cf->file) != cf->headers.obj_strtab->size ) 
        {
            free(cf->lazy_strtab);
            cf->lazy_strtab = NULL;
            return NULL;
        }
    }

    if ( strid >= cf->headers.obj_strtab->size )
        return NULL;

    return cf->lazy_strtab + strid;
}

int main(int argc, char ** argv)
{
    if (argc != 2) return 1;
    FILE* f = fopen(argv[1], "rb");
    if (!f) return 1;

    ChunkFile ch = {0};
    if ( ChunkFile_open(&ch, f) != 0 )
        return 1;

    Aof aof = {0};
    if ( Aof_read(&aof, &ch) != 0 ) {
        ChunkFile_close(&ch);
        return 1;
    }

    for (size_t i = 0; i < aof.header.num_areas; i ++) {
        AofAreaHeader* h = &aof.areas[i];
        assert(ch.headers.obj_strtab);
        char const* name = ChunkFile_getStr(&ch, h->name);
        puts(name);
    }

    Aof_free(&aof);

    ChunkFile_close(&ch);

    return 0;
}
