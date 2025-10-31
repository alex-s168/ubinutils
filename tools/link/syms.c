#include "ubu/aof.h"
#include <stdlib.h>

typedef struct {
  AofObj aof;
  char *file_name;
} LinkObj;

void LinkObj_free(LinkObj *obj) {
  fclose(obj->aof.ch.file);
  free(obj->file_name);
  AofObj_close(&obj->aof);
}

/** 0 = ok */
int LinkObj_open(LinkObj *obj, char const *file_name) {
  obj->file_name = strdup(file_name);
  if (!obj->file_name)
    return 1;

  FILE *fp = fopen(file_name, "rb");
  if (!fp) {
    free(obj->file_name);
    return 1;
  }

  if (AofObj_open(&obj->aof, fp) != 0) {
    free(obj->file_name);
    fclose(fp);
    return 1;
  }

  return 0;
}

typedef struct {
  size_t namelen;
  char const *namep;

  LinkObj *src;
  AofSym *p;
} LinkSym;

typedef struct {
  LinkSym *entries;
  size_t len;
} LinkSymBucket;

#define LINK_SYMTAB_NBUCK (32)

typedef struct {
  size_t num_objs;
  LinkObj *objs;

  LinkSymBucket symtab[LINK_SYMTAB_NBUCK];
} Link;

void Link_close(Link *li) {
  for (size_t i = 0; i < LINK_SYMTAB_NBUCK; i++) {
    free(li->symtab[i].entries);
  }

  for (size_t i = 0; i < li->num_objs; i++) {
    LinkObj_free(&li->objs[i]);
  }

  free(li->objs);
}

/** 0 = ok */
int Link_addSym(Link *li, LinkSym sym) {
  size_t buck =
      hash((unsigned char const *)sym.namep, sym.namelen) % LINK_SYMTAB_NBUCK;

  LinkSym *resz = realloc(li->symtab[buck].entries,
                          sizeof(LinkSym) * (li->symtab[buck].len + 1));
  if (!resz)
    return 1;

  li->symtab[buck].entries = resz;

  resz[li->symtab[buck].len++] = sym;

  return 0;
}

LinkSym *Link_findSym(Link *li, char const *name, size_t namelen) {
  size_t buck = hash((unsigned char const *)name, namelen) % LINK_SYMTAB_NBUCK;

  for (size_t i = 0; i < li->symtab[buck].len; i++) {
    LinkSym *sy = &li->symtab[buck].entries[i];

    if (sy->namelen == namelen && !memcmp(sy->namep, name, namelen)) {
      return sy;
    }
  }

  return NULL;
}

// TODO: use across the whole lib and add message enum
typedef void (*LinkErrClbk)(char const *message, bool fatal, void *arg);

void Link_dedeupSyms(Link *li, LinkErrClbk clbk, void *clbkarg) {
  for (size_t bucki = 0; bucki < LINK_SYMTAB_NBUCK; bucki++) {
    LinkSymBucket *buck = &li->symtab[bucki];

    // for each unique symbol name:
    //   seperate list for definitions and references
  }
}
