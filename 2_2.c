#include <stdio.h>
// #include "common/test.h"
int main() {
  FILE *fp;
  fp = fopen("mnt/file0", "w");
  if (fp < 0)
  {
    printf("Success\n");
    return -1;
  }
  else {
    fclose(fp);
    printf("Success\n");
    return 0;
  }
}