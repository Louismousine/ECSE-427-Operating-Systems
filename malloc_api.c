#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "malloc_api.h"

#define FIRST_FIT 1
#define BEST_FIT 2

#define MIN_FREE 100
#define MAX_FREE_BLOCK 128000
#define MALLOC_BLOCKSIZE 2048
#define FREE_RM 20000

#define ALIGN(x) ((x/MALLOC_BLOCKSIZE + 1) * MALLOC_BLOCKSIZE)
#define INCR_PTR(ptr, len) (((char*)ptr) + len)
#define DECR_PTR(ptr, len) (((char*)ptr) - len)
#define NEW_TAG(len,free) ((len << 1) + free == 1 ? 0b1 : 0b0)

typedef struct freeListNode
{
  struct FreeListNode *prev;
  struct FreeListNode *next;
} freeListNode;

void updateContiguous();
void updateTopFreeBlock();

static FreeListNode head = {NULL, NULL};
static FreeListNode tail = {NULL, NULL};
extern char *my_malloc_error;

int currentPolicy = 1;
int bytesAlloc = 0;
int freeSpace = 0;
int largestSpace = 0;

void *my_malloc(int size)
{
  int full_size = ALIGN(size);

  if(head.next == NULL || tail.prev == NULL)
  {
    int* start = (int*)sbrk(full_size);
    int* free_end = (int*)sbrk(0);

    fprintf(stdout, "initial start: %p\n", start);
    start = NEW_TAG(size, 0);
    fprintf(stdout, "post tag start: %p\n", start);
    start = INCR_PTR(start, 4);
    fprintf(stdout, "post INCR start: %p\n", start);
    int* end = INCR_PTR(start, size-4);
    end = NEW_TAG(size, 0);
    //end = INCR_PTR(end, 4)

  }

}

void my_free(void *ptr)
{

}

void updateContiguous()
{

}

void updateTopFreeBlock()
{

}

void my_mallopt(int policy)
{
  if(policy == FIRST_FIT)
  {
    currentPolicy = FIRST_FIT;
  } else if(policy == BEST_FIT)
  {
    currentPolicy = BEST_FIT;
  } else
  {
    fprintf(stderr, "Error, not a valid policy");
    return;
  }
}

void my_mallinfo()
{
  fprintf(stdout, "Current number of bytes allocated: %d\n", bytesAlloc);
  fprintf(stdout, "Current amount of free space: %d\n", freeSpace);
  fprintf(stdout, "Current largest contiguous free space: %d\n", largestSpace);
  fprintf(stdout, "Current policy number: %d\n", currentPolicy);
}
