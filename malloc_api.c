#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "malloc_api.h"

#define FIRST_FIT 1
#define BEST_FIT 2

#define MIN_FREE 100
#define MAX_FREE_BLOCK 128000
#define MALLOC_BLOCKSIZE 16384
#define FREE_RM 20000

#define ALIGN(x) ((x/MALLOC_BLOCKSIZE + 1) * MALLOC_BLOCKSIZE)
#define INCR_PTR(ptr, len) (((char*)ptr) + len)
#define DECR_PTR(ptr, len) (((char*)ptr) - len)
#define NEW_TAG(len,free) ((len << 1) + (free == 1 ? 0b0 : 0b1))
#define TAG_SIZE

typedef struct FreeListNode
{
  struct FreeListNode *next;
  struct FreeListNode *prev;
} FreeListNode;

void updateContiguous();
void updateTopFreeBlock();
unsigned getBits(unsigned start, unsigned finish, unsigned num);

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
  fprintf(stdout, "full_size: %d\n", full_size);

  if(head.next == NULL || tail.prev == NULL)
  {
    int* start = (int*)sbrk(full_size);
    int* free_end = (int*)sbrk(0);

    *start = NEW_TAG(size, 0);
    start = (int*)INCR_PTR(start, 4);
    int* end = (int*)INCR_PTR(start, size);
    *end = NEW_TAG(size, 0);

    int* free_start = (int*)INCR_PTR(end, 4);
    *free_start = NEW_TAG(full_size - size, 1);
    free_start = (int*)INCR_PTR(free_start, 4);

    FreeListNode *new = (FreeListNode*)free_start;

    new->next = &(tail);
    new->prev = &(head);
    tail.prev = new;
    head.next = new;

    free_end = (int*)DECR_PTR(free_end, 4);
    *free_end = NEW_TAG(full_size - size, 1);

    start = (int*)DECR_PTR(start, 4);

    unsigned val = getBits(1, 31, (unsigned)(start[0] >> 1));
    unsigned used = getBits(0, 0, (unsigned)(start[0]));
    fprintf(stdout, "val: %d\nused: %d\n", val, used);

    return (void*)start;
  } else
  {
    FreeListNode *next_iter = head.next;
    FreeListNode *prev_init = tail.prev;
    while(next_iter != NULL || prev_init != NULL)
    {
      if(next_iter != NULL)
      {
        int* check = (int*)(next_iter);

        check = (int*)DECR_PTR(check, 4);


      }
    }
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

unsigned getBits(unsigned start, unsigned finish, unsigned num)
{
   unsigned result = 0;
   unsigned i;
   for (i=start; i<=finish; i++)
   {
       result |= 1 << i;
    }
   return result & num;
}
