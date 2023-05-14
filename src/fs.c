#include "defs.h"

char* read_file(char* filepath) {
  FILE* fp = fopen(filepath, "r");
  if (!fp) {
    write_to_log("Unable to open file %s. Are you sure the path is correct?\n",
                 filepath);
    exit(1);
  }

  fseek(fp, 0, SEEK_END);
  long fsize = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  char* buf = xmalloc(fsize + 1);
  fread(buf, fsize, 1, fp);
  fclose(fp);

  buf[fsize] = '\0';

  return buf;
}
