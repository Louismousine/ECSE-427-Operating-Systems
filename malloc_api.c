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

static freeListNode *freeListHead = {NULL, NULL, 0, NULL};
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
  int mallocSize = ALIGN(alignSize);

  freeListNode *nextUp = freeListHead;
  //check free list for open spot
  while(nextUp->next != NULL)
  {
    if(currentPolicy == 1)  //First fit
    {
        if(nextUp->next->size >= size )
        {

          if(nextUp->next->size > size + MIN_FREE)
          {
            fprintf(stdout, "spliting free block that was found\n");
            freeListNode *newSpace = (void*)((char*)(nextUp->next->startTag));
            newSpace->startTag = nextUp->next->startTag;
            newSpace->size = size;
            newSpace->endTag = (void*)((char*)(nextUp->next->startTag) + size );
            newSpace->next = NULL;

            nextUp->next->startTag = (void*)((char*)(nextUp->startTag) + size );
            nextUp->next->size = nextUp->next->size - size;

            fprintf(stdout, "newspace startTag: %p\nnewspace endTag: %p\nnextUp startTag: %p\nnextUp endTag: %p\n",
                    newSpace->startTag, newSpace->endTag, nextUp->next->startTag, nextUp->next->endTag);


            // fprintf(stdout, "next: %p\nnewSpace: %p\n", freeListHead.next, &(newSpace));
            //
            // fprintf(stdout, "newSpace startTag: %p\nnewSpace endTag: %p\nnextUp startTag: %p\nnextUp endTag: %p\n",
            //         newSpace->startTag, newSpace->endTag, nextUp->startTag, nextUp->endTag);

            bytesAlloc += size;
            freeSpace -= size;
            updateContiguous();

            return ((char*) newSpace) + sizeof(freeListNode);
          }else
          {
            freeListNode *ret = nextUp->next;
            nextUp->next = nextUp->next->next;
            ret->next = NULL;


            bytesAlloc += size;
            updateContiguous();
            return ((char*) ret) + sizeof(freeListNode);
          }
        }
    }else if(currentPolicy == 2) //Best Fit
    {
      if(nextUp->next != NULL)
      {
        if(nextUp->next->size >= size)  //find best possible in next
        {
          if(nextUp->next->size == size)
          {
            freeListNode *ret = nextUp->next;
            nextUp->next = nextUp->next->next;
            ret->next = NULL;

            bytesAlloc += size;
            updateContiguous();
            return ((char*) ret) + sizeof(freeListNode);

          } else if(nextUp->next->size < bestSize)
          {
            bestTag = nextUp->next->startTag;
            bestSize = nextUp->next->size;
          }
        }
      }
      //check previous free blocks if next yields no results
    }
    if(nextUp->next != NULL)
    {
      nextUp = nextUp->next;
    }
  }
  //if running a best fit approach, now search for the best result that was found and return that spot
  if(currentPolicy == 2)
  {
    freeListNode *next = freeListHead;

    while(next->next != NULL)
    {
        if(next->next->startTag == bestTag)
        {
          if(next->next->size > size + MIN_FREE)  //if space split block up
          {
            fprintf(stdout, "spliting free block that was found\n");
            freeListNode *newSpace = (void*)((char*)(next->next->startTag));
            newSpace->startTag = next->next->startTag;
            newSpace->size = size;
            newSpace->endTag = (void*)((char*)(next->next->startTag) + size);
            newSpace->next = NULL;

            next->next->startTag = (void*)((char*)(next->next->startTag) + size);
            next->next->size = next->next->size - size;

            bytesAlloc += size;
            freeSpace -= size;
            updateContiguous();

            return ((char*) newSpace) + sizeof(freeListNode);
          }else //if not return this block
          {
            freeListNode *ret = next->next;
            next->next = next->next->next;
            ret->next = NULL;

            bytesAlloc += size;
            updateContiguous();
            return ((char*) ret) + sizeof(freeListNode);
          }
        }
      if(next != NULL)
      {
        next = next->next;
      }
    }
  }
  //if no free blocks set up a new block and return the address of the pointer



  if(nextUp == NULL)
  {

    freeListNode *nextNew;
    void* currentLoc = sbrk(0);

    nextNew = (freeListNode*)sbrk(size + sizeof(freeListNode));
    nextNew->startTag = currentLoc;
    nextNew->endTag = sbrk(0);
    nextNew->size = size;
    nextNew->next = NULL;

    //fprintf(stdout, "nextNew startTag: %p\nnextNew endTag: %p\nnextNew size: %d\n", nextNew->startTag, nextNew->endTag, nextNew->size);

    //put extra allocated memory into free list
    void* newLoc = sbrk(0);
    freeListNode *newFree = (freeListNode*)sbrk(mallocSize - size + sizeof(freeListNode));
    newFree->startTag = newLoc;
    newFree->endTag = sbrk(0);
    *(newFree->next) = *(freeListHead.next);
    newFree->size = mallocSize - size;

    freeListHead.next = newFree;

    fprintf(stdout, "newFree startTag: %p\nnewFree endTag: %p\nnewFree size: %d\n", newFree->startTag, newFree->endTag, newFree->size);

    bytesAlloc += size;
    freeSpace += (mallocSize - size);
    updateContiguous();
    return ((char*)nextNew) + sizeof(freeListNode);
  }

  return NULL;
}

void my_free(void *ptr)
{
  if(ptr == NULL)
    return;
  freeListNode *new = (freeListNode*)(((char*)ptr) - sizeof(freeListNode));
  *(new->next) = *(freeListHead.next);

  freeSpace += new->size;
  bytesAlloc -= new->size;

  freeListHead.next = new;


  //set uo required data to check adjacent free blocks
  freeListNode *next = new->next;

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
      *(new->next) = *(next->next);


      next->size = 0;
      next->endTag = NULL;
      next->startTag = NULL;
      next->next = NULL;
      fprintf(stdout, "new startTag: %p\nnew endTag: %p\nnew size: %d\n", new->startTag, new->endTag, new->size);

      next = new->next;

      within = 1;
    //check corresponding tags, if they are equal update free list
    }else if(next->endTag == new->startTag)
    {
      fprintf(stdout, "next endTag: %p\nnew startTag: %p\n", next->endTag, new->startTag);
      new->startTag = next->startTag;
      new->size += next->size;
      *(new->next) = *(next->next);

      next->size = 0;
      next->endTag = NULL;
      next->startTag = NULL;
      next->next = NULL;

      fprintf(stdout, "new startTag: %p\nnew endTag: %p\nnew size: %d\n", new->startTag, new->endTag, new->size);

      next = new->next;

      within = 1;
    }
    if(!within)
    {
      if(next != NULL)
      {
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
