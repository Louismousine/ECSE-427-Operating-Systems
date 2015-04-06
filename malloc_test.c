#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "malloc_api.h"

int main(int argc, char* argv[])
{
  my_mallopt(2);
  int* buffer = my_malloc(512);
  int* buffer_check = malloc(512);
  int i = 0;
  int k = 0;
  for(i = 0; i < 512/sizeof(int); i++)
  {
    buffer[i] = i+k;
    k++;
    k++;
  }
  k = 0;
  for(i = 0; i < 512/sizeof(int); i++)
  {
    buffer_check[i] = i+k;
    k++;
    k++;
  }
  fprintf(stdout, "buffer: \n");
  for(i = 0; i < 512/sizeof(int); i++)
  {
    fprintf(stdout, "%d\n", buffer[i]);
    if(buffer_check[i] != buffer[i])
    {
      fprintf(stderr, "Error, buffer does not match expected result\n");
      return -1;
    }
  }
  my_free(buffer);
  free(buffer_check);

  // if(buffer != NULL && buffer_check != NULL)
  // {
  //   fprintf(stderr, "Error freeing buffer");
  //   return -1;
  // }

  return 0;
}
