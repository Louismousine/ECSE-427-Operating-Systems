#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "malloc_api.h"

int main(int argc, char* argv[])
{
  my_mallopt(1);
  FILE *output = fopen("malloc_output.txt", "w");
  int* buffer = my_malloc(2048);
  int i = 0;
  int k = 0;
  for(i = 0; i < 2048/sizeof(int); i++)
  {
    buffer[i] = i + k;
    k++;
  }
  int* buffer2 = my_malloc(12048);
  //int* buffer5 = my_malloc(5000);
  k = 0;
  for(i = 0; i < 12048/sizeof(int); i++)
  {
    buffer2[i] = i + k;
    k++;
  }
  my_free(buffer);
  //my_free(buffer2);
  int* buffer3 = my_malloc(13000);
  k = 0;
  for(i = 0; i < 13000/sizeof(int); i++)
  {
    buffer3[i] = i + k;
    k++;
  }
  my_free(buffer3);
  int* buffer4 = my_malloc(2048);
  k = 0;
  for(i = 0; i < 2048/sizeof(int); i++)
  {
    buffer4[i] = i + k;
    k++;
  }

  my_free(buffer2);
  my_free(buffer4);


  fclose(output);
  return 0;
}
