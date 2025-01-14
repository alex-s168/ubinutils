#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
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
    free(file->chunks);
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

typedef enum : uint32_t
{
    AofAreaAttrib_ABSOLUTE = 0x00000100 /** is absolute? */,
    AofAreaAttrib_CODE     = 0x00000200 /** is code? */,
    AofAreaAttrib_CBD      = 0x00000400 /** common block definition */,
    AofAreaAttrib_CBR      = 0x00000800 /** common block reference */,
    AofAreaAttrib_ZEROI    = 0x00001000 /** zero initialized */,
    AofAreaAttrib_R_ONLY   = 0x00002000,
    AofAreaAttrib_PIE      = 0x00004000 /** position independent */,
    AofAreaAttrib_DBG_TABS = 0x00008000 /** debugging tables */,

    // only for code areas:
    AofAreaAttrib_32_APCS  = 0x00010000 /** compiles with 32 bit APCS */,
    AofAreaAttrib_REE      = 0x00020000 /** reentrant code */,
    AofAreaAttrib_EX_FP    = 0x00040000 /** extended FP instrctuon set */,
    AofAreaAttrib_NOSPP    = 0x00080000 /** no software stack protection */,
    AofAreaAttrib_THUMB    = 0x00100000 /** all relocations are of thumb code */,
    AofAreaAttrib_HW_INST  = 0x00200000 /** may contain ARM half-word instructions */,
    AofAreaAttrib_IW       = 0x00400000 /** suitable for ARM/THUMB interworking */,
} AofAreaAttrib;

static char* AofAreaAttrib_str(AofAreaAttrib a) 
{
    char buf[256];
    buf[0] = '\0';

    if (a & AofAreaAttrib_ABSOLUTE) strcat(buf, "ABS,");
    if (a & AofAreaAttrib_CODE) strcat(buf, "CODE,");
    if (a & AofAreaAttrib_CBD) strcat(buf, "CBD,");
    if (a & AofAreaAttrib_CBR) strcat(buf, "CBR,");
    if (a & AofAreaAttrib_ZEROI) strcat(buf, "ZI,");
    if (a & AofAreaAttrib_R_ONLY) strcat(buf, "RO,");
    if (a & AofAreaAttrib_PIE) strcat(buf, "PIE,");
    if (a & AofAreaAttrib_DBG_TABS) strcat(buf, "DBG,");
    if (a & AofAreaAttrib_32_APCS) strcat(buf, "APCS32,");
    if (a & AofAreaAttrib_REE) strcat(buf, "REE,");
    if (a & AofAreaAttrib_EX_FP) strcat(buf, "EXFP,");
    if (a & AofAreaAttrib_NOSPP) strcat(buf, "NOSPP,");
    if (a & AofAreaAttrib_THUMB) strcat(buf, "THUMB,");
    if (a & AofAreaAttrib_HW_INST) strcat(buf, "HW,");
    if (a & AofAreaAttrib_IW) strcat(buf, "IW,");

    return strdup(buf);
}

typedef struct {
    uint32_t name;
    AofAreaAttrib attributes;
    uint32_t size;
    uint32_t num_relocs;
    uint32_t base_addr;
} PACKED AofAreaHeader;

#define AofAreaHeader_alignment(hp) \
    ((hp)->attributes & 0xFF)

typedef enum : uint32_t
{
    AofSymAttr_DEFINE = 0x00000001 /** symbol is defined in this file */,
    AofSymAttr_GLOBAL = 0x00000002 /** global symbol */,
    AofSymAttr_ABS    = 0x00000004 /** absolute */,
    AofSymAttr_CI     = 0x00000008 /** case insensitive */, // TODO: make sure that linker takes care of that
    AofSymAttr_WEAK   = 0x00000010 /** weak definition */,
    AofSymAttr_COMMON = 0x00000040 ,

    // code symbols only:
    AofSymAttr_DATUM       = 0x00000100 /** symbol points to a datum instead of code */,
    AofSymAttr_FP_REG_ARGS = 0x00000200 /** FP args are in FP registers;
                                            linker can only link two symbols together, 
                                            if both have equal value for this. */, // TODO: make sure that linker takes care of that
    AofSymAttr_THUMB       = 0x00001000 /** THUMB symbol? */,
} AofSymAttr;

char * AofSymAttr_str(AofSymAttr a)
{
    char out[256];
    out[0] = '\0';

    if (a & AofSymAttr_DEFINE) strcat(out, "DEF,");
    if (a & AofSymAttr_GLOBAL) strcat(out, "PUB,");
    if (a & AofSymAttr_ABS) strcat(out, "ABS,");
    if (a & AofSymAttr_CI) strcat(out, "CI,");
    if (a & AofSymAttr_WEAK) strcat(out, "WEAK,");
    if (a & AofSymAttr_COMMON) strcat(out, "C,");
    if (a & AofSymAttr_DATUM) strcat(out, "DATUM,");
    if (a & AofSymAttr_FP_REG_ARGS) strcat(out, "FP_REG_ARGS,");
    if (a & AofSymAttr_THUMB) strcat(out, "THUMB,");

    return strdup(out);
}

typedef struct {
    uint32_t name;
    AofSymAttr attribs;
    uint32_t value /** if is absolute: absolute address,
                       if is common sym: byte length of referenced common area,
                       otherwise: offset from referenced area */;
    uint32_t ref_area /** if non absolute defining occurance: name of area in which sym is defined */;
} PACKED AofSym;

typedef enum {
    AofFT_8  = 0b00,
    AofFT_16 = 0b01,
    AofFT_32 = 0b10,
    AofFT_INSTR = 0b11 /** field to reloc is instruction (-sequence) */,
} AofFT;

typedef struct {
    uint32_t offset;
    uint32_t flags;
} PACKED AofReloc;

static AofFT AofReloc_FT(AofReloc r) {
    return (AofFT) ((r.flags >> 24) & 0b11);
}
static bool AofReloc_IsThumbInstr(AofReloc r) {
    return AofReloc_FT(r) == AofFT_INSTR && (r.offset & 1) == 0;
}
static bool AofReloc_A(AofReloc r) {
    return ((r.flags >> 27) & 1) == 1;
}
static uint32_t AofReloc_SID(AofReloc r) {
    return r.flags & 0xFFFFFF;
}
/** the amount of instructions the linker needs to modify; 0 means automatic */
static uint8_t AofReloc_II(AofReloc r) {
    return (r.flags >> 29) & 0b11;
}
static bool AofReloc_R(AofReloc r) {
    return ((r.flags >> 26) & 1) == 1;
}
static bool AofReloc_B(AofReloc r) {
    return ((r.flags >> 28) & 1) == 1;
}

typedef struct {
    /** lazy; check header for length */
    AofReloc* relocs;
} AofAreaData;

typedef struct {
    AofHeader header;
    AofAreaHeader * areas;
    AofAreaData * area_data;

    AofSym * syms;
} Aof;

void Aof_free(Aof* aof)
{
    for (size_t i = 0; i < aof->header.num_areas; i ++)
    {
        AofAreaData* data = &aof->area_data[i];
        if ( data->relocs ) 
            free(data->relocs);
    }
    free(aof->area_data);
    free(aof->areas);
    free(aof->syms);
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

    out->syms = malloc(sizeof(AofSym) * out->header.num_syms);
    if ( !out->syms) {
        free(out->areas);
        return 1;
    }

    if ( ch->headers.obj_symtab->file_offset )
    {
        fseek(ch->file, ch->headers.obj_symtab->file_offset, SEEK_SET);
        
        if ( fread(out->syms, sizeof(AofSym), out->header.num_syms, ch->file) != out->header.num_syms )
        {
            free(out->areas);
            free(out->syms);
            return 1;
        }

        if ( ch->read_swapped ) {
            for (size_t i = 0; i < out->header.num_syms; i ++) {
                AofSym* s = &out->syms[i];
                endianess_swap(s->name);
                endianess_swap(s->attribs);
                endianess_swap(s->value);
                endianess_swap(s->ref_area);
            }
        }
    } else {
        out->syms = NULL;
    }

    out->area_data = calloc(out->header.num_areas, sizeof(AofAreaData));
    if ( !out->area_data ) {
        free(out->areas);
        free(out->syms);
        return 1;
    }

    return 0;
}

/** 0 = err */
size_t Aof_areaFileOffset(ChunkFile* cf, Aof* aof, size_t idx)
{
    if ( cf->headers.obj_area == NULL )
        return 0;

    size_t off = cf->headers.obj_area->file_offset;

    for (size_t i = 0; i < idx; i ++) {
        off += aof->areas[i].size;
        off += aof->areas[i].num_relocs * sizeof(AofReloc);
    }

    return off;
}

AofReloc const* Aof_readAreaRelocs(ChunkFile* cf, Aof* aof, size_t area_idx)
{
    if ( aof->area_data[area_idx].relocs )
        return aof->area_data[area_idx].relocs;

    AofAreaHeader *ahp = &aof->areas[area_idx];

    AofReloc* relocs = malloc(sizeof(AofReloc) * ahp->num_relocs);
    if ( !relocs )
        return NULL;

    size_t off = Aof_areaFileOffset(cf, aof, area_idx);
    if ( off == 0 )
        return NULL;
    fseek(cf->file, off + ahp->size, SEEK_SET);

    if ( fread(relocs, sizeof(AofReloc), ahp->num_relocs, cf->file) != ahp->num_relocs )
    {
        free(relocs);
        return NULL;
    }

    if ( cf->read_swapped ) {
        for (size_t i = 0; i < ahp->num_relocs; i ++)
        {
            endianess_swap(relocs[i].flags);
            endianess_swap(relocs[i].offset);
        }
    }

    aof->area_data[area_idx].relocs = relocs;
    return relocs;
}

static char * ChunkFile_readChunk(ChunkFile * cf, ChunkFile_EntHeader * hd)
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

static char * ChunkFile_readIdentHeap(ChunkFile * cf)
{
    if ( !cf->headers.obj_identification )
        return NULL;

    return ChunkFile_readChunk(cf, cf->headers.obj_identification);
}

static char const* ChunkFile_getStr(ChunkFile * cf, uint32_t strid)
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

    printf("compiler identification: %s\n", ChunkFile_readIdentHeap(&ch));

    printf("areas:\n");
    for (size_t i = 0; i < aof.header.num_areas; i ++) {
        AofAreaHeader* h = &aof.areas[i];
        assert(ch.headers.obj_strtab);
        char const* name = ChunkFile_getStr(&ch, h->name);
        char * astr = AofAreaAttrib_str(h->attributes);
        uint8_t align = AofAreaHeader_alignment(h);
        printf("- %s\talign=%u\t%s\n", name, align, astr);
        printf("    relocs (%u):\n", h->num_relocs);
        AofReloc const* relocs = Aof_readAreaRelocs(&ch, &aof, i);
        if ( !relocs )
            printf("    bruh\n");
        else {
            for (size_t r = 0; r < h->num_relocs; r ++)
            {
                AofReloc rel = relocs[r];
                printf("    -");
                if (AofReloc_A(rel)) {
                    uint32_t sidx = AofReloc_SID(rel);
                    char const* name = ChunkFile_getStr(&ch, aof.syms[sidx].name);
                    printf(" symbol: %s", name);
                } else {
                    uint32_t sidx = AofReloc_SID(rel);
                    const char* name = ChunkFile_getStr(&ch, aof.areas[sidx].name);
                    printf(" area: %s", name);
                }
                printf("\n");
            }
        }
        free(astr);
    }

    printf("symbols:\n");
    for (size_t i = 0; i < aof.header.num_syms; i ++) {
        AofSym* s = &aof.syms[i];
        char const* name = ChunkFile_getStr(&ch, s->name);
        printf("- %s\t%s", name, AofSymAttr_str(s->attribs));
        if (s->attribs & AofSymAttr_ABS) {
            printf("\t=0x%X", s->value);
        } else if (s->attribs & AofSymAttr_DEFINE) {
            char const* ref_area_name = ChunkFile_getStr(&ch, s->ref_area);
            printf("\t%s + 0x%X", ref_area_name, s->value);
        }
        printf("\n");
    }

    Aof_free(&aof);

    ChunkFile_close(&ch);

    return 0;
}
