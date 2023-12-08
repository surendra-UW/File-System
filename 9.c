#include <stdio.h>
#include <string.h>
#include "test.h"
int main() {
  FILE* fp;
  fp = fopen("mnt/file0", "w");

  if (fp == NULL) {
  printf("fd canot opening a file\n");
    return FAIL;

  }

  printf(" after opening a file\n");

  fprintf(fp, "Hello");

  fclose(fp);

  fp = fopen("mnt/data11.txt", "r");
  char buffer[5] = {0};

  int bytesRead = fread(buffer, 1, sizeof(buffer), fp);

  const char* content = "Hello";

  if (bytesRead != strlen(content) || strcmp(content, buffer) != 0) {
    fclose(fp);
    printf("Wrong content! File contains %s\n", buffer);
    return FAIL;
  }
  fclose(fp);

  return PASS;
}