#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "malloc_api.h"

#define FIRST_FIT 1
#define BEST_FIT 2

#define MIN_FREE 100
#define MALLOC_BLOCKSIZE 16384
#define FREE_RM 20000
#define TOP_FREE 131072

#define ALIGN(x) ((x/MALLOC_BLOCKSIZE + 1) * MALLOC_BLOCKSIZE)
#define INCR_PTR(ptr, len) (((char*)ptr) + len)
#define DECR_PTR(ptr, len) (((char*)ptr) - len)
#define NEW_TAG(len,free) ((len << 1) + (free == 1 ? 0b0 : 0b1))
#define GET_TAG_SIZE(len) (len >> 1)
#define GET_TAG_FREE(ptr) (ptr & 0b1)

typedef struct FreeListNode
{
  struct FreeListNode *next;
  struct FreeListNode *prev;
} FreeListNode;

void updateContiguous();
void updateTopFreeBlock();
void* createNew(int size, int best_size);
void removeNode(FreeListNode *iter);
void addNode(FreeListNode *new);

FreeListNode *head;
FreeListNode *tail;

char *my_malloc_error;
int* top_block;

int currentPolicy = 1;
int bytesAlloc = 0;
int freeSpace = 0;
int largestSpace = 0;
int number_calls = 0;

void *my_malloc(int size)
{
  if(number_calls == 0)
  {
    my_malloc_error = (char*)sbrk(1024);
  }
  number_calls++;

  if(size < 0)
  {
    my_malloc_error = "Error, mallocing requested space";
    return NULL;
  }
  if(currentPolicy == 1)
  {
    FreeListNode *iter = head;
    //unsigned best = 131073;

    while(iter != NULL)
    {
      int* check = (int*)(iter);

      check = (int*)DECR_PTR(check, 4);
      unsigned avail_size = GET_TAG_SIZE(check[0]);
      //fprintf(stdout, "size:%d\n", avail_size);
      if(avail_size - 8 >= size)
      {
        return createNew(size, avail_size);
      }
      if(iter != NULL)
        iter = iter->next;
    }
    return createNew(size, -1);
  }else if(currentPolicy == 2)
  {
    FreeListNode *iter = head;
    unsigned best = 131073;

    while(iter != NULL)
    {
      int* check = (int*)(iter);

      check = (int*)DECR_PTR(check, 4);
      unsigned avail_size = GET_TAG_SIZE(check[0]);
      //fprintf(stdout, "size:%d\n", avail_size);
      if(avail_size - 8 == size)
        return createNew(size, avail_size);
      else if(avail_size - 8 >= size && avail_size <= best)
      {
        best = avail_size;
      }
      if(iter != NULL)
        iter = iter->next;
    }
    return createNew(size, best);
  }
  my_malloc_error = "Error, mallocing required space";
  return NULL;
}

void my_free(void *ptr)
{
  int* new_free = (int*)(ptr);

  new_free = (int*)DECR_PTR(new_free, 4);

  int free_size = GET_TAG_SIZE(new_free[0]);
  fprintf(stdout, "free_size: %d\n", free_size);

  int* bot_check = (int*)DECR_PTR(new_free, 4);
  int* top_check = (int*)INCR_PTR(new_free, (free_size + 8));

  int bot_free = GET_TAG_FREE(bot_check[0]);
  fprintf(stdout, "bot_free: %d\n", bot_free);
  int top_free = GET_TAG_FREE(top_check[0]);
  fprintf(stdout, "top_free: %d\n", top_free);

  int bot_size_check = GET_TAG_SIZE(bot_check[0]);
  if(bot_size_check == 0)
    bot_free = -1;
  if(!bot_free && !top_free)
  {
    int bot_size = GET_TAG_SIZE(bot_check[0]);
    bot_check = (int*)DECR_PTR(bot_check, (bot_size - 8));
    fprintf(stdout, "bot_size: %d\n", bot_size);

    int top_size = GET_TAG_SIZE(top_check[0]);
    top_check = (int*)INCR_PTR (top_check, 4);
    fprintf(stdout, "top_size: %d\n", top_size);

    FreeListNode *rem_bot = (FreeListNode*)(bot_check);
    FreeListNode *rem_top = (FreeListNode*)(top_check);

    removeNode(rem_bot);
    removeNode(rem_top);

    free_size += (bot_size + top_size + 8);
    bot_check = (int*)DECR_PTR(bot_check, 4);
    top_check = (int*)INCR_PTR(top_check, (top_size - 8));
    *bot_check = NEW_TAG(free_size, 1);
    *top_check = NEW_TAG(free_size, 1);

    int* start = (int*)INCR_PTR(bot_check, 4);
    FreeListNode *new = (FreeListNode*)start;
    addNode(new);

  }else if(!bot_free)
  {
    int bot_size = GET_TAG_SIZE(bot_check[0]);
    fprintf(stdout, "bot_size: %d\n", bot_size);
    bot_check = (int*)DECR_PTR(bot_check, (bot_size - 8));

    FreeListNode *rem_bot = (FreeListNode*)(bot_check);

    removeNode(rem_bot);

    bot_check = (int*)DECR_PTR(bot_check, 4);
    new_free = (int*)INCR_PTR(new_free, (free_size + 4));

    free_size += (bot_size + 8);

    *bot_check = NEW_TAG(free_size, 1);
    *new_free = NEW_TAG(free_size, 1);

    int* start = (int*)INCR_PTR(bot_check, 4);

    FreeListNode *new = (FreeListNode*)start;
    addNode(new);
  }else if(!top_free)
  {
    int top_size = GET_TAG_SIZE(top_check[0]);
    top_check = (int*)INCR_PTR (top_check, (4));
    fprintf(stdout, "top_size: %d\n", top_size);

    FreeListNode *rem_top = (FreeListNode*)(top_check);

    removeNode(rem_top);

    top_check = (int*)INCR_PTR(top_check, (top_size - 8));

    free_size += (top_size + 8);
    *new_free = NEW_TAG(free_size, 1);
    *top_check = NEW_TAG(free_size, 1);

    int* start = (int*)INCR_PTR(new_free, 4);
    FreeListNode *new = (FreeListNode*)start;
    addNode(new);
  }else
  {

    *new_free = NEW_TAG((free_size + 8), 1);
    new_free = (int*)INCR_PTR(new_free, (free_size + 4));
    *new_free = NEW_TAG((free_size+8), 1);

    int* start = (int*)DECR_PTR(new_free, free_size);

    FreeListNode *new = (FreeListNode*)start;
    addNode(new);
  }
  updateTopFreeBlock();
}

void updateContiguous()
{

}

void updateTopFreeBlock()
{
  int* check_top = (int*)(top_block);
  check_top = (int*)DECR_PTR(check_top, 4);
  int top_tag = GET_TAG_FREE(check_top[0]);
  if(top_tag == 0)
  {
    unsigned top_size = GET_TAG_SIZE(check_top[0]);

    if(top_size >=  TOP_FREE)
    {
      check_top = (int*)DECR_PTR(check_top, (top_size - 4));
      sbrk(-20000);
      top_size -= 20000;

      *check_top = NEW_TAG(top_size, 1);
      check_top = (int*)INCR_PTR(check_top, (top_size - 4));

      *check_top = NEW_TAG(top_size, 1);
    }
  }
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
  fprintf(stdout, "Total number of my_malloc() calls: %d\n", number_calls);
}

void* createNew(int size, int best_size)
{
  int full_size = ALIGN(size);
  fprintf(stdout, "full_size: %d\n", full_size);

  if(best_size == -1 || best_size == 131073)
  {
    int* start = (int*)sbrk(full_size + 8);
    int* free_end = (int*)sbrk(0);

    top_block = free_end;
    if(full_size > (size + sizeof(FreeListNode) +  8))
    {

      *start = NEW_TAG(size, 0);
      start = (int*)INCR_PTR(start, 4);
      int* end = (int*)INCR_PTR(start, size);
      *end = NEW_TAG(size, 0);

      int* free_start = (int*)INCR_PTR(end, 4);
      *free_start = NEW_TAG((full_size - size), 1);
      free_start = (int*)INCR_PTR(free_start, 4);

      FreeListNode *new = (FreeListNode*)free_start;
      //FreeListNode *temp = head;
      addNode(new);

      fprintf(stdout, "new free node: %p\n", new);

      fprintf(stdout, "new free node next: %p\n", new->next);

      fprintf(stdout, "new free node prev: %p\n", new->prev);

      free_end = (int*)INCR_PTR(free_start, (full_size-(size + 8)));
      *free_end = NEW_TAG((full_size - size), 1);

      return (void*)start;
    } else
    {
      //int* start = (int*)sbrk(full_size + 8);

      *start = NEW_TAG(full_size, 0);
      start = (int*)INCR_PTR(start, 4);
      int* end = (int*)INCR_PTR(start, full_size);
      *end = NEW_TAG(full_size, 0);

      fprintf(stdout, "Allocated %d extra bytes to requested malloc()\n", full_size - size);
      return (void*)start;
    }
  }else
  {
    FreeListNode *iter = head;
    while(iter != NULL)
    {
      int* find = (int*)(iter);

      find = (int*)DECR_PTR(find, 4);
      unsigned avail_size = GET_TAG_SIZE(find[0]);
      if(avail_size == best_size)
        break;
      else
        iter = iter->next;

    }
    int* check = (int*)(iter);

    check = (int*)DECR_PTR(check, 4);
    unsigned avail_size = GET_TAG_SIZE(check[0]);
    fprintf(stdout, "size:%d\n", avail_size);
    if(avail_size - 8 >= size && avail_size > (size + sizeof(FreeListNode) + 8))
    {
      fprintf(stdout, "filling free block\n");
      int new_size = (avail_size - (size + 8));
      int* start = (int*)check;
      int* free_end = (int*)INCR_PTR(check, (avail_size));

      *start = NEW_TAG(size, 0);
      start = (int*)INCR_PTR(start, 4);
      int* end = (int*)INCR_PTR(start, size);
      *end = NEW_TAG(size, 0);

      int* free_start = (int*)INCR_PTR(end, 4);
      *free_start = NEW_TAG(new_size, 1);
      free_start = (int*)INCR_PTR(free_start, 4);

      removeNode(iter);

      FreeListNode *new = (FreeListNode*)free_start;
      addNode(new);

      fprintf(stdout, "current free node: %p\n", new);
      fprintf(stdout, "current free node next: %p\n", new->next);
      fprintf(stdout, "current free node prev: %p\n", new->prev);

      free_end = (int*)DECR_PTR(free_end, 4);
      *free_end = NEW_TAG(new_size, 1);
      int val = GET_TAG_SIZE(free_end[0]);
      fprintf(stdout, "val: %d\n", val);
      return (void*)start;
    } else if(avail_size - 8 >= size)
    {
      int* start = (int*)check;

      *start = NEW_TAG((avail_size-8), 0);
      start = (int*)INCR_PTR(start, 4);
      int* end = (int*)INCR_PTR(start, (avail_size-8));
      *end = NEW_TAG((avail_size-8), 0);

      fprintf(stdout, "removed free node: %p\n", iter);

      fprintf(stdout, "removed free node next: %p\n", iter->next);

      fprintf(stdout, "removed free node prev: %p\n", iter->prev);

      if(iter->next != NULL)
      iter->next->prev = iter->prev;
      if(iter->prev != NULL)
      iter->prev->next = iter->next;

      iter->next = NULL;
      iter->prev = NULL;
      iter = NULL;

      fprintf(stdout, "Allocated %d extra bytes to requested malloc()\n", avail_size - (size + 8));
      return (void*)start;
    }
  }
}

void removeNode(FreeListNode *iter)
{
  if(iter->prev != NULL)
    iter->prev->next = iter->next;
  if(iter->next != NULL)
    iter->next->prev = iter->prev;

  if(iter == head)
  {
    head = iter->next;
  }
  if(iter == tail)
  {
    tail = iter->prev;
  }

  iter->next = NULL;
  iter->prev = NULL;
  iter = NULL;

}

void addNode(FreeListNode *new)
{
  if(head != NULL)
  {
    head->prev = new;
    new->next = head;
    head = new;
    new->prev = NULL;
  }else
  {
    head = new;
    tail = new;
  }
}
