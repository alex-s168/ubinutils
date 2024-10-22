#include "memfile.h"

#ifdef _WIN32 

#include <windows.h>
#include <io.h>
#include <share.h>
#include <fcntl.h>
#include <sys/stat.h>

FILE* memFileOpenReadOnly(void* data, size_t len)
{
  char tp[MAX_PATH+1];
  if (!GetTempPathA(sizeof(tp), tp))
    return NULL;
  char fn[MAX_PATH+1];
  if (!GetTempFileNameA(tp, "MemFile_", 0, fn))
    return NULL;
  int fd;
  int err = _sopen_s(&fd, fn, _O_CREAT | _O_SHORT_LIVED | _O_TEMPORARY | _O_RDWR | _O_BINARY | _O_NOINHERIT, _SH_DENYRW, _S_IREAD);
  if (err)
    return NULL;
  if (fd == -1)
    return NULL;
  FILE* fp = _fdopen(fd, "wb+");
  if (!fp) {
    _close(fd);
    return NULL;
  }
  fwrite(data, 1, len, fp);
  fseek(fp, 0, SEEK_SET);
  return fp;
}

#endif 
