#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "malloc_api.h"

#define FIRST_FIT 1
#define BEST_FIT 2

#define FREELIST_ENTRY_SIZE sizeof(freeListNode)
#define MAX_FREE_BLOCK 128000
#define MALLOC_BLOCKSIZE 2048


#define ALIGN(x) ((x/MALLOC_BLOCKSIZE+ 1) * MALLOC_BLOCKSIZE)

typedef struct freeListNode
{
  int startTag, endTag;
  size_t size;
  struct freeListNode *next;
  struct freeListNode *prev;

} freeListNode;

static freeListNode freeListHead = {-1, -1, 0, NULL, NULL};

extern char *my_malloc_error;

int currentPolicy = 1;
int bytesAlloc = 0;
int freeSpace = 0;
int largestSpace = 0;

void *my_malloc(int size)
{
  //int tempStartTag;
  int correctSize = ALIGN(size + sizeof(freeListNode));
  int currentLoc = (int)sbrk(0);

  freeListNode *nextUp = freeListHead.next;
  freeListNode *previous = freeListHead.prev;
  freeListNode **nextAddr = &(freeListHead.next);
  freeListNode **prevAddr = &(freeListHead.prev);

  //check free list for open spot
  while(nextUp != NULL || previous != NULL)
  {
    if(currentPolicy == 1)
    {
      if(nextUp->size >= correctSize && nextUp != NULL)
      {   //TODO: need to modify to cut the extra space into another free list entry
        nextUp->next->prev = nextUp->prev;
        *nextAddr = nextUp->next;
        //*nextAddr->prev = &(nextUp->prev);
        return ((void*) nextUp) + sizeof(freeListNode);
      }else if (previous->size >= correctSize && previous != NULL)
      {
        previous->prev->next = previous->next;
        *prevAddr = previous->prev;
        //*prevAddr->next = &(previous->next);
        return ((void*) previous) + sizeof(freeListNode);
      }
    }else if(currentPolicy == 2)
    {

    }

    if(nextUp != NULL)
    {
      nextAddr = &(nextUp->next);
      nextUp = nextUp->next;
    }
    if(previous != NULL)
    {
      prevAddr = &(previous->prev);
      previous = previous->prev;
    }

  }
  //if no free blocks set up a new block and return the address of the pointer



  if(nextUp == NULL)
  {
    freeListNode *nextNew;

    nextNew = (freeListNode*)sbrk(size + sizeof(freeListNode));
    nextNew->startTag = currentLoc;
    nextNew->endTag = (int)sbrk(0);
    nextNew->size = size + sizeof(freeListNode);
    nextNew->next = NULL;
    nextNew->prev = NULL;
    //put extra allocated memory into free list
    int newLoc = (int)sbrk(0);
    freeListNode *newNext = (freeListNode*)sbrk(correctSize -(size + sizeof(freeListNode)));
    newNext->startTag = newLoc;
    newNext->endTag = (int)sbrk(0);
    newNext->next = freeListHead.next;
    if(newNext->next != NULL)
      newNext->next->prev = newNext;
    newNext->prev = &(freeListHead);

    return ((void*)nextNew) + sizeof(freeListNode);
  }else if(previous == NULL)
  {
    freeListNode *prevNew;

    prevNew = (freeListNode*)sbrk(size + sizeof(freeListNode));
    prevNew->startTag = currentLoc;
    prevNew->endTag = (int)sbrk(0);
    prevNew->size = size + sizeof(freeListNode);
    prevNew->next = NULL;
    prevNew->prev = NULL;
    //put extra allocated memory into free list
    int newLoc = (int)sbrk(0);
    freeListNode *newPrev = (freeListNode*)sbrk(correctSize -(size + sizeof(freeListNode)));
    newPrev->startTag = newLoc;
    newPrev->endTag = (int)sbrk(0);
    newPrev->prev = freeListHead.prev;
    if(newPrev->prev != NULL)
      newPrev->prev->next = newPrev;
    newPrev->next = &(freeListHead);

    return ((void*)prevNew) + sizeof(freeListNode);
  }
  bytesAlloc += correctSize;
  //error handling for my_malloc
  //my_malloc_error = "Error, mallocing required memory";
  //fprintf(stderr, "%s\n", my_malloc_error);
  return NULL;
}

void my_free(void *ptr)
{
  if(ptr == NULL)
    return;
  freeListNode *new = (freeListNode*)(((char*)ptr) - sizeof(freeListNode));
  new->next = freeListHead.next;
  new->next->prev = new;
  new->prev = &(freeListHead);

  freeListHead.next = new;
  freeSpace += new->size;
  //set uo required data to check adjacent free blocks
  freeListNode *next = new->next;
  freeListNode *previous = new->prev;
  freeListNode **nextAddr = &(new->next);
  freeListNode **prevAddr = &(new->prev);
  //if the block we are freeing has adjacent free block combine them
  while(next != NULL || previous != NULL)
  {
    if(next != NULL)
    {
      //check corresponding tags, if they are equal update free list
      if(next->startTag == new->endTag)
      {
        new->endTag = next->endTag;
        new->size += next->size;
        next->next->prev = next->prev;
        *nextAddr = next->next;
      //check corresponding tags, if they are equal update free list
      } else if(next->endTag == new->startTag)
      {
        new->startTag = next->startTag;
        new->size += next->size;
        next->next->prev = next->prev;
        *nextAddr = next->next;
      }
    }
    if(previous != NULL)
    {
      //check corresponding tags, if they are equal update free list
      if(previous->startTag == new->endTag)
      {
        new->endTag = previous->endTag;
        new->size += previous->size;
        previous->prev->next = previous->next;
        *prevAddr = previous->prev;
      //check corresponding tags, if they are equal update free list
      } else if(previous->endTag == new->startTag)
      {
        new->startTag = previous->startTag;
        new->size += previous->size;
        previous->prev->next = previous->next;
        *prevAddr = previous->prev;
      }
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
}
