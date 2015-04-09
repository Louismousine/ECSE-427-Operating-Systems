#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "malloc_api.h"

int main(int argc, char* argv[])
{
  FILE *output = fopen("malloc_output.txt", "w");

  int i = 0;
  int k = 0;
  my_mallopt(1);

  int* buffer = my_malloc(512);

  for(i = 0; i < 512/sizeof(int); i++)
  {
    buffer[i] = i+k;
    k++;
    k++;
  }
  k = 0;
  fprintf(output, "buffer: \n");
  for(i = 0; i < 512/sizeof(int); i++)
  {
    fprintf(output, "%d\n", buffer[i]);
  }


  int *buffer3 = my_malloc(2050);

  k=0;
  for(i = 0; i < 512/sizeof(int); i++)
  {
    buffer3[i] = i+k;
    k++;
    k++;
  }
  my_free(buffer);

  fprintf(output, "first free successfull\n");
  my_free(buffer3);
  fprintf(output, "second free succesfull\n");


  int *buffer2 = my_malloc(512);
  k=0;
  for(i = 0; i < 512/sizeof(int); i++)
  {
    buffer2[i] = i+k;
    k++;
    k++;
  }

  my_mallinfo();
  fprintf(output, "buffer2: \n");
  for(i = 0; i < 512/sizeof(int); i++)
  {
    fprintf(output, "%d\n", buffer2[i]);
  }


  my_free(buffer2);

  my_mallinfo();
  return 0;
}
