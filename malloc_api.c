#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "malloc_api.h"

#define FIRST_FIT 1
#define BEST_FIT 2

//#define FREELIST_ENTRY_SIZE sizeof(freeListNode)
#define MAX_FREE_BLOCK 128000
#define MALLOC_BLOCKSIZE 2048


#define ALIGN(x) ((x/MALLOC_BLOCKSIZE + 1) * MALLOC_BLOCKSIZE)

typedef struct freeListNode
{
  int startTag, endTag;
  size_t size;
  struct freeListNode *next;
  struct freeListNode *prev;

} freeListNode;

void updateContiguous();

static freeListNode freeListHead = {-1, -1, 0, NULL, NULL};
extern char *my_malloc_error;

int currentPolicy = 1;
int bytesAlloc = 0;
int freeSpace = 0;
int largestSpace = 0;

void *my_malloc(int size)
{
  //int tempStartTag;
  int bestTag = 0;
  int bestSize = 128001;
  int alignSize = size + sizeof(freeListNode);
  fprintf(stdout, "size of size: %d\n", size);
  int mallocSize = ALIGN(alignSize);
  fprintf(stdout, "correct size is %d\n", mallocSize);
  int currentLoc = (int)sbrk(0);

  freeListNode *nextUp = freeListHead.next;
  freeListNode *previous = freeListHead.prev;
  freeListNode **nextAddr = &(freeListHead.next);
  freeListNode **prevAddr = &(freeListHead.prev);

  //check free list for open spot
  while(nextUp != NULL || previous != NULL)
  {
    if(currentPolicy == 1)  //First fit
    {
      if(nextUp != NULL)
      {
        if(nextUp->size >= size)
        {   //TODO: need to modify to cut the extra space into another free list entry
          if(nextUp->next != NULL)
            nextUp->next->prev = nextUp->prev;
          if(nextUp->prev != NULL)
            nextUp->prev->next = nextUp->next;
          *nextAddr = nextUp->next;

          nextUp->next == NULL;
          nextUp->prev == NULL;
          //*nextAddr->prev = &(nextUp->prev);
          return ((void*) nextUp) + sizeof(freeListNode);
        }
      } else if(previous != NULL)
      {
        if (previous->size >= size && previous != NULL)
        {
          if(previous->prev != NULL)
            previous->prev->next = previous->next;
          if(previous->next != NULL)
            previous->next->prev = previous->prev;
          *prevAddr = previous->prev;

          previous->prev == NULL;
          previous->next == NULL;
          //*prevAddr->next = &(previous->next);
          return ((void*) previous) + sizeof(freeListNode);
        }
      }
    }else if(currentPolicy == 2) //Best Fit
    {
      if(nextUp != NULL)
      {
        if(nextUp->size >= size)  //find best possible in next
        {
          if(nextUp->size == size)
          {
            if(nextUp->next != NULL)
              nextUp->next->prev = nextUp->prev;
            if(nextUp->prev != NULL)
              nextUp->prev->next = nextUp->next;
            *nextAddr = nextUp->next;

            nextUp->next == NULL;
            nextUp->prev == NULL;

            return ((void*) nextUp) + sizeof(freeListNode);

          } else if(nextUp->size < bestSize)
          {
            bestTag = nextUp->startTag;
            bestSize = nextUp->size;
          }
        }
      }
      //check previous free blocks if next yields no results
      if(previous != NULL)
      {
        if(previous->size >= size)
        {
          if(previous->size == size)
          {
            if(previous->prev != NULL)
              previous->prev->next = previous->next;
            if(previous->next != NULL)
              previous->next->prev = previous->prev;
            *prevAddr = previous->prev;

            previous->next == NULL;
            previous->prev == NULL;

            return ((void*) previous) + sizeof(freeListNode);

          } else if(previous->size < bestSize)
          {
            bestTag = previous->startTag;
            bestSize = previous->size;
          }
        }
      }
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
  //if running a best fit approach, now search for the best result that was found and return that spot
  if(currentPolicy == 2)
  {
    freeListNode *next = freeListHead.next;
    freeListNode *prev = freeListHead.prev;
    freeListNode **nextOne = &(freeListHead.next);
    freeListNode **prevOne = &(freeListHead.prev);
    while(next != NULL || prev != NULL)
    {
      if(next != NULL)
      {
        if(next->startTag == bestTag)
        {
          if(next->next != NULL)
            next->next->prev = next->prev;
          if(next->prev != NULL)
            next->prev->next = next->next;
          *nextOne = next->next;

          next->next == NULL;
          next->prev == NULL;

          return ((void*) next) + sizeof(freeListNode);
        }
      }
      if(prev != NULL)
      {
        if(prev->size >= size)
        {
          if(prev->prev != NULL)
            prev->prev->next = prev->next;
          if(prev->next != NULL)
            prev->next->prev = prev->prev;
          *prevOne = prev->prev;

          prev->next == NULL;
          prev->prev == NULL;

          return ((void*) prev) + sizeof(freeListNode);
        }
      }

      if(next != NULL)
      {
        nextOne = &(next->next);
        next = next->next;
      }
      if(prev != NULL)
      {
        prevOne = &(prev->prev);
        prev = prev->prev;
      }
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

    fprintf(stdout, "nextNew startTag: %d\nnextNew endTag: %d\nnextNew size: %d\n", nextNew->startTag, nextNew->endTag, nextNew->size);

    //put extra allocated memory into free list
    int newLoc = (int)sbrk(0);
    freeListNode *newNext = (freeListNode*)sbrk(mallocSize - size + sizeof(freeListNode));
    newNext->startTag = newLoc;
    newNext->endTag = (int)sbrk(0);
    newNext->next = freeListHead.next;
    newNext->size = mallocSize - size + sizeof(freeListNode);
    if(newNext->next != NULL)
      newNext->next->prev = newNext;
    newNext->prev = &(freeListHead);
    freeListHead.next = newNext;

    fprintf(stdout, "newNext startTag: %d\nnewNext endTag: %d\nnewNext size: %d\n", newNext->startTag, newNext->endTag, newNext->size);


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

    fprintf(stdout, "prevNew startTag: %d\nprevNew endTag: %d\nprevNew size: %d\n", prevNew->startTag, prevNew->endTag, prevNew->size);

    //put extra allocated memory into free list
    int newLoc = (int)sbrk(0);
    freeListNode *newPrev = (freeListNode*)sbrk(mallocSize - size + sizeof(freeListNode));
    newPrev->startTag = newLoc;
    newPrev->endTag = (int)sbrk(0);
    newPrev->prev = freeListHead.prev;
    newPrev->size= mallocSize - size + sizeof(freeListNode);
    if(newPrev->prev != NULL)
      newPrev->prev->next = newPrev;
    newPrev->next = &(freeListHead);
    freeListHead.next = newPrev;

    fprintf(stdout, "newPrev startTag: %d\nnewPrev endTag: %d\nnewPrev size: %d\n", newPrev->startTag, newPrev->endTag, newPrev->size);


    return ((void*)prevNew) + sizeof(freeListNode);
  }
  bytesAlloc += (size + sizeof(freeListNode));
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
  if(new->next != NULL)
    new->next->prev = new;
  new->prev = &(freeListHead);
  if(new->prev != NULL)
    new->prev->next = new;

  freeListHead.next = new;
  freeSpace += new->size;
  bytesAlloc -= new->size;
  //set uo required data to check adjacent free blocks
  freeListNode *next = new->next;
  freeListNode *previous = new->prev;
  freeListNode **nextAddr = &(new->next);
  freeListNode **prevAddr = &(new->prev);
  int within = 0;
  //if the block we are freeing has adjacent free block combine them
  while(next != NULL || previous != NULL)
  {
    within = 0;
    if(next != NULL)
    {
      //check corresponding tags, if they are equal update free list
      if(next->startTag == new->endTag)
      {
        fprintf(stdout, "next startTag: %d\nnew endTag: %d\n", next->startTag, new->endTag);
        new->endTag = next->endTag;
        new->size += next->size;
        if(next->next != NULL)
          next->next->prev = next->prev;
        *nextAddr = next->next;

        next->size = 0;
        next->next = NULL;
        next->prev = NULL;
        fprintf(stdout, "new startTag: %d\nnew endTag: %d\nnew size: %d\n", new->startTag, new->endTag, new->size);

        next = new->next;
        previous = new->prev;
        nextAddr = &(new->next);
        prevAddr = &(new->prev);
        within = 1;
      //check corresponding tags, if they are equal update free list
      }else if(next->endTag == new->startTag)
      {
        fprintf(stdout, "next endTag: %d\nnew startTag: %d\n", next->endTag, new->startTag);
        new->startTag = next->startTag;
        new->size += next->size;
        if(next->next != NULL)
          next->next->prev = next->prev;
        *nextAddr = next->next;
        next->size = 0;
        next->next = NULL;
        next->prev = NULL;
        fprintf(stdout, "new startTag: %d\nnew endTag: %d\nnew size: %d\n", new->startTag, new->endTag, new->size);

        next = new->next;
        previous = new->prev;
        nextAddr = &(new->next);
        prevAddr = &(new->prev);
        within = 1;
      }
    }
    if(previous != NULL)
    {
      //check corresponding tags, if they are equal update free list
      if(previous->startTag == new->endTag)
      {
        fprintf(stdout, "previous startTag: %d\nnew endTag: %d\n", previous->startTag, new->endTag);
        new->endTag = previous->endTag;
        new->size += previous->size;
        if(previous->prev != NULL)
          previous->prev->next = previous->next;
        *prevAddr = previous->prev;
        previous->size = 0;
        previous->next = NULL;
        previous->prev = NULL;
        fprintf(stdout, "previous startTag: %d\nprevious endTag: %d\nprevious size: %d\n", previous->startTag, previous->endTag, previous->size);

        next = new->next;
        previous = new->prev;
        nextAddr = &(new->next);
        prevAddr = &(new->prev);
        within = 1;
      //check corresponding tags, if they are equal update free list
      }else if(previous->endTag == new->startTag)
      {
        fprintf(stdout, "previous endTag: %d\nnew startTag: %d\n", previous->endTag, new->startTag);

        new->startTag = previous->startTag;
        new->size += previous->size;
        if(previous->prev != NULL)
          previous->prev->next = previous->next;
        *prevAddr = previous->prev;
        previous->size = 0;
        previous->next = NULL;
        previous->prev = NULL;
        fprintf(stdout, "previous startTag: %d\nprevious endTag: %d\nprevious size: %d\n", previous->startTag, previous->endTag, previous->size);

        next = new->next;
        previous = new->prev;
        nextAddr = &(new->next);
        prevAddr = &(new->prev);
        within = 1;
      }
    }
    if(!within)
    {
      if(next != NULL)
      {
        nextAddr = &(next->next);
        next = next->next;
      }
      if(previous != NULL)
      {
        prevAddr = &(previous->prev);
        previous = previous->prev;
      }
    }
  }
  updateContiguous();
}

void updateContiguous()
{
  freeListNode *next = freeListHead.next;
  freeListNode *prev = freeListHead.prev;
  freeListNode **nextAddr = &(freeListHead.next);
  freeListNode **prevAddr = &(freeListHead.prev);

  while(next != NULL || prev != NULL)
  {
    if(next != NULL)
    {
      if(next->size > largestSpace)
        largestSpace = next->size;
    }
    if(prev != NULL)
    {
      if(prev->size > largestSpace)
        largestSpace = prev->size;
    }

    if(next != NULL)
    {
      nextAddr = &(next->next);
      next = next->next;
    }
    if(prev != NULL)
    {
      prevAddr = &(prev->prev);
      prev = prev->prev;
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
