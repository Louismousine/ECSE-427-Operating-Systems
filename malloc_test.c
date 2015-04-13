#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "malloc_api.h"

#define MALLOC_BLOCKSIZE BLOCK_SIZE
#define SIZE_ONE 16383
#define SIZE_TWO 4096
#define SIZE_THREE 15000
#define BLOCK_SIZE 16384

int main(int argc, char* argv[])
{
  FILE *output = fopen("malloc_output.txt", "w");
  my_mallinfo();
  my_mallopt(1);
  int* fail = my_malloc(-10);
  fprintf(output, "%s\r\n", my_malloc_error);
  int* buffer = my_malloc(SIZE_ONE);
  int i = 0;
  int k = 0;
  for(i = 0; i < SIZE_ONE/sizeof(int); i++)
  {
    buffer[i] = i + k;
    k++;
  }
  my_mallinfo();
  fprintf(output, "buffer: %p\r\n", buffer);
  if(my_bytes() == SIZE_ONE)
    fprintf(output, "number of bytes allocated successful.\r\n");
  else
    fprintf(output, "number of bytes allocated unsuccessful.\r\n");

  if(my_free_space() == (BLOCK_SIZE - SIZE_ONE))
    fprintf(output, "number of bytes freed succesful.\r\n");
  else
    fprintf(output, "number of bytes freed unsuccessful.\r\n");

  if(my_largestSpace() == (BLOCK_SIZE - SIZE_ONE))
    fprintf(output, "largest space is correct.\r\n");
  else
    fprintf(output, "largest space is incorrect.\r\n");


  int* buffer2 = my_malloc(SIZE_TWO);
  fprintf(output, "buffer2: %p\r\n", buffer2);

  if(my_bytes() == SIZE_ONE + SIZE_TWO)
    fprintf(output, "number of bytes allocated successful.\r\n");
  else
    fprintf(output, "number of bytes allocated unsuccessful.\r\n");

  if(my_free_space() == (BLOCK_SIZE - (SIZE_ONE+SIZE_TWO+8)))
    fprintf(output, "number of bytes freed succesful.\r\n");
  else
    fprintf(output, "number of bytes freed unsuccessful.\r\n");

  if(my_largestSpace() == (BLOCK_SIZE - (SIZE_ONE+SIZE_TWO+8)))
    fprintf(output, "largest space is correct.\r\n");
  else
    fprintf(output, "largest space is incorrect.\r\n");
  my_mallinfo();


  k = 0;
  for(i = 0; i < SIZE_TWO/sizeof(int); i++)
  {
    buffer2[i] = i + k;
    k++;
  }
  my_free(buffer);

  if(my_bytes() == SIZE_TWO)
    fprintf(output, "number of bytes allocated successful.\r\n");
  else
    fprintf(output, "number of bytes allocated unsuccessful.\r\n");

  if(my_free_space() == (BLOCK_SIZE - SIZE_TWO))
    fprintf(output, "number of bytes freed succesful.\r\n");
  else
    fprintf(output, "number of bytes freed unsuccessful.\r\n");

  if(my_largestSpace() == (BLOCK_SIZE - (SIZE_ONE+SIZE_TWO+8)))
    fprintf(output, "largest space is correct.\r\n");
  else
    fprintf(output, "largest space is incorrect.\r\n");
  my_mallinfo();

  my_mallinfo();

  int* buffer3 = my_malloc(SIZE_ONE);
  k = 0;
  for(i = 0; i < SIZE_ONE/sizeof(int); i++)
  {
    buffer3[i] = i + k;
    k++;
  }
  fprintf(output, "buffer3: %p\r\n", buffer3);
  if(my_bytes() == SIZE_ONE + SIZE_TWO)
    fprintf(output, "number of bytes allocated successful.\r\n");
  else
    fprintf(output, "number of bytes allocated unsuccessful.\r\n");

  if(my_free_space() == (BLOCK_SIZE - (SIZE_ONE+SIZE_TWO+8)))
    fprintf(output, "number of bytes freed succesful.\r\n");
  else
    fprintf(output, "number of bytes freed unsuccessful.\r\n");

  if(my_largestSpace() == (BLOCK_SIZE - (SIZE_ONE+SIZE_TWO+8)))
    fprintf(output, "largest space is correct.\r\n");
  else
    fprintf(output, "largest space is incorrect.\r\n");
  my_mallinfo();

  my_free(buffer3);
  if(my_bytes() == SIZE_TWO)
    fprintf(output, "number of bytes allocated successful.\r\n");
  else
    fprintf(output, "number of bytes allocated unsuccessful.\r\n");

  if(my_free_space() == (BLOCK_SIZE - SIZE_TWO))
    fprintf(output, "number of bytes freed succesful.\r\n");
  else
    fprintf(output, "number of bytes freed unsuccessful.\r\n");

  if(my_largestSpace() == (BLOCK_SIZE - (SIZE_ONE+SIZE_TWO+8)))
    fprintf(output, "largest space is correct.\r\n");
  else
    fprintf(output, "largest space is incorrect.\r\n");
  my_mallinfo();

  int* buffer4 = my_malloc(SIZE_THREE);
  k = 0;
  for(i = 0; i < SIZE_THREE/sizeof(int); i++)
  {
    buffer4[i] = i + k;
    k++;
  }
  fprintf(output, "buffer4: %p\r\n", buffer4);

  my_mallinfo();
  if(my_bytes() == SIZE_TWO+SIZE_THREE)
    fprintf(output, "number of bytes allocated successful.\r\n");
  else
    fprintf(output, "number of bytes allocated unsuccessful.\r\n");

  if(my_free_space() == (2*(BLOCK_SIZE) - (SIZE_TWO+SIZE_THREE)))
    fprintf(output, "number of bytes freed succesful.\r\n");
  else
    fprintf(output, "number of bytes freed unsuccessful.\r\n");

  if(my_largestSpace() == (BLOCK_SIZE - (SIZE_ONE+SIZE_TWO+8)))
    fprintf(output, "largest space is correct.\r\n");
  else
    fprintf(output, "largest space is incorrect.\r\n");
  my_free(buffer2);

  if(my_bytes() == SIZE_THREE)
    fprintf(output, "number of bytes allocated successful.\r\n");
  else
    fprintf(output, "number of bytes allocated unsuccessful.\r\n");

  if(my_free_space() == (2*(BLOCK_SIZE)+8 - SIZE_THREE))
    fprintf(output, "number of bytes freed succesful.\r\n");
  else
    fprintf(output, "number of bytes freed unsuccessful.\r\n");

  if(my_largestSpace() == (BLOCK_SIZE+8))
    fprintf(output, "largest space is correct.\r\n");
  else
    fprintf(output, "largest space is incorrect.\r\n");
  my_mallinfo();

  my_free(buffer4);
  if(my_bytes() == 0)
    fprintf(output, "number of bytes allocated successful.\r\n");
  else
    fprintf(output, "number of bytes allocated unsuccessful.\r\n");

  if(my_free_space() == (2*(BLOCK_SIZE) + 16))
    fprintf(output, "number of bytes freed succesful.\r\n");
  else
    fprintf(output, "number of bytes freed unsuccessful.\r\n");

  if(my_largestSpace() == (2*(BLOCK_SIZE) + 16))
    fprintf(output, "largest space is correct.\r\n");
  else
    fprintf(output, "largest space is incorrect.\r\n");
  my_mallinfo();


  fclose(output);
  return 0;
}
