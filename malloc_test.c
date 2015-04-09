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

  int *buffer3 = my_malloc(2050);

  k=0;
  for(i = 0; i < 2050/sizeof(int); i++)
  {
    buffer3[i] = i+k;
    k++;
    k++;
  }
  k=0;

  for(i = 0; i < 512/sizeof(int); i++)
  {
    if(buffer[i] != i+k)
      fprintf(output, "Error, data offset in buffer at %d\n", i);
    k++;
    k++;
  }
  k=0;

  for(i = 0; i < 2050/sizeof(int); i++)
  {
    if(buffer3[i] != i+k)
      fprintf(output, "Error, data offset in buffer3 at %d\n", i);
    k++;
    k++;
  }


  my_free(buffer);

  fprintf(output, "buffer freed successfully\n");



  // int *buffer2 = my_malloc(512);
  // k=0;
  // for(i = 0; i < 512/sizeof(int); i++)
  // {
  //   buffer2[i] = i+k;
  //   k++;
  //   k++;
  // }

  my_mallinfo();

  my_free(buffer3);
  fprintf(output, "buffer3 freed succesfully\n");
  // k = 0;
  // for(i = 0; i < 512/sizeof(int); i++)
  // {
  //   if(buffer2[i] != i+k)
  //     fprintf(output, "Error, data offset in buffer2 at %d\n", i);
  //   k++;
  //   k++;
  // }

  // my_free(buffer2);
  // fprintf(output, "buffer2 freed succesfully\n");

  my_mallinfo();











  fclose(output);
  return 0;
}
