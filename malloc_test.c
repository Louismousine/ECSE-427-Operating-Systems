#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "malloc_api.h"

int main(int argc, char* argv[])
{
  FILE *output = fopen("malloc_output.txt", "w");
  my_mallinfo();
  my_mallopt(1);
  int* fail = my_malloc(-10);
  fprintf(output, "%s\n", my_malloc_error);
  int* buffer = my_malloc(2048);
  int i = 0;
  int k = 0;
  for(i = 0; i < 2048/sizeof(int); i++)
  {
    buffer[i] = i + k;
    k++;
  }
  my_mallinfo();
  fprintf(output, "buffer: %p\n", buffer);


  int* buffer2 = my_malloc(2048);
  fprintf(output, "buffer2: %p\n", buffer2);

  my_mallinfo();

  k = 0;
  for(i = 0; i < 2048/sizeof(int); i++)
  {
    buffer2[i] = i + k;
    k++;
  }
  my_free(buffer);
  my_mallinfo();

  int* buffer3 = my_malloc(2048);
  k = 0;
  for(i = 0; i < 2048/sizeof(int); i++)
  {
    buffer3[i] = i + k;
    k++;
  }
  fprintf(output, "buffer3: %p\n", buffer3);

  my_mallinfo();

  my_free(buffer3);
  my_mallinfo();

  int* buffer4 = my_malloc(17000);
  k = 0;
  for(i = 0; i < 17000/sizeof(int); i++)
  {
    buffer4[i] = i + k;
    k++;
  }
  fprintf(output, "buffer4: %p\n", buffer4);

  my_mallinfo();

  my_free(buffer2);
  my_mallinfo();

  my_free(buffer4);

  my_mallinfo();


  fclose(output);
  return 0;
}
