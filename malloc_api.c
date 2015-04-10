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

typedef struct freeListNode
{
  void* startTag;
  void* endTag;
  size_t size;
  struct freeListNode *next;
} freeListNode;

void updateContiguous();
void updateTopFreeBlock();

static freeListNode freeListHead = {NULL, NULL, 0, NULL};
extern char *my_malloc_error;

int currentPolicy = 1;
int bytesAlloc = 0;
int freeSpace = 0;
int largestSpace = 0;

void *my_malloc(int size)
{
  //int tempStartTag;
  void* bestTag;
  size_t bestSize = 128001;
  int alignSize = size + sizeof(freeListNode);
  //fprintf(stdout, "size of size: %d\n", size);
  int mallocSize = ALIGN(alignSize);
  //fprintf(stdout, "correct size is %d\n", mallocSize);
  void* currentLoc = sbrk(0);

  bytesAlloc += size;

  freeListNode *nextUp = freeListHead.next;
  freeListNode **nextAddr = &(freeListHead.next);

  //check free list for open spot
  while(nextUp != NULL)
  {
    if(currentPolicy == 1)  //First fit
    {
        if(nextUp->size >= size )
        {
          freeSpace -= size;
          if(nextUp->size > size + MIN_FREE)
          {
            fprintf(stdout, "spliting free block that was found\n");
            freeListNode *newSpace = (void*)((char*)(nextUp->startTag));
            newSpace->startTag = nextUp->startTag;
            newSpace->size = size;
            newSpace->endTag = (void*)((char*)(nextUp->startTag) + size );
            newSpace->next = NULL;

            nextUp->startTag = (void*)((char*)(nextUp->startTag) + size );
            nextUp->size = nextUp->size - size;

            fprintf(stdout, "newspace startTag: %p\nnewspace endTag: %p\nnextUp startTag: %p\nnextUp endTag: %p\n",
                    newSpace->startTag, newSpace->endTag, nextUp->startTag, nextUp->endTag);


            // fprintf(stdout, "next: %p\nnewSpace: %p\n", freeListHead.next, &(newSpace));
            //
            // fprintf(stdout, "newSpace startTag: %p\nnewSpace endTag: %p\nnextUp startTag: %p\nnextUp endTag: %p\n",
            //         newSpace->startTag, newSpace->endTag, nextUp->startTag, nextUp->endTag);

            updateContiguous();
            return ((char*) newSpace) + sizeof(freeListNode);
          }else
          {
            *nextAddr = nextUp->next;
            nextUp->next == NULL;

            updateContiguous();
            return ((char*) nextUp) + sizeof(freeListNode);
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
            *nextAddr = nextUp->next;
            nextUp->next == NULL;

            updateContiguous();
            return ((char*) nextUp) + sizeof(freeListNode);

          } else if(nextUp->size < bestSize)
          {
            bestTag = nextUp->startTag;
            bestSize = nextUp->size;
          }
        }
      }
      //check previous free blocks if next yields no results
    }
    if(nextUp != NULL)
    {
      nextAddr = &(nextUp->next);
      nextUp = nextUp->next;
    }
  }
  //if running a best fit approach, now search for the best result that was found and return that spot
  if(currentPolicy == 2)
  {
    freeListNode *next = freeListHead.next;
    freeListNode **nextOne = &(freeListHead.next);

    while(next != NULL)
    {
        if(next->startTag == bestTag)
        {
          freeSpace -= size;
          if(next->size > size + MIN_FREE)  //if space split block up
          {
            fprintf(stdout, "spliting free block that was found\n");
            freeListNode *newSpace = (void*)((char*)(next->startTag));
            newSpace->startTag = next->startTag;
            newSpace->size = size;
            newSpace->endTag = (void*)((char*)(next->startTag) + size);
            newSpace->next = NULL;

            next->startTag = (void*)((char*)(next->startTag) + size);
            next->size = next->size - size;

            updateContiguous();
            return ((char*) newSpace) + sizeof(freeListNode);
          }else //if not return this block
          {
            *nextOne = next->next;

            next->next == NULL;

            updateContiguous();
            return ((char*) next) + sizeof(freeListNode);
          }
        }
      if(next != NULL)
      {
        nextOne = &(next->next);
        next = next->next;
      }
    }
  }
  //if no free blocks set up a new block and return the address of the pointer



  if(nextUp == NULL)
  {

    freeListNode *nextNew;

    nextNew = (freeListNode*)sbrk(size + sizeof(freeListNode));
    nextNew->startTag = currentLoc;
    nextNew->endTag = sbrk(0);
    nextNew->size = size;
    nextNew->next = NULL;

    freeSpace += (mallocSize - size);


    //fprintf(stdout, "nextNew startTag: %p\nnextNew endTag: %p\nnextNew size: %d\n", nextNew->startTag, nextNew->endTag, nextNew->size);

    //put extra allocated memory into free list
    void* newLoc = sbrk(0);
    freeListNode *newNext = (freeListNode*)sbrk(mallocSize - size + sizeof(freeListNode));
    newNext->startTag = newLoc;
    newNext->endTag = sbrk(0);
    newNext->next = freeListHead.next;
    newNext->size = mallocSize - size;

    freeListHead.next = newNext;

    fprintf(stdout, "newNext startTag: %p\nnewNext endTag: %p\nnewNext size: %d\n", newNext->startTag, newNext->endTag, newNext->size);

    updateContiguous();
    return ((char*)nextNew) + sizeof(freeListNode);
  }
  updateContiguous();
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

  freeSpace += new->size;
  bytesAlloc -= new->size;

  freeListHead.next = new;


  //set uo required data to check adjacent free blocks
  freeListNode *next = new->next;
  freeListNode **nextAddr = &(new->next);

  int within = 0;
  //if the block we are freeing has adjacent free block combine them
  while(next != NULL)
  {
    within = 0;
    //check corresponding tags, if they are equal update free list
    if(next->startTag == new->endTag)
    {
      fprintf(stdout, "next startTag: %p\nnew endTag: %p\n", next->startTag, new->endTag);
      new->endTag = next->endTag;
      new->size += next->size;
      new->next = next->next;

      *nextAddr = next->next;

      next->size = 0;
      next->endTag = NULL;
      next->startTag = NULL;
      next->next = NULL;
      fprintf(stdout, "new startTag: %p\nnew endTag: %p\nnew size: %d\n", new->startTag, new->endTag, new->size);

      next = freeListHead.next;
      nextAddr = &(freeListHead.next);

      within = 1;
    //check corresponding tags, if they are equal update free list
    }else if(next->endTag == new->startTag)
    {
      fprintf(stdout, "next endTag: %p\nnew startTag: %p\n", next->endTag, new->startTag);
      new->startTag = next->startTag;
      new->size += next->size;
      new->next = next->next;

      *nextAddr = next->next;
      next->size = 0;
      next->endTag = NULL;
      next->startTag = NULL;
      next->next = NULL;

      fprintf(stdout, "new startTag: %p\nnew endTag: %p\nnew size: %d\n", new->startTag, new->endTag, new->size);

      next = freeListHead.next;
      nextAddr = &(freeListHead.next);

      within = 1;
    }
    if(!within)
    {
      if(next != NULL)
      {
        nextAddr = &(next->next);
        next = next->next;
      }
    }
  }
  updateTopFreeBlock();
  updateContiguous();
}

void updateContiguous()
{
  freeListNode *next = freeListHead.next;

  if(next != NULL)
    largestSpace = next->size;

  while(next != NULL)
  {
    if(next->size > largestSpace)
      largestSpace = next->size;

    if(next != NULL)
    {
      next = next->next;
    }
  }
}

void updateTopFreeBlock()
{
  void* topBlock = sbrk(0);

  freeListNode *next = freeListHead.next;
  freeListNode **nextAddr = &(freeListHead.next);

  while(next != NULL)
  {
    if(next->endTag == topBlock)
    {
      if(next->size >= MAX_FREE_BLOCK)
      {
        sbrk(-20000);
        next->size -= 20000;
        next->endTag = sbrk(0);
      }
    }
    if(next != NULL)
    {
      next = next->next;
    }
  }
  updateContiguous();
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
