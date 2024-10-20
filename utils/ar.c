#include "../lib/ar.h"

static void print_usage()
{
  puts("Usage: ar command archive-file file...");
  puts(" commands:");
  puts("  t - display contents of the archive");
}

int main(int argc, char** argv)
{
  if (argc < 3) {
    print_usage();
    return 1;
  }

  char * mode = argv[1];
  if ( strcmp(mode, "t") ) {
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

  char* name;
  while ( (name = SmartArchive_nextFileNameHeap(&a)) )
  {
    if ( name[0] != '/' )
      puts(name);
    free(name);
    SmartArchive_continueNoData(&a);
  }

  SmartArchive_close(&a);
  return 0;
}
