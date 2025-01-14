#ifndef _AOF_H
#define _AOF_H

#include "utils.h"
#include "chunkfile.h"

#ifndef PACKED
# define PACKED __attribute__ ((packed))
#endif 

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

char* AofAreaAttrib_str(AofAreaAttrib a);

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

char * AofSymAttr_str(AofSymAttr a);

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

void Aof_free(Aof* aof);

int Aof_read(Aof* out, ChunkFile const* ch);

/** 0 = err */
size_t Aof_areaFileOffset(ChunkFile* cf, Aof* aof, size_t idx);

AofReloc const* Aof_readAreaRelocs(ChunkFile* cf, Aof* aof, size_t area_idx);

#define SYMTAB_N_BUCK (16)

typedef struct {
    size_t namelen;
    char const * namep;
    AofSym *p;
} SymtabEnt;

typedef struct {
    SymtabEnt * ents;
    size_t      nents;
} SymtabBucket;

typedef struct {
    Aof aof;
    ChunkFile ch;

    SymtabBucket buckets[SYMTAB_N_BUCK];
} AofObj;

/** 0 == ok */
int Symtab_add(AofObj* out, SymtabEnt ent);

void AofObj_close(AofObj* obj);

int AofObj_open(AofObj* out, FILE* file);

#endif
