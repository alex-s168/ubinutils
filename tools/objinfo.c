#include "../lib/aof.h"
#include <stdlib.h>
#include <assert.h>

int main(int argc, char ** argv)
{
    if (argc != 2) return 1;
    FILE* f = fopen(argv[1], "rb");
    if (!f) return 1;
    ChunkFile ch = {0};
    if ( ChunkFile_open(&ch, f) != 0 ) {
        printf("doesn't seem like a AOF file\n");
        return 1;
    }
    Aof aof = {0};
    if ( Aof_read(&aof, &ch) != 0 ) {
        printf("doesn't seem like a AOF file\n");
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

