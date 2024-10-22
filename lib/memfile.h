#ifndef _MEMFILE_H
#define _MEMFILE_H

#include <stddef.h>
#include <stdio.h>

#ifdef _WIN32 

FILE* memFileOpenReadOnly(void* data, size_t len);

#else 

static FILE* memFileOpenReadOnly(void* data, size_t len) {
    return fmemopen(data, len, "r");
}

#endif 

#endif 
