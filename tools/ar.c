#include "ubu/ar.h"
#include "ubu/elf.h"

static void print_usage()
{
  puts("Usage: ar command archive-file file..."
  "\n" " commands:"
  "\n" "  t   - display contents of the archive"
  "\n" "  x   - extract specified files"
  "\n" "  p   - print specified or all files concatenated"
  "\n" "  rcs - create archive with files and generate symbol indecies"
  "\n" "  rc  - create archive with files");
}

static void arDisplayContents(SmartArchive* a)
{
  SmartArchive_rewind(a);
  char* name;
  while ( (name = SmartArchive_nextFileNameHeap(a)) )
  {
    if ( name[0] && name[0] != '/' )
      puts(name);
    free(name);
    SmartArchive_continueNoData(a);
  }
}

static void arPrintContents(SmartArchive* a, int argc, char** argv, int* code)
{
  if ( argc )
  {
    for ( int i = 0; i < argc; i ++ )
    {
      SmartArchive_rewind(a);
      void* data; size_t uz;
      if ( SmartArchive_findNext(a, &data, &uz, argv[i]) )
      {
        fprintf(stderr, "%s not found in archive\n", argv[i]);
        *code = 1;
        continue;
      }

      fwrite(data, 1, uz, stdout);
      free(data);
    }
  }
  else 
  {
    SmartArchive_rewind(a);
    char* name;
    while ( (name = SmartArchive_nextFileNameHeap(a)) )
    {
      free(name);

      void* data; size_t uz;
      SmartArchive_continueWithData(&data, &uz, a);

      fwrite(data, 1, uz, stdout);
      free(data);
    }
  }
}

static void arExtract(SmartArchive* a, int argc, char** argv, int* code)
{
  if ( !argc )
  {
    fprintf(stderr, "no files specified\n");
    *code = 1;
    return;
  }

  for ( int i = 0; i < argc; i ++ )
  {
    SmartArchive_rewind(a);
    void* data; size_t uz;
    if ( SmartArchive_findNext(a, &data, &uz, argv[i]) )
    {
      fprintf(stderr, "%s not found in archive\n", argv[i]);
      *code = 1;
      continue;
    }

    FILE* fp = fopen(argv[i], "wb");
    if ( fp )
    {
      fwrite(data, 1, uz, fp);
      fclose(fp);
    }
    else 
    {
      fprintf(stderr, "could not open file %s for writing\n", argv[i]);
    }

    free(data);
  }
}

static void init_ar_header(Ar_FileHeader* header, bool is_idx)
{
  header->padding[0] = 0x60;
  header->padding[1] = 0x0A;
  memset(header->filename, ' ', 16);
  memset(header->decimal_file_modific_timestamp, ' ', 12);
  memset(header->decimal_ownder_id, ' ', 6);
  memset(header->decimal_group_id, ' ', 6);
  memset(header->octal_file_mode, ' ', 8);
  memset(header->decimal_file_size, ' ', 10);
  if (!is_idx) {
    header->decimal_file_modific_timestamp[0] = '0';
    header->decimal_ownder_id[0] = '0';
    header->decimal_group_id[0] = '0';
    header->octal_file_mode[0] = '0';
  }
}

static void no_nt_strcpy(char * dest, const char * src)
{
  size_t len = strlen(src);
  memcpy(dest, src, len);
}

// TODO: endianess
static int gen_ar(char * out, char ** ins, size_t num_ins, bool ranlib) {
  FILE* outf = fopen(out, "wb");
  if (out == NULL) {
    fprintf(stderr, "Can't create output file\n");
    return 1;
  }

  static char magic[8] = "!<arch>\x0A";
  fwrite(magic, 1, 8, outf);

  int code = 0;

  char* filenames = NULL;
  size_t filenames_len = 0;

  FILE* handles[num_ins];
  size_t fnidc[num_ins];
  for (size_t i = 0; i < num_ins; i ++)
  {
    FILE* infile = fopen(ins[i], "rb");
    if (infile == NULL) {
      fprintf(stderr, "Can't open %s\n", ins[i]);
      code = 1;
      handles[i] = NULL;
      continue;
    }
    handles[i] = infile;

    size_t fnidx = filenames_len;
    size_t fnlen = strlen(ins[i]);
    filenames = realloc(filenames, filenames_len + fnlen + 2);
    memcpy(filenames + filenames_len, ins[i], fnlen);
    filenames_len += fnlen;
    filenames[filenames_len++] = '/';
    filenames[filenames_len++] = '\n';

    fnidc[i] = fnidx;
  }

  uint32_t * syms_offs = NULL;
  size_t syms_offs_len = 0;

  char * syms_names = NULL;
  size_t syms_names_len = 0;

  size_t where_write_offsets = 0;

  size_t * file2offs[num_ins];
  size_t   file2offs_len[num_ins];
  memset(file2offs, 0, sizeof(file2offs));
  memset(file2offs_len, 0, sizeof(file2offs_len));

  if (ranlib) {
    for (size_t i = 0; i < num_ins; i ++) {
      FILE* infile = handles[i];
      rewind(infile);

      OpElf elf;
      if ( OpElf_open(&elf, infile, NULL) ) {
        printf("could not generate symbol indexes for %s\n", ins[i]);
        continue;
      }

      ssize_t symtab = OpElf_findSection(&elf, ".symtab");
      if ( symtab == -1 )
        symtab = OpElf_findSection(&elf, ".dynsym");

      if ( symtab != -1 )
      {
        Elf64_SectionHeader sec = elf.sectionHeaders[symtab];

        char * tsstab;
        if ( !Elf_getStrTable(&tsstab, NULL, sec.sh_link, &elf.header, elf.file, NULL) )
        {
          Elf64_Sym* syms;
          size_t syms_len = 0;
          if ( !Elf_getSymTable(&syms, &syms_len, &elf.header, &sec, elf.file, NULL) )
          {
            if ( syms_len ) {
              // first symbol is fake
              syms_len --;
              syms ++;
            }

            for (size_t u = 0; u < syms_len; u++)
            {
              uint8_t bind_attrib = syms[u].info >> 4;
              uint8_t sym_type = syms[u].info & 0xF;

              const char * sname = NULL;
              {
                uint32_t sectionnam = 0;
                if ( syms[u].shndx && syms[u].shndx < elf.header.part3.shnum )
                  sectionnam = elf.sectionHeaders[syms[u].shndx].sh_name;
                if ( sectionnam ) 
                  sname = elf.master_strtab + sectionnam;
              }

              bool is_global = u >= sec.sh_info;

              if (is_global) {
                size_t slen = strlen(sname);
                syms_names = realloc(syms_names, syms_names_len + slen + 1);
                memcpy(syms_names + syms_names_len, sname, slen);
                syms_names_len += slen;
                syms_names[syms_names_len++] = '/';

                syms_offs = realloc(syms_offs, (syms_offs_len + 1) * sizeof(uint32_t));
                syms_offs[syms_offs_len] = 0; // will be overwritten later

                file2offs[i] = realloc(file2offs[i], file2offs_len[i] + 1);
                file2offs[i][file2offs_len[i]++] = syms_offs_len;

                syms_offs_len ++;
              }
            }

            free(syms);
          }

          free(tsstab);
        }
      }

      OpElf_close(&elf);
    }

    if ( syms_offs_len > 0 )
    {
      Ar_FileHeader header;
      init_ar_header(&header, true);
      no_nt_strcpy(header.filename, "/");
      char _filesize[20];
      sprintf(_filesize, "%zu", sizeof(uint32_t) + sizeof(uint32_t) * syms_offs_len + syms_names_len);
      no_nt_strcpy(header.decimal_file_size, _filesize);

      fwrite(&header, 1, sizeof(header), outf);

      uint32_t num_ents = syms_offs_len;
      fwrite(&num_ents, 1, sizeof(num_ents), outf);

      where_write_offsets = ftell(outf);
      fwrite(syms_offs, 1, syms_offs_len * sizeof(uint32_t), outf);
      fwrite(syms_names, 1, syms_names_len, outf);

      free(syms_names);
    }
  }

  {
    Ar_FileHeader header;
    init_ar_header(&header, true);
    no_nt_strcpy(header.filename, "//");
    char _filesize[20];
    sprintf(_filesize, "%zu", filenames_len);
    no_nt_strcpy(header.decimal_file_size, _filesize);

    fwrite(&header, 1, sizeof(header), outf);
    fwrite(filenames, 1, filenames_len, outf);

    free(filenames);
  }

  for (size_t i = 0; i < num_ins; i ++)
  {
    FILE* infile = handles[i];
    if (infile == NULL) continue;
    size_t off = ftell(outf);

    for (size_t o = 0; o < file2offs_len[i]; o ++)
      syms_offs[file2offs[i][o]] = off;

    fseek(infile, 0, SEEK_END);
    size_t filesize = ftell(infile);
    rewind(infile);

    Ar_FileHeader header;
    init_ar_header(&header, false);
    char _filename[20];
    sprintf(_filename, "/%zu", fnidc[i]);
    no_nt_strcpy(header.filename, _filename);
    char _filesize[20];
    sprintf(_filesize, "%zu", filesize);
    no_nt_strcpy(header.decimal_file_size, _filesize);

    fwrite(&header, 1, sizeof(header), outf);

    char* infp = malloc(filesize);
    fread(infp, 1, filesize, infile);
    fwrite(infp, 1, filesize, outf);
    free(infp);
  }

  if (where_write_offsets != 0) {
    fseek(outf, where_write_offsets, SEEK_SET);
    fwrite(syms_offs, 1, syms_offs_len * sizeof(uint32_t), outf);
  }

  free(syms_offs);

  for (size_t i = 0; i < num_ins; i ++) {
    if (handles[i]) {
      fclose(handles[i]);
      free(file2offs[i]);
    }
  }

  fclose(outf);

  return code;
}

int main(int argc, char** argv)
{
  if (argc < 3) {
    print_usage();
    return 1;
  }

  if (!strcmp("rcs", argv[1])) {
    return gen_ar(argv[2], argv + 3, argc - 3, true);
  }

  if (!strcmp("rc", argv[1])) {
    return gen_ar(argv[2], argv + 3, argc - 3, false);
  }

  char mode = argv[1][0];

  if ( argv[1][1] || !strchr("txp", mode) ) {
    print_usage();
    return 1;
  }

  FILE* file = fopen(argv[2], "rb");
  if ( !file ) {
    fprintf(stderr, "could not open file\n");
    return 1;
  }

  SmartArchive a;
  if ( SmartArchive_open(&a, file) ) {
    fprintf(stderr, "error reading archive\n");
    return 1;
  }

  int code = 0;
  switch ( mode )
  {
    case 't':
      arDisplayContents(&a);
      break;

    case 'p':
      arPrintContents(&a, argc-3, argv+3, &code);
      break;

    case 'x':
      arExtract(&a, argc-3, argv+3, &code);
      break;

    default:break;
  }

  SmartArchive_close(&a);
  fclose(file);
  return code;
}
