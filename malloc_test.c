#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "malloc_api.h"

int main(int argc, char* argv[])
{
  FILE *output = fopen("malloc_output.txt", "w");
  int* buffer = my_malloc(2048);

  return 0;
}
