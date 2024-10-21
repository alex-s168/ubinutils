#include "../lib/ar.h"

static void print_usage()
{
  puts("Usage: ar command archive-file file..."
  "\n" " commands:"
  "\n" "  t - display contents of the archive"
  "\n" "  x - extract specified files"
  "\n" "  p - print specified or all files concatenated");
}

static void arDisplayContents(SmartArchive* a)
{
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

    FILE* fp = fopen(argv[i], "w");
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

int main(int argc, char** argv)
{
  if (argc < 3) {
    print_usage();
    return 1;
  }

  char mode = argv[1][0];
  if ( argv[1][1] || !strchr("txp", mode) ) {
    print_usage();
    return 1;
  }

  FILE* file = fopen(argv[2], "r");
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
  return code;
}
