/*
  Memory Allocator ECSE 427 Winter 2015
  Author: Stephen Carter 260500858
  Last Edit: April 13th, 2015
  Stephen Carter (C) April 2015.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "malloc_api.h"
//define policies
#define FIRST_FIT 1
#define BEST_FIT 2
//define constants
#define MALLOC_BLOCKSIZE 16384
#define FREE_RM 20000
#define TOP_FREE 131072
#define MY_ERROR_SIZE 1024
//define Macros
#define ALIGN(x) ((x/MALLOC_BLOCKSIZE + 1) * MALLOC_BLOCKSIZE)
#define INCR_PTR(ptr, len) (((char*)ptr) + len)
#define DECR_PTR(ptr, len) (((char*)ptr) - len)
#define NEW_TAG(len,free) ((len << 1) + (free == 1 ? 0b0 : 0b1))
#define GET_TAG_SIZE(len) (len >> 1)
#define GET_TAG_FREE(ptr) (ptr & 0b1)
//struct for free blocks
typedef struct FreeListNode
{
  struct FreeListNode *next;
  struct FreeListNode *prev;
} FreeListNode;

void updateContiguous();                    //updates the global variable largestSpace (contiguous space)
void updateTopFreeBlock();                  //updates the program break if the top free block is too large
void* createNew(int size, int best_size);   //used to create/fill a block in memory
void removeNode(FreeListNode *iter);        //used to remove a free node from the free list
void addNode(FreeListNode *new);            //used to add a free node to the free list at the head
//define head and tail
FreeListNode *head;
FreeListNode *tail;

char *my_malloc_error;  //constant for error reporting
int* top_block;         //holds the current top of the program break

//global variables for my_mallinfo to report on
int currentPolicy = 1;
int bytesAlloc = 0;
int freeSpace = 0;
int largestSpace = 0;
int number_calls = 0;

void *my_malloc(int size)
{
  if(number_calls == 0)
    my_malloc_error = (char*)sbrk(1028); //set up the error reporting location if first call.

  number_calls++;
  if(size < 0)
  {
    my_malloc_error = "Error, mallocing requested space";
    return NULL;
  }
  if(currentPolicy == FIRST_FIT)
  {
    FreeListNode *iter = head;  //find the first availiable free block that can fit requested size
    while(iter != NULL)
    {
      int* check = (int*)(iter);
      check = (int*)DECR_PTR(check, 4);
      unsigned avail_size = GET_TAG_SIZE(check[0]);
      if(avail_size - 8 >= size)
        return createNew(size, avail_size);
      if(iter != NULL)
        iter = iter->next;
    }
    return createNew(size, -1); //if no block was found signal creation of a new block
  }else if(currentPolicy == BEST_FIT)
  {
    FreeListNode *iter = head;
    unsigned best = 131073;

    while(iter != NULL)   //find the best fitting free block for requested memory
    {
      int* check = (int*)(iter);
      check = (int*)DECR_PTR(check, 4);
      unsigned avail_size = GET_TAG_SIZE(check[0]);
      if(avail_size - 8 == size)
        return createNew(size, avail_size);
      else if(avail_size - 8 >= size && avail_size <= best)
        best = avail_size;
      if(iter != NULL)
        iter = iter->next;
    }
    return createNew(size, best); //return the best fir or signal for new block to be created.
  }
  my_malloc_error = "Error, mallocing required space";  //if there is an issue, return NULL
  return NULL;
}

void my_free(void *ptr)
{
  int* new_free = (int*)(ptr);  //Set up the pointer and retrieve size information
  new_free = (int*)DECR_PTR(new_free, 4);

  int free_size = GET_TAG_SIZE(new_free[0]);

  bytesAlloc -= free_size;
  freeSpace += (free_size + 8);

  int* bot_check = (int*)DECR_PTR(new_free, 4);
  int* top_check = (int*)INCR_PTR(new_free, (free_size + 8));

  int bot_free = GET_TAG_FREE(bot_check[0]);
  int top_free = GET_TAG_FREE(top_check[0]);
  //if the bottom block is the start of my_malloc() do not attempt to free the block below this.
  int bot_size_check = GET_TAG_SIZE(bot_check[0]);
  if(bot_size_check == 0)
    bot_free = -1;

  if(!bot_free && !top_free) //if both the bottom and top blocks are free
  {
    int bot_size = GET_TAG_SIZE(bot_check[0]);
    bot_check = (int*)DECR_PTR(bot_check, (bot_size - 8));

    int top_size = GET_TAG_SIZE(top_check[0]);
    top_check = (int*)INCR_PTR (top_check, 4);

    FreeListNode *rem_bot = (FreeListNode*)(bot_check); //remove the two old free list nodes
    FreeListNode *rem_top = (FreeListNode*)(top_check);
    removeNode(rem_bot);
    removeNode(rem_top);

    free_size += (bot_size + top_size + 8);             //update the size and tags
    bot_check = (int*)DECR_PTR(bot_check, 4);
    top_check = (int*)INCR_PTR(top_check, (top_size - 8));
    *bot_check = NEW_TAG(free_size, 1);
    *top_check = NEW_TAG(free_size, 1);

    int* start = (int*)INCR_PTR(bot_check, 4);
    FreeListNode *new = (FreeListNode*)start;
    addNode(new);                                       //add the appropriate new block
  }else if(!bot_free)             //if only the bottom block is free
  {
    int bot_size = GET_TAG_SIZE(bot_check[0]);
    bot_check = (int*)DECR_PTR(bot_check, (bot_size - 8));

    FreeListNode *rem_bot = (FreeListNode*)(bot_check);   //remove the previous free list node
    removeNode(rem_bot);

    bot_check = (int*)DECR_PTR(bot_check, 4);
    new_free = (int*)INCR_PTR(new_free, (free_size + 4));

    free_size += (bot_size + 8);                          //update the size and tag information
    *bot_check = NEW_TAG(free_size, 1);
    *new_free = NEW_TAG(free_size, 1);

    int* start = (int*)INCR_PTR(bot_check, 4);

    FreeListNode *new = (FreeListNode*)start;
    addNode(new);                                       //add the new free list node to the linked list
  }else if(!top_free)   //if only the top block is free
  {
    int top_size = GET_TAG_SIZE(top_check[0]);
    top_check = (int*)INCR_PTR (top_check, (4));

    FreeListNode *rem_top = (FreeListNode*)(top_check);   //remove the old free list node
    removeNode(rem_top);

    top_check = (int*)INCR_PTR(top_check, (top_size - 8));  //update the size and tag information
    free_size += (top_size + 8);
    *new_free = NEW_TAG(free_size, 1);
    *top_check = NEW_TAG(free_size, 1);

    int* start = (int*)INCR_PTR(new_free, 4);
    FreeListNode *new = (FreeListNode*)start;       //add the new free list node
    addNode(new);
  }else
  { //if there are no adjacent free blocks simply free this block and add a new ndoe to the free list
    *new_free = NEW_TAG((free_size + 8), 1);
    new_free = (int*)INCR_PTR(new_free, (free_size + 4));
    *new_free = NEW_TAG((free_size+8), 1);

    int* start = (int*)DECR_PTR(new_free, free_size);
    FreeListNode *new = (FreeListNode*)start;
    addNode(new);
  }
  updateTopFreeBlock(); //update the top free block and contiguous space
}

void updateContiguous() //check to see if there has been a change in the largest single free list block
{                       //and update the infomation
  FreeListNode *iter = head;
  int biggest = 0;
  while(iter != NULL)
  {
    int* update = (int*)iter;
    update = (int*)DECR_PTR(update, 4);
    int update_size = GET_TAG_SIZE(update[0]);
    if(update_size > biggest)
      biggest = update_size;
    iter = iter->next;
  }
  largestSpace = biggest;
}

void updateTopFreeBlock()   //check to see if the top block is a free block and udate accordingly
{
  int* check_top = (int*)(top_block);
  check_top = (int*)DECR_PTR(check_top, 4);
  int top_tag = GET_TAG_FREE(check_top[0]);
  if(top_tag == 0)
  {
    unsigned top_size = GET_TAG_SIZE(check_top[0]);

    if(top_size >=  TOP_FREE) //if top block is a free block and larger that 128KB
    {                         //decrease the program count and update information accordingly
      check_top = (int*)DECR_PTR(check_top, (top_size - 4));
      sbrk(-20000);
      top_size -= 20000;

      *check_top = NEW_TAG(top_size, 1);
      check_top = (int*)INCR_PTR(check_top, (top_size - 4));

      *check_top = NEW_TAG(top_size, 1);
      top_block = (int*)DECR_PTR(top_block, 20000);

      freeSpace -= 20000;
    }
  }
  updateContiguous(); //check to see if the largest contiguous space has changed
}

void my_mallopt(int policy) //change the memory allocation policy as specified
{
  if(policy == FIRST_FIT)
    currentPolicy = FIRST_FIT;
  else if(policy == BEST_FIT)
    currentPolicy = BEST_FIT;
  else
  {
    fprintf(stderr, "Error, not a valid policy");
    return;
  }
}

void my_mallinfo()  //print all info of my_malloc variables
{
  fprintf(stdout, "Current number of bytes allocated: %d\n", bytesAlloc);
  fprintf(stdout, "Current amount of free space: %d\n", freeSpace);
  fprintf(stdout, "Current largest contiguous free space: %d\n", largestSpace);
  fprintf(stdout, "Current policy number: %d\n", currentPolicy);
  fprintf(stdout, "Total number of my_malloc() calls: %d\n", number_calls);
}
int my_bytes()  //return bytes allocated
{
  return (int)bytesAlloc;
}
int my_free_space() //return free space
{
  return (int)freeSpace;
}
int my_largestSpace() //return largest contiguous space
{
  return (int)largestSpace;
}

void* createNew(int size, int best_size)
{
  int full_size = ALIGN(size);

  if(best_size == -1 || best_size == 131073)  //if a new block must be created do so
  {
    int* start = (int*)sbrk(full_size + 8);   //increment the program break
    int* free_end = (int*)sbrk(0);
    bytesAlloc += size;
    top_block = free_end;
    //if the requested size has enough excess to form a free block do so
    if(full_size > (size + sizeof(FreeListNode) +  8))
    {
      full_size = ALIGN(size);
    }else
      full_size = ALIGN(size + MALLOC_BLOCKSIZE);

    *start = NEW_TAG(size, 0);
    start = (int*)INCR_PTR(start, 4);
    int* end = (int*)INCR_PTR(start, size);
    *end = NEW_TAG(size, 0);

    int* free_start = (int*)INCR_PTR(end, 4);
    *free_start = NEW_TAG((full_size - size), 1);
    free_start = (int*)INCR_PTR(free_start, 4);

    FreeListNode *new = (FreeListNode*)free_start;  //add a new free block
    addNode(new);

    freeSpace += (full_size - size);
    free_end = (int*)INCR_PTR(free_start, (full_size-(size + 8)));
    *free_end = NEW_TAG((full_size - size), 1);
    updateContiguous();
    return (void*)start;
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

    if(avail_size - 8 >= size && avail_size > (size + sizeof(FreeListNode) + 8))
    {
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

      free_end = (int*)DECR_PTR(free_end, 4);
      *free_end = NEW_TAG(new_size, 1);
      int val = GET_TAG_SIZE(free_end[0]);
      //fprintf(stdout, "val: %d\n", val);
      bytesAlloc += size;
      freeSpace -= (size+8);
      updateContiguous();
      return (void*)start;
    } else if(avail_size - 8 >= size)
    {
      int* start = (int*)check;

      *start = NEW_TAG((avail_size-8), 0);
      start = (int*)INCR_PTR(start, 4);
      int* end = (int*)INCR_PTR(start, (avail_size-8));
      *end = NEW_TAG((avail_size-8), 0);

      removeNode(iter);

      bytesAlloc += (avail_size-8);
      freeSpace -= avail_size;

      fprintf(stdout, "Allocated %d extra bytes to requested malloc()\n", avail_size - (size + 8));
      updateContiguous();
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
    head = iter->next;
  if(iter == tail)
    tail = iter->prev;

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
