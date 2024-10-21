#ifndef _PE_H 
#define _PE_H

// https://learn.microsoft.com/en-us/windows/win32/debug/pe-format 

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef PACKED
# define PACKED __attribute__ ((packed))
#endif 

typedef struct {
  uint16_t machine;
  uint16_t numSections;
  uint32_t timeDateStamp;
  uint32_t fileOffToCoffSymTable;
  uint32_t numCoffSym;
  uint16_t optHeaderSize;
  uint16_t characteristics;
} PACKED CoffHeader;

typedef enum : uint8_t {
  IMAGE_SYM_CLASS_END_OF_FUNCTION = 0xFF, // A special symbol that represents the end of function, for debugging purposes.
  IMAGE_SYM_CLASS_NULL = 0, // No assigned storage class.
  IMAGE_SYM_CLASS_AUTOMATIC = 1, // The automatic (stack) variable. The Value field specifies the stack frame offset.
  IMAGE_SYM_CLASS_EXTERNAL = 2, // A value that Microsoft tools use for external symbols. The Value field indicates the size if the section number is IMAGE_SYM_UNDEFINED (0). If the section number is not zero, then the Value field specifies the offset within the section.
  IMAGE_SYM_CLASS_STATIC = 3, // The offset of the symbol within the section. If the Value field is zero, then the symbol represents a section name.
  IMAGE_SYM_CLASS_REGISTER = 4, // A register variable. The Value field specifies the register number.
  IMAGE_SYM_CLASS_EXTERNAL_DEF = 5, // A symbol that is defined externally.
  IMAGE_SYM_CLASS_LABEL = 6, // A code label that is defined within the module. The Value field specifies the offset of the symbol within the section.
  IMAGE_SYM_CLASS_UNDEFINED_LABEL = 7, // A reference to a code label that is not defined.
  IMAGE_SYM_CLASS_MEMBER_OF_STRUCT = 8, // The structure member. The Value field specifies the n th member.
  IMAGE_SYM_CLASS_ARGUMENT = 9, // A formal argument (parameter) of a function. The Value field specifies the n th argument.
  IMAGE_SYM_CLASS_STRUCT_TAG = 10, // The structure tag-name entry.
  IMAGE_SYM_CLASS_MEMBER_OF_UNION = 11, // A union member. The Value field specifies the n th member.
  IMAGE_SYM_CLASS_UNION_TAG = 12, // The Union tag-name entry.
  IMAGE_SYM_CLASS_TYPE_DEFINITION = 13, // A Typedef entry.
  IMAGE_SYM_CLASS_UNDEFINED_STATIC = 14, // A static data declaration.
  IMAGE_SYM_CLASS_ENUM_TAG = 15, // An enumerated type tagname entry.
  IMAGE_SYM_CLASS_MEMBER_OF_ENUM = 16, // A member of an enumeration. The Value field specifies the n th member.
  IMAGE_SYM_CLASS_REGISTER_PARAM = 17, // A register parameter.
  IMAGE_SYM_CLASS_BIT_FIELD = 18, // A bit-field reference. The Value field specifies the n th bit in the bit field.
  IMAGE_SYM_CLASS_BLOCK = 100, // A .bb (beginning of block) or .eb (end of block) record. The Value field is the relocatable address of the code location.
  IMAGE_SYM_CLASS_FUNCTION = 101, // A value that Microsoft tools use for symbol records that define the extent of a function: begin function (.bf ), end function ( .ef ), and lines in function ( .lf ). For .lf records, the Value field gives the number of source lines in the function. For .ef records, the Value field gives the size of the function code.
  IMAGE_SYM_CLASS_END_OF_STRUCT = 102, // An end-of-structure entry.
  IMAGE_SYM_CLASS_FILE = 103, // A value that Microsoft tools, as well as traditional COFF format, use for the source-file symbol record. The symbol is followed by auxiliary records that name the file.
  IMAGE_SYM_CLASS_SECTION = 104, // A definition of a section (Microsoft tools use STATIC storage class instead).
  IMAGE_SYM_CLASS_WEAK_EXTERNAL = 105, // A weak external. For more information, see Auxiliary Format 3: Weak Externals.
  IMAGE_SYM_CLASS_CLR_TOKEN = 107, // A CLR token symbol. The name is an ASCII string that consists of the hexadecimal value of the token. For more information, see CLR Token Definition (Object Only).
} CoffSymClass;

typedef struct {
  char name[8];
  uint32_t value;
  uint16_t sectionId;
  uint16_t type;
  CoffSymClass storageClass;
  uint8_t numAuxSyms;
} PACKED CoffSym;

/** or-ed together */ 
typedef enum : uint32_t {
  IMAGE_SCN_TYPE_NO_PAD = 0x00000008, // The section should not be padded to the next boundary. This flag is obsolete and is replaced by IMAGE_SCN_ALIGN_1BYTES. This is valid only for object files.
  IMAGE_SCN_CNT_CODE = 0x00000020, // The section contains executable code.
  IMAGE_SCN_CNT_INITIALIZED_DATA = 0x00000040, // The section contains initialized data.
  IMAGE_SCN_CNT_UNINITIALIZED_DATA = 0x00000080, // The section contains uninitialized data.
  IMAGE_SCN_LNK_OTHER = 0x00000100, // Reserved for future use.
  IMAGE_SCN_LNK_INFO = 0x00000200, // The section contains comments or other information. The .drectve section has this type. This is valid for object files only.
  IMAGE_SCN_LNK_REMOVE = 0x00000800, // The section will not become part of the image. This is valid only for object files.
  IMAGE_SCN_LNK_COMDAT = 0x00001000, // The section contains COMDAT data. For more information, see COMDAT Sections (Object Only). This is valid only for object files.
  IMAGE_SCN_GPREL = 0x00008000, // The section contains data referenced through the global pointer (GP).
  IMAGE_SCN_MEM_PURGEABLE = 0x00020000, // Reserved for future use.
  IMAGE_SCN_MEM_16BIT = 0x00020000, // Reserved for future use.
  IMAGE_SCN_MEM_LOCKED = 0x00040000, // Reserved for future use.
  IMAGE_SCN_MEM_PRELOAD = 0x00080000, // Reserved for future use.
  IMAGE_SCN_ALIGN_1BYTES = 0x00100000, // Align data on a 1-byte boundary. Valid only for object files.
  IMAGE_SCN_ALIGN_2BYTES = 0x00200000, // Align data on a 2-byte boundary. Valid only for object files.
  IMAGE_SCN_ALIGN_4BYTES = 0x00300000, // Align data on a 4-byte boundary. Valid only for object files.
  IMAGE_SCN_ALIGN_8BYTES = 0x00400000, // Align data on an 8-byte boundary. Valid only for object files.
  IMAGE_SCN_ALIGN_16BYTES = 0x00500000, // Align data on a 16-byte boundary. Valid only for object files.
  IMAGE_SCN_ALIGN_32BYTES = 0x00600000, // Align data on a 32-byte boundary. Valid only for object files.
  IMAGE_SCN_ALIGN_64BYTES = 0x00700000, // Align data on a 64-byte boundary. Valid only for object files.
  IMAGE_SCN_ALIGN_128BYTES = 0x00800000, // Align data on a 128-byte boundary. Valid only for object files.
  IMAGE_SCN_ALIGN_256BYTES = 0x00900000, // Align data on a 256-byte boundary. Valid only for object files.
  IMAGE_SCN_ALIGN_512BYTES = 0x00A00000, // Align data on a 512-byte boundary. Valid only for object files.
  IMAGE_SCN_ALIGN_1024BYTES = 0x00B00000, // Align data on a 1024-byte boundary. Valid only for object files.
  IMAGE_SCN_ALIGN_2048BYTES = 0x00C00000, // Align data on a 2048-byte boundary. Valid only for object files.
  IMAGE_SCN_ALIGN_4096BYTES = 0x00D00000, // Align data on a 4096-byte boundary. Valid only for object files.
  IMAGE_SCN_ALIGN_8192BYTES = 0x00E00000, // Align data on an 8192-byte boundary. Valid only for object files.
  IMAGE_SCN_LNK_NRELOC_OVFL = 0x01000000, // The section contains extended relocations.
  IMAGE_SCN_MEM_DISCARDABLE = 0x02000000, // The section can be discarded as needed.
  IMAGE_SCN_MEM_NOT_CACHED = 0x04000000, // The section cannot be cached.
  IMAGE_SCN_MEM_NOT_PAGED = 0x08000000, // The section is not pageable.
  IMAGE_SCN_MEM_SHARED = 0x10000000, // The section can be shared in memory.
  IMAGE_SCN_MEM_EXECUTE = 0x20000000, // The section can be executed as code.
  IMAGE_SCN_MEM_READ = 0x40000000, // The section can be read.
  IMAGE_SCN_MEM_WRITE = 0x80000000, // The section can be written to.
} CoffSectionCharacteristics;

typedef struct {
  char name[8];
  uint32_t virtualSize;
  uint32_t virtualAddress;
  uint32_t dataUz;
  uint32_t fileDataOffset;
  uint32_t fileRelocsOffset;
  uint32_t fileLinenumsOffset;
  uint16_t numRelocs;
  uint16_t numLinenums;
  CoffSectionCharacteristics characteristics;
} PACKED PeSection;

typedef struct {
  char name[8];
  uint32_t virtualSize;
  uint32_t virtualAddress;
  uint32_t dataUz;
  uint32_t fileDataOffset;
  uint32_t fileRelocsOffset;
  uint32_t fileLinenumsOffset;
  uint32_t numRelocs;
  uint32_t numLinenums;
  CoffSectionCharacteristics characteristics;
  char _pad[4];
} PACKED CoffSection;

void ToPeSection(PeSection* dst, CoffSection const* src);

typedef struct {
  CoffHeader header;
  char* heapStrTab;
  FILE* file;
  bool isCoff;
  void* sections;
} OpPe;

static void* OpPe_getSectionPtr(OpPe const* pe, size_t idx) {
  return pe->isCoff
      ? (void*) &((CoffSection*) pe->sections)[idx]
      : (void*) &((PeSection*) pe->sections)[idx]
      ;
}

#define OpPe_sectionMem(pe, sptr, wantty, member) \
    (pe->isCoff \
    ? (wantty) ((CoffSection*) sptr)->member \
    : (wantty) ((PeSection*) sptr)->member)

static void OpPe_getPeSection(PeSection* dest, OpPe const* pe, size_t idx) {
  if ( pe->isCoff ) {
    ToPeSection(dest, &((CoffSection*) pe->sections)[idx]);
  } else {
    *dest = ((PeSection*) pe->sections)[idx];
  }
}

const char* CoffSym_name(CoffSym const* sym, OpPe* pe);
void OpPe_rewindToSyms(OpPe* pe);
void OpPe_nextSym(CoffSym* out, OpPe* pe);
void OpPe_close(OpPe* pe);
int OpPe_open(OpPe* dest, FILE* file);

#endif 
