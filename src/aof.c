#include "ubu/aof.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *AofAreaAttrib_str(AofAreaAttrib a) {
  char buf[256];
  buf[0] = '\0';

  if (a & AofAreaAttrib_ABSOLUTE)
    strcat(buf, "ABS,");
  if (a & AofAreaAttrib_CODE)
    strcat(buf, "CODE,");
  if (a & AofAreaAttrib_CBD)
    strcat(buf, "CBD,");
  if (a & AofAreaAttrib_CBR)
    strcat(buf, "CBR,");
  if (a & AofAreaAttrib_ZEROI)
    strcat(buf, "ZI,");
  if (a & AofAreaAttrib_R_ONLY)
    strcat(buf, "RO,");
  if (a & AofAreaAttrib_PIE)
    strcat(buf, "PIE,");
  if (a & AofAreaAttrib_DBG_TABS)
    strcat(buf, "DBG,");
  if (a & AofAreaAttrib_32_APCS)
    strcat(buf, "APCS32,");
  if (a & AofAreaAttrib_REE)
    strcat(buf, "REE,");
  if (a & AofAreaAttrib_EX_FP)
    strcat(buf, "EXFP,");
  if (a & AofAreaAttrib_NOSPP)
    strcat(buf, "NOSPP,");
  if (a & AofAreaAttrib_THUMB)
    strcat(buf, "THUMB,");
  if (a & AofAreaAttrib_HW_INST)
    strcat(buf, "HW,");
  if (a & AofAreaAttrib_IW)
    strcat(buf, "IW,");

  return strdup(buf);
}

char *AofSymAttr_str(AofSymAttr a) {
  char out[256];
  out[0] = '\0';

  if (a & AofSymAttr_DEFINE)
    strcat(out, "DEF,");
  if (a & AofSymAttr_GLOBAL)
    strcat(out, "PUB,");
  if (a & AofSymAttr_ABS)
    strcat(out, "ABS,");
  if (a & AofSymAttr_CI)
    strcat(out, "CI,");
  if (a & AofSymAttr_WEAK)
    strcat(out, "WEAK,");
  if (a & AofSymAttr_COMMON)
    strcat(out, "C,");
  if (a & AofSymAttr_DATUM)
    strcat(out, "DATUM,");
  if (a & AofSymAttr_FP_REG_ARGS)
    strcat(out, "FP_REG_ARGS,");
  if (a & AofSymAttr_THUMB)
    strcat(out, "THUMB,");

  return strdup(out);
}

void Aof_free(Aof *aof) {
  for (size_t i = 0; i < aof->header.num_areas; i++) {
    AofAreaData *data = &aof->area_data[i];
    if (data->relocs)
      free(data->relocs);
  }
  free(aof->area_data);
  free(aof->areas);
  free(aof->syms);
}

int Aof_read(Aof *out, ChunkFile const *ch) {
  if (!ch->headers.obj_head)
    return 1;

  fseek(ch->file, ch->headers.obj_head->file_offset, SEEK_SET);
  if (fread(&out->header, sizeof(AofHeader), 1, ch->file) != 1)
    return 1;

  if (ch->read_swapped) {
    endianess_swap(out->header.file_type);
    endianess_swap(out->header.version_id);
    endianess_swap(out->header.num_areas);
    endianess_swap(out->header.num_syms);
    endianess_swap(out->header.entry_area);
    endianess_swap(out->header.entry_offset);
  }

  out->areas = malloc(sizeof(AofAreaHeader) * out->header.num_areas);
  if (!out->areas)
    return 1;

  if (fread(out->areas, sizeof(AofAreaHeader), out->header.num_areas,
            ch->file) != out->header.num_areas) {
    free(out->areas);
    return 1;
  }

  if (ch->read_swapped) {
    for (size_t i = 0; i < out->header.num_areas; i++) {
      AofAreaHeader *h = &out->areas[i];
      endianess_swap(h->name);
      endianess_swap(h->attributes);
      endianess_swap(h->size);
      endianess_swap(h->num_relocs);
      endianess_swap(h->base_addr);
    }
  }

  out->syms = malloc(sizeof(AofSym) * out->header.num_syms);
  if (!out->syms) {
    free(out->areas);
    return 1;
  }

  if (ch->headers.obj_symtab->file_offset) {
    fseek(ch->file, ch->headers.obj_symtab->file_offset, SEEK_SET);

    if (fread(out->syms, sizeof(AofSym), out->header.num_syms, ch->file) !=
        out->header.num_syms) {
      free(out->areas);
      free(out->syms);
      return 1;
    }

    if (ch->read_swapped) {
      for (size_t i = 0; i < out->header.num_syms; i++) {
        AofSym *s = &out->syms[i];
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
  if (!out->area_data) {
    free(out->areas);
    free(out->syms);
    return 1;
  }

  return 0;
}

/** 0 = err */
size_t Aof_areaFileOffset(ChunkFile *cf, Aof *aof, size_t idx) {
  if (cf->headers.obj_area == NULL)
    return 0;

  size_t off = cf->headers.obj_area->file_offset;

  for (size_t i = 0; i < idx; i++) {
    off += aof->areas[i].size;
    off += aof->areas[i].num_relocs * sizeof(AofReloc);
  }

  return off;
}

AofReloc const *Aof_readAreaRelocs(ChunkFile *cf, Aof *aof, size_t area_idx) {
  if (aof->area_data[area_idx].relocs)
    return aof->area_data[area_idx].relocs;

  AofAreaHeader *ahp = &aof->areas[area_idx];

  AofReloc *relocs = malloc(sizeof(AofReloc) * ahp->num_relocs);
  if (!relocs)
    return NULL;

  size_t off = Aof_areaFileOffset(cf, aof, area_idx);
  if (off == 0)
    return NULL;
  fseek(cf->file, off + ahp->size, SEEK_SET);

  if (fread(relocs, sizeof(AofReloc), ahp->num_relocs, cf->file) !=
      ahp->num_relocs) {
    free(relocs);
    return NULL;
  }

  if (cf->read_swapped) {
    for (size_t i = 0; i < ahp->num_relocs; i++) {
      endianess_swap(relocs[i].flags);
      endianess_swap(relocs[i].offset);
    }
  }

  aof->area_data[area_idx].relocs = relocs;
  return relocs;
}

/** 0 == ok */
int Symtab_add(AofObj *out, SymtabEnt ent) {
  size_t buck =
      hash((unsigned char const *)ent.namep, ent.namelen) % AOF_SYMTAB_N_BUCK;

  SymtabEnt *new = realloc(out->buckets[buck].ents,
                           (out->buckets[buck].nents + 1) * sizeof(SymtabEnt));
  if (new == NULL)
    return 1;

  new[out->buckets[buck].nents++] = ent;

  out->buckets[buck].ents = new;

  return 0;
}

void AofObj_close(AofObj *obj) {
  for (size_t i = 0; i < AOF_SYMTAB_N_BUCK; i++) {
    free(obj->buckets[i].ents);
  }

  Aof_free(&obj->aof);
  ChunkFile_close(&obj->ch);
}

/** takes ownership of file */
int AofObj_open(AofObj *out, FILE *file) {
  memset(out, 0, sizeof(AofObj));

  if (ChunkFile_open(&out->ch, file) != 0)
    return 1;

  if (Aof_read(&out->aof, &out->ch) != 0) {
    ChunkFile_close(&out->ch);
    return 1;
  }

  for (size_t i = 0; i < out->aof.header.num_syms; i++) {
    AofSym *s = &out->aof.syms[i];
    char const *name = ChunkFile_getStr(&out->ch, s->name);
    if (name == NULL) {
      AofObj_close(out);
      return 1;
    }
    Symtab_add(out, (SymtabEnt){
                        .namelen = strlen(name),
                        .namep = name,
                        .p = s,
                    });
  }

  return 0;
}
