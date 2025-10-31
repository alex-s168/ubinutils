#ifndef _ELF_H  
#define _ELF_H

// 32: https://refspecs.linuxfoundation.org/elf/elf.pdf
// 64: https://uclibc.org/docs/elf-64-gen.pdf

//  elf file 
// +-------------------------+
// | header                  |
// | (required)              |
// +-------------------------+
// | program header table    |
// | (required for exec)     |
// +-------------------------+
// | section 1               |
// +-------------------------+
// | ...                     |
// +-------------------------+
// | section n               |
// +-------------------------+
// | section header table    |
// | (required for linktime) |
// +-------------------------+

#include <stdint.h>

#if __POINTER_WIDTH__  == 64 
# define ssize_t int64_t
#elif __POINTER_WIDTH__ == 32 
# define ssize_t int32_t 
#elif __POINTER_WIDTH__ == 16
# define ssize_t int16_t 
#endif

#ifndef PACKED
# define PACKED __attribute__ ((packed))
#endif 

typedef uint32_t Elf32_Addr;
typedef uint32_t Elf32_Off;
typedef uint16_t Elf32_Half;
typedef  int32_t Elf32_Sword;
typedef uint32_t Elf32_Word;

typedef uint64_t Elf64_Addr;
typedef uint64_t Elf64_Off;

typedef enum : Elf32_Half {
  /**    no type    */ OT_NONE   = 0,
  /**  relocatable  */ OT_REL    = 1,
  /**  executable   */ OT_EXEC   = 2,
  /**  shared obj   */ OT_DYN    = 3,
  /**   core file   */ OT_CORE   = 4,
  /** proc specific */ OT_LOPROC = 0xff00,
  /** proc specific */ OT_HIPROC = 0xffff,
} Elf_ObjType;

// stolen from LLVM (llvm/include/llvm/BinaryFormat/ELF.h)
typedef enum : Elf32_Half
{
  EM_NONE = 0,           // No machine
  EM_M32 = 1,            // AT&T WE 32100
  EM_SPARC = 2,          // SPARC
  EM_386 = 3,            // Intel 386
  EM_68K = 4,            // Motorola 68000
  EM_88K = 5,            // Motorola 88000
  EM_IAMCU = 6,          // Intel MCU
  EM_860 = 7,            // Intel 80860
  EM_MIPS = 8,           // MIPS R3000
  EM_S370 = 9,           // IBM System/370
  EM_MIPS_RS3_LE = 10,   // MIPS RS3000 Little-endian
  EM_PARISC = 15,        // Hewlett-Packard PA-RISC
  EM_VPP500 = 17,        // Fujitsu VPP500
  EM_SPARC32PLUS = 18,   // Enhanced instruction set SPARC
  EM_960 = 19,           // Intel 80960
  EM_PPC = 20,           // PowerPC
  EM_PPC64 = 21,         // PowerPC64
  EM_S390 = 22,          // IBM System/390
  EM_SPU = 23,           // IBM SPU/SPC
  EM_V800 = 36,          // NEC V800
  EM_FR20 = 37,          // Fujitsu FR20
  EM_RH32 = 38,          // TRW RH-32
  EM_RCE = 39,           // Motorola RCE
  EM_ARM = 40,           // ARM
  EM_ALPHA = 41,         // DEC Alpha
  EM_SH = 42,            // Hitachi SH
  EM_SPARCV9 = 43,       // SPARC V9
  EM_TRICORE = 44,       // Siemens TriCore
  EM_ARC = 45,           // Argonaut RISC Core
  EM_H8_300 = 46,        // Hitachi H8/300
  EM_H8_300H = 47,       // Hitachi H8/300H
  EM_H8S = 48,           // Hitachi H8S
  EM_H8_500 = 49,        // Hitachi H8/500
  EM_IA_64 = 50,         // Intel IA-64 processor architecture
  EM_MIPS_X = 51,        // Stanford MIPS-X
  EM_COLDFIRE = 52,      // Motorola ColdFire
  EM_68HC12 = 53,        // Motorola M68HC12
  EM_MMA = 54,           // Fujitsu MMA Multimedia Accelerator
  EM_PCP = 55,           // Siemens PCP
  EM_NCPU = 56,          // Sony nCPU embedded RISC processor
  EM_NDR1 = 57,          // Denso NDR1 microprocessor
  EM_STARCORE = 58,      // Motorola Star*Core processor
  EM_ME16 = 59,          // Toyota ME16 processor
  EM_ST100 = 60,         // STMicroelectronics ST100 processor
  EM_TINYJ = 61,         // Advanced Logic Corp. TinyJ embedded processor family
  EM_X86_64 = 62,        // AMD x86-64 architecture
  EM_PDSP = 63,          // Sony DSP Processor
  EM_PDP10 = 64,         // Digital Equipment Corp. PDP-10
  EM_PDP11 = 65,         // Digital Equipment Corp. PDP-11
  EM_FX66 = 66,          // Siemens FX66 microcontroller
  EM_ST9PLUS = 67,       // STMicroelectronics ST9+ 8/16 bit microcontroller
  EM_ST7 = 68,           // STMicroelectronics ST7 8-bit microcontroller
  EM_68HC16 = 69,        // Motorola MC68HC16 Microcontroller
  EM_68HC11 = 70,        // Motorola MC68HC11 Microcontroller
  EM_68HC08 = 71,        // Motorola MC68HC08 Microcontroller
  EM_68HC05 = 72,        // Motorola MC68HC05 Microcontroller
  EM_SVX = 73,           // Silicon Graphics SVx
  EM_ST19 = 74,          // STMicroelectronics ST19 8-bit microcontroller
  EM_VAX = 75,           // Digital VAX
  EM_CRIS = 76,          // Axis Communications 32-bit embedded processor
  EM_JAVELIN = 77,       // Infineon Technologies 32-bit embedded processor
  EM_FIREPATH = 78,      // Element 14 64-bit DSP Processor
  EM_ZSP = 79,           // LSI Logic 16-bit DSP Processor
  EM_MMIX = 80,          // Donald Knuth's educational 64-bit processor
  EM_HUANY = 81,         // Harvard University machine-independent object files
  EM_PRISM = 82,         // SiTera Prism
  EM_AVR = 83,           // Atmel AVR 8-bit microcontroller
  EM_FR30 = 84,          // Fujitsu FR30
  EM_D10V = 85,          // Mitsubishi D10V
  EM_D30V = 86,          // Mitsubishi D30V
  EM_V850 = 87,          // NEC v850
  EM_M32R = 88,          // Mitsubishi M32R
  EM_MN10300 = 89,       // Matsushita MN10300
  EM_MN10200 = 90,       // Matsushita MN10200
  EM_PJ = 91,            // picoJava
  EM_OPENRISC = 92,      // OpenRISC 32-bit embedded processor
  EM_ARC_COMPACT = 93,   // ARC International ARCompact processor (old
                         // spelling/synonym: EM_ARC_A5)
  EM_XTENSA = 94,        // Tensilica Xtensa Architecture
  EM_VIDEOCORE = 95,     // Alphamosaic VideoCore processor
  EM_TMM_GPP = 96,       // Thompson Multimedia General Purpose Processor
  EM_NS32K = 97,         // National Semiconductor 32000 series
  EM_TPC = 98,           // Tenor Network TPC processor
  EM_SNP1K = 99,         // Trebia SNP 1000 processor
  EM_ST200 = 100,        // STMicroelectronics (www.st.com) ST200
  EM_IP2K = 101,         // Ubicom IP2xxx microcontroller family
  EM_MAX = 102,          // MAX Processor
  EM_CR = 103,           // National Semiconductor CompactRISC microprocessor
  EM_F2MC16 = 104,       // Fujitsu F2MC16
  EM_MSP430 = 105,       // Texas Instruments embedded microcontroller msp430
  EM_BLACKFIN = 106,     // Analog Devices Blackfin (DSP) processor
  EM_SE_C33 = 107,       // S1C33 Family of Seiko Epson processors
  EM_SEP = 108,          // Sharp embedded microprocessor
  EM_ARCA = 109,         // Arca RISC Microprocessor
  EM_UNICORE = 110,      // Microprocessor series from PKU-Unity Ltd. and MPRC
                         // of Peking University
  EM_EXCESS = 111,       // eXcess: 16/32/64-bit configurable embedded CPU
  EM_DXP = 112,          // Icera Semiconductor Inc. Deep Execution Processor
  EM_ALTERA_NIOS2 = 113, // Altera Nios II soft-core processor
  EM_CRX = 114,          // National Semiconductor CompactRISC CRX
  EM_XGATE = 115,        // Motorola XGATE embedded processor
  EM_C166 = 116,         // Infineon C16x/XC16x processor
  EM_M16C = 117,         // Renesas M16C series microprocessors
  EM_DSPIC30F = 118,     // Microchip Technology dsPIC30F Digital Signal
                         // Controller
  EM_CE = 119,           // Freescale Communication Engine RISC core
  EM_M32C = 120,         // Renesas M32C series microprocessors
  EM_TSK3000 = 131,      // Altium TSK3000 core
  EM_RS08 = 132,         // Freescale RS08 embedded processor
  EM_SHARC = 133,        // Analog Devices SHARC family of 32-bit DSP
                         // processors
  EM_ECOG2 = 134,        // Cyan Technology eCOG2 microprocessor
  EM_SCORE7 = 135,       // Sunplus S+core7 RISC processor
  EM_DSP24 = 136,        // New Japan Radio (NJR) 24-bit DSP Processor
  EM_VIDEOCORE3 = 137,   // Broadcom VideoCore III processor
  EM_LATTICEMICO32 = 138, // RISC processor for Lattice FPGA architecture
  EM_SE_C17 = 139,        // Seiko Epson C17 family
  EM_TI_C6000 = 140,      // The Texas Instruments TMS320C6000 DSP family
  EM_TI_C2000 = 141,      // The Texas Instruments TMS320C2000 DSP family
  EM_TI_C5500 = 142,      // The Texas Instruments TMS320C55x DSP family
  EM_MMDSP_PLUS = 160,    // STMicroelectronics 64bit VLIW Data Signal Processor
  EM_CYPRESS_M8C = 161,   // Cypress M8C microprocessor
  EM_R32C = 162,          // Renesas R32C series microprocessors
  EM_TRIMEDIA = 163,      // NXP Semiconductors TriMedia architecture family
  EM_HEXAGON = 164,       // Qualcomm Hexagon processor
  EM_8051 = 165,          // Intel 8051 and variants
  EM_STXP7X = 166,        // STMicroelectronics STxP7x family of configurable
                          // and extensible RISC processors
  EM_NDS32 = 167,         // Andes Technology compact code size embedded RISC
                          // processor family
  EM_ECOG1 = 168,         // Cyan Technology eCOG1X family
  EM_ECOG1X = 168,        // Cyan Technology eCOG1X family
  EM_MAXQ30 = 169,        // Dallas Semiconductor MAXQ30 Core Micro-controllers
  EM_XIMO16 = 170,        // New Japan Radio (NJR) 16-bit DSP Processor
  EM_MANIK = 171,         // M2000 Reconfigurable RISC Microprocessor
  EM_CRAYNV2 = 172,       // Cray Inc. NV2 vector architecture
  EM_RX = 173,            // Renesas RX family
  EM_METAG = 174,         // Imagination Technologies META processor
                          // architecture
  EM_MCST_ELBRUS = 175,   // MCST Elbrus general purpose hardware architecture
  EM_ECOG16 = 176,        // Cyan Technology eCOG16 family
  EM_CR16 = 177,          // National Semiconductor CompactRISC CR16 16-bit
                          // microprocessor
  EM_ETPU = 178,          // Freescale Extended Time Processing Unit
  EM_SLE9X = 179,         // Infineon Technologies SLE9X core
  EM_L10M = 180,          // Intel L10M
  EM_K10M = 181,          // Intel K10M
  EM_AARCH64 = 183,       // ARM AArch64
  EM_AVR32 = 185,         // Atmel Corporation 32-bit microprocessor family
  EM_STM8 = 186,          // STMicroeletronics STM8 8-bit microcontroller
  EM_TILE64 = 187,        // Tilera TILE64 multicore architecture family
  EM_TILEPRO = 188,       // Tilera TILEPro multicore architecture family
  EM_MICROBLAZE = 189,    // Xilinx MicroBlaze 32-bit RISC soft processor core
  EM_CUDA = 190,          // NVIDIA CUDA architecture
  EM_TILEGX = 191,        // Tilera TILE-Gx multicore architecture family
  EM_CLOUDSHIELD = 192,   // CloudShield architecture family
  EM_COREA_1ST = 193,     // KIPO-KAIST Core-A 1st generation processor family
  EM_COREA_2ND = 194,     // KIPO-KAIST Core-A 2nd generation processor family
  EM_ARC_COMPACT2 = 195,  // Synopsys ARCompact V2
  EM_OPEN8 = 196,         // Open8 8-bit RISC soft processor core
  EM_RL78 = 197,          // Renesas RL78 family
  EM_VIDEOCORE5 = 198,    // Broadcom VideoCore V processor
  EM_78KOR = 199,         // Renesas 78KOR family
  EM_56800EX = 200,       // Freescale 56800EX Digital Signal Controller (DSC)
  EM_BA1 = 201,           // Beyond BA1 CPU architecture
  EM_BA2 = 202,           // Beyond BA2 CPU architecture
  EM_XCORE = 203,         // XMOS xCORE processor family
  EM_MCHP_PIC = 204,      // Microchip 8-bit PIC(r) family
  EM_INTEL205 = 205,      // Reserved by Intel
  EM_INTEL206 = 206,      // Reserved by Intel
  EM_INTEL207 = 207,      // Reserved by Intel
  EM_INTEL208 = 208,      // Reserved by Intel
  EM_INTEL209 = 209,      // Reserved by Intel
  EM_KM32 = 210,          // KM211 KM32 32-bit processor
  EM_KMX32 = 211,         // KM211 KMX32 32-bit processor
  EM_KMX16 = 212,         // KM211 KMX16 16-bit processor
  EM_KMX8 = 213,          // KM211 KMX8 8-bit processor
  EM_KVARC = 214,         // KM211 KVARC processor
  EM_CDP = 215,           // Paneve CDP architecture family
  EM_COGE = 216,          // Cognitive Smart Memory Processor
  EM_COOL = 217,          // iCelero CoolEngine
  EM_NORC = 218,          // Nanoradio Optimized RISC
  EM_CSR_KALIMBA = 219,   // CSR Kalimba architecture family
  EM_AMDGPU = 224,        // AMD GPU architecture
  EM_RISCV = 243,         // RISC-V
  EM_LANAI = 244,         // Lanai 32-bit processor
  EM_BPF = 247,           // Linux kernel bpf virtual machine
  EM_VE = 251,            // NEC SX-Aurora VE
  EM_CSKY = 252,          // C-SKY 32-bit processor
  EM_LOONGARCH = 258,     // LoongArch
} Elf_MachType;

typedef enum : Elf32_Word {
  EV_NONE    = 0,
  EV_CURRENT = 1,
} Elf_Version;

typedef enum : unsigned char {
  /** invalid                    */ ELFCLASS_NONE = 0,
  /** objs that 1, 2, or 4 bytes */ ELFCLASS_32   = 1,
  /** objs that 8 bytes          */ ELFCLASS_64   = 2,
} Elf_Class;

typedef enum : unsigned char {
  /** invalid       */ ELFDATA_NONE = 0,
  /** little-endian */ ELFDATA_LITTLE = 1,
  /** big-endian    */ ELFDATA_BIG = 2,
} Elf_HData;

typedef struct {
  struct {
    unsigned char  magic [4] /** {0x7f,'E','L','F'} */;
    Elf_Class      clazz;
    Elf_HData      datat;
    unsigned char  version_copy;
    unsigned char  _pad [9];
  } PACKED begin;

  struct {
    Elf_ObjType  type;
    Elf_MachType machine;
    Elf_Version  version;
  } part1;

  union {
    struct {
      Elf32_Addr     entry;
      Elf32_Off      phoff     /** program header offset from file or 0 */;
      Elf32_Off      shoff     /** section header offset from file or 0 */;
    } PACKED m32;

    struct {
      Elf64_Addr     entry;
      Elf64_Off      phoff     /** program header offset from file or 0 */;
      Elf64_Off      shoff     /** section header offset from file or 0 */;
    } PACKED m64;
  } PACKED;

  struct {
    Elf32_Word     flags;
    Elf32_Half     ehsize    /** elf header size bytes */;
    Elf32_Half     phentsize /** size of one entry in the program header table */;
    Elf32_Half     phnum     /** num entries in program header table */;
    Elf32_Half     shentsize /** size of one entry in the section header table*/;
    Elf32_Half     shnum     /** num entries in the section header table */;
    Elf32_Half     shstrndx  /** section name string table index into the section header table */;
  } part3;
} PACKED Elf_Header;

#define Elf_part2(elfhp, wantty, member) \
  (((elfhp)->begin.clazz == ELFCLASS_32) ? ((wantty) ((elfhp)->m32.member)) : ((wantty) ((elfhp)->m64.member)))

typedef enum : Elf32_Word {
  /** inactive                              */ SHT_NULL     = 0,
  /** program specific data                 */ SHT_PROGBITS = 1,
  /** symbol table                          */ SHT_SYMTAB   = 2,
  /** symbol table                          */ SHT_DYNSYM   = 11,
  /** string table                          */ SHT_STRTAB   = 3,
  /** relocation entries + explicit addends */ SHT_RELA     = 4,
  /** symbol hash table                     */ SHT_HASH     = 5,
  /** dynamic linking info                  */ SHT_DYNAMIC  = 6,
  /** file information                      */ SHT_NOTE     = 7,
  /** empty                                 */ SHT_NOBITS   = 8,
  /** relocation entries - explicit addends */ SHT_REL      = 9,
  /** reserved                              */ SHT_SHLIB    = 10,
  /** proc specific                         */ SHT_LOPROC   = 0x70000000,
  /** proc specific                         */ SHT_HIPROC   = 0x7fffffff,
  /** app specific                          */ SHT_LOUSER   = 0x80000000,
  /** app specific                          */ SHT_HIUSER   = 0xffffffff,
} Elf_SectionHeaderType;

typedef uint32_t Elf32_SectionHeaderFlags;

/** or-ed together */
typedef enum : uint64_t {
  /** writable data   */ SHF_WRITE     = 0x1,
  /** in program mem  */ SHF_ALLOC     = 0x2,
  /** executable      */ SHF_EXECINSTR = 0x4,
  /** might be merged */ SHF_MERGE     = 0x10,
  /** strings         */ SHF_STRINGS   = 0x20,
  /** sh_info+sht idx */ SHF_INFO_LINK = 0x40,
  /** presv ordr link */ SHF_LINK_ORDER       = 0x80,
  /** os spec handl r */ SHF_OS_NONCONFIRMING = 0x100,
  /** group member    */ SHF_GROUP            = 0x200,
  /** TLS             */ SHF_TLS              = 0x400,
  /** os specific     */ SHF_MASKOS           = 0x0FF00000,
  /** proc specific   */ SHF_MASKPROC         = 0xf0000000,
  /** special ord req */ SHF_ORDERED          = 0x4000000,
  /** excluded unl ref | alloc */ SHF_EXCLUDE = 0x8000000,
} Elf64_SectionHeaderFlags;

typedef struct {
  Elf32_Word sh_name   /** index into the section name string table */;
  Elf_SectionHeaderType    sh_type;
  Elf32_SectionHeaderFlags sh_flags;
  Elf32_Addr sh_addr   /** at which address this section should be placed in a process or 0 */;
  Elf32_Off  sh_offset /** data offset from file */;
  Elf32_Word sh_size   /** data size */;
  Elf32_Word sh_link   /** for SHT_DYNAMIC: section header index of string table;
                           for SHT_HASH: section header index of symbol table for hashes;
                           for SHT_REL{,A}: section header index of associated sym table; */;
  Elf32_Word sh_info;
  Elf32_Word sh_addralign /** address alignment of section data or 0/1 */;
  Elf32_Word sh_entsize   /** size of each entry in extra table (ex: sym table) or 0 */;
} PACKED Elf32_SectionHeader;

typedef struct {
  Elf32_Word sh_name   /** index into the section name string table */;
  Elf_SectionHeaderType    sh_type;
  Elf64_SectionHeaderFlags sh_flags;
  Elf64_Addr sh_addr   /** at which address this section should be placed in a process or 0 */;
  Elf64_Off  sh_offset /** data offset from file */;
  uint64_t   sh_size   /** data size */;
  Elf32_Word sh_link   /** for SHT_DYNAMIC: section header index of string table;
                           for SHT_HASH: section header index of symbol table for hashes;
                           for SHT_REL{,A}: section header index of associated sym table; */;
  Elf32_Word sh_info;
  uint64_t   sh_addralign /** address alignment of section data or 0/1 */;
  uint64_t   sh_entsize   /** size of each entry in extra table (ex: sym table) or 0 */;
} PACKED Elf64_SectionHeader;

typedef struct {
  /* sym name idx */        uint32_t      name;
  /* type +binding attrb */ unsigned char info;
  /* reserved            */ unsigned char other;
  /** section table idx  */ uint16_t      shndx;
  /** sym value          */ Elf64_Addr    value;
  /** obj size           */ uint64_t      size;
} PACKED Elf64_Sym;

typedef struct {
  uint32_t      name;
  Elf32_Addr    value;
  uint32_t      size;
  unsigned char info;
  unsigned char other;
  uint16_t      shndx;
} PACKED Elf32_Sym;


/** Used to mark an undeﬁned or meaningless section reference */
#define SHN_UNDEF  (0)
/** Processor-speciﬁc use */
#define SHN_LOPROC (0xFF00)
/** Processor-speciﬁc use */
#define SHN_HIPROC (0xFF1F)
/** Environment-speciﬁc use */
#define SHN_LOOS   (0xFF20) 
/** Environment-speciﬁc use */
#define SHN_HIOS   (0xFF3F)
/** Indicates that the corresponding reference is an absolute value */
#define SHN_ABS    (0xFFF1)
/** Indicates a symbol that has been declared as a common block */
#define SHN_COMMON (0xFFF2)

#include <stdio.h>

int Elf_decodeElfHeader(Elf_Header* dest, FILE* file, void (*err)(const char *));
int Elf_decodeSectionHeader(Elf64_SectionHeader* dest, size_t id, Elf_Header const* elf, FILE* file, void (*err)(const char *));
int Elf_readSection(void** heapDest, size_t* sizeDest, Elf_Header const* elf, Elf64_SectionHeader const* section, FILE* file, void (*err)(const char *));
int Elf_getStrTable(char** heapDest, size_t* sizeDest, size_t id, Elf_Header const* elf, FILE* file, void (*err)(const char *));
int Elf_getSymTable(Elf64_Sym** heapDest, size_t* sizeDest, Elf_Header const* elf, Elf64_SectionHeader const* section, FILE* file, void (*err)(const char *));

typedef struct {
  FILE* file;
  Elf_Header header;
  Elf64_SectionHeader * sectionHeaders;
  char * master_strtab;
} OpElf;

void OpElf_close(OpElf* elf);
int OpElf_open(OpElf* dest, FILE* file /** will not close */, void (*err)(const char *));
ssize_t OpElf_findSection(OpElf const* elf, const char * want);

#endif



