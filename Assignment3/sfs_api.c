#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "disk_emu.h"
#include "sfs_api.h"

#define MAGIC_NUMBER 256
#define BLOCKSIZE 512 //M block size
#define MAXFILENAME 15
#define NUM_BLOCKS SUPERBLOCK_SIZE + FREELIST_SIZE + DIRECTORY_SIZE + INODE_TABLE_SIZE + BLOCKSIZE  //N blocks
#define MAX_FILES 100

#define SUPERBLOCK 0
#define SUPERBLOCK_SIZE 1
#define FREELIST 1
#define FREELIST_SIZE 1
#define DIRECTORY_LOCATION 2
#define DIRECTORY_SIZE 4
#define INODE_TABLE DIRECTORY_LOCATION + DIRECTORY_SIZE
#define INODE_TABLE_SIZE 13
#define START INODE_TABLE + INODE_TABLE_SIZE

#define FILENAME "my.sfs"

#define ALIGN(x)((x/BLOCKSIZE + 1) * BLOCKSIZE)

typedef struct directoryEntry
{
  char filename[MAXFILENAME + 1];
  unsigned int inode;
} directoryEntry;

typedef struct inodeEntry
{
  unsigned int mode;
  unsigned int linkCount;
  unsigned int size;
  unsigned int directptr[12];
  unsigned int singleIndirectPtr;
} inodeEntry;

typedef struct fileDescriptorEntry
{
  unsigned int inode;
  unsigned int readPointer;
  unsigned int writePointer;
} fileDescriptorEntry;

int createInodeTable();
int createRootDir();
int createSuperblock();
int createFreelist();               //create a freelist as bitmap
void setFree(unsigned int index);   //set index to free in FREELIST
void setAlloc(unsigned int index);  //set index to allocated in FREELIST
int findFree();                     //find next free block in sfs
int findFreeSeq(unsigned int size); //find next free block sequence for required size

int dirLoc = 0;
int numFiles;
directoryEntry *rootDir;
inodeEntry *inodeTable;
fileDescriptorEntry **descriptorTable;

int mksfs(int fresh)
{
  if(fresh == 1)
  {
    if(access(FILENAME, F_OK) != -1)
      unlink(FILENAME);

    if(init_fresh_disk(FILENAME, BLOCKSIZE, NUM_BLOCKS) != 0)
    {
      fprintf(stderr, "Error creating fresh file system");
      return -1;
    }

    if(createSuperblock() != 0)
    {
      fprintf(stderr, "Error creating superblock");
      return -1;
    }

    if(createFreeList() != 0)
    {
      fprintf(stderr, "Error creating free list");
      return -1;
    }

    if(createRootDir() != 0)
    {
      fprintf(stderr, "Error creating root directory");
      return -1;
    }

    unsigned int *updateBuff = malloc(BLOCKSIZE);

    if(createInodeTable() != 0)
    {
      fprintf(stderr, "Error creating i-node table");
      return -1;
    }

    inodeEntry *inode = malloc(ALIGN(MAX_FILES*sizeof(inodeEntry)));
    read_blocks(INODE_TABLE, INODE_TABLE_SIZE, inode);
    if(inode == 0)
    {
      return -1;
    }
    //set first inode to point to directory
    inode[0].size = DIRECTORY_SIZE*BLOCKSIZE;
    inode[0].linkCount = DIRECTORY_SIZE;
    inode[0].mode = 1;

    if(DIRECTORY_SIZE > 12)     //check to see if we need to use singleindirectptr
    {
      inode[0].singleIndirectPtr = findfree();
      setAlloc(inode[0].singleIndirectPtr);
      unsigned int *buff = malloc(BLOCKSIZE);
      write_blocks(inode[0].singleIndirectPtr, 1, buff);
      free(buff);
    }
    //assign the pointers the location of directory files
    int k;
    for(k = 0; k < DIRECTORY_SIZE; k++)
    {
      if(k > 11)
      {
        unsigned int *buff = malloc(BLOCKSIZE);
        read_blocks(inode[0].singleIndirectPtr, 1, buff);
        buff[k - 12] = DIRECTORY_LOCATION + k;
        write_blocks(inode[0].singleIndirectPtr, 1, buff);
        free(buff);
      } else {
        inode[0].directptr[k] = DIRECTORY_LOCATION + k;
      }
    }
    //update the inode and free main memory
    write_blocks(INODE_TABLE, INODE_TABLE_SIZE, inode);
    free(inode);

  }else if(fresh == 0) // initialize file system from an already existing file system
  {
    if(init_disk(FILENAME, BLOCKSIZE, NUM_BLOCKS) != 0)
    {
      fprintf(stderr, "Error initializing disk");
      return -1;
    }
  }
  //allocate main memory for filesystem data structures
  int *superblock = malloc(BLOCKSIZE*SUPERBLOCK_SIZE);

  if(superblock == 0)
  {
    fprintf(stderr, "Error allocating main memory for superblock");
    return -1;
  }

  read_blocks(SUPERBLOCK, SUPERBLOCK_SIZE, superblock);

  rootDir = malloc(ALIGN(sizeof(directoryEntry)*MAX_FILES));


  if(rootDir == 0)
  {
    fprintf(stderr, "Error allocating main memory for directory");
    return -1;
  }

  read_blocks(DIRECTORY_LOCATION, DIRECTORY_SIZE, rootDir);

  inodeTable = malloc(ALIGN(sizeof(inodeEntry)*MAX_FILES));

  if(inodeTable == 0)
  {
    fprintf(stderr, "Error allocating main memory for i-node cache");
    return -1;
  }

  read_blocks(INODE_TABLE, INODE_TABLE_SIZE, inodeTable);

  numFiles = 0;
  return 0;
}
//open or create file with given name
int sfs_fopen(char *name)
{
  if(sizeof(name) > MAXFILENAME) //check to see if filename is of correct size
  {
    fprintf(stderr, "File name too long");
    return -1;
  }
  //Check to see if filesystem has been setup
  if(rootDir == 0)
  {
    fprintf(stderr, "File system not initiallized");
    return -1;
  }

  //search for file
  int i;

  for(i = 0; i < MAX_FILES; i++)
  {
    if(strncmp(rootDir[i].filename, name, MAXFILENAME + 1) == 0)
    {
      int j,entry = -1;
      //Check to see if file is already open
      for(j = 0; j < numFiles; j++)
      {
        if(descriptorTable[j] && rootDir[i].inode == descriptorTable[j]->inode)
        {
          return j;
        }
      }
      //create a file descriptor slot for file
      for(j = 0; j < numFiles; j++)
      {
        if(!descriptorTable[j])
        {
          descriptorTable[j] = malloc(sizeof(fileDescriptorEntry));
          entry = j;
          break;
        }
      }
      //create a new descriptor entry if required
      if(entry == -1)
      {
        descriptorTable = realloc(descriptorTable, (1+numFiles)*(sizeof(fileDescriptorEntry*)));
        descriptorTable[numFiles] = (fileDescriptorEntry *) malloc(sizeof(fileDescriptorEntry));
        entry = numFiles;
        numFiles++;
      }
      //fill table slot with required infomation
      fileDescriptorEntry *update = descriptorTable[entry];
      if(update == 0)
      {
        fprintf(stderr, "Error opening requested file");
        return -1;
      }

      update->readPointer = 0;
      update->writePointer = inodeTable[rootDir[i].inode].size;
      update->inode = rootDir[i].inode;
      return entry;
    }
  }

  //since file does not exist create new one
  for(i = 0; i < MAX_FILES; i++)
  {
    //find spot in directory
    if(strncmp(rootDir[i].filename, "\0", 1) == 0)
    {
      int entry = -1;

      int j;
      //find a spot in descriptor table
      for(j = 0; j < numFiles; j++)
      {
        if(descriptorTable[j] == NULL)
        {
          descriptorTable[j] = malloc(sizeof(fileDescriptorEntry));
          entry = j;
          break;
        }
      }
      //if no slots left create a new one
      if(entry == -1)
      {
        descriptorTable = realloc(descriptorTable, (1+numFiles)*(sizeof(fileDescriptorEntry)));
        descriptorTable[numFiles] = (fileDescriptorEntry *) malloc(sizeof(fileDescriptorEntry));
        entry = numFiles;
        numFiles++;
      }

      //allocate inode for new entry
      fileDescriptorEntry *newEntry = descriptorTable[entry];

      if(newEntry == 0)
      {
        fprintf(stderr, "Error creating new file");
        return -1;
      }
      int inode = -1;
      int k;
      for(k = 1; k < MAX_FILES; k++)
      {
        if(inodeTable[k].mode == 0)
        {
          inode = k;
          break;
        }
      }
      if(inode == -1)
      {
        fprintf(stderr, "Error, i-node Table full");
        return -1;
      }
      //find next free location to create new file in
      int writeLoc = findFree();
      if(writeLoc == -1)
        return -1;

      setAlloc(writeLoc);

      newEntry->writePointer = 0;
      newEntry->readPointer = 0;
      newEntry->inode = inode;

      //update rootDir
      strncpy(rootDir[i].filename, name, MAXFILENAME+1);
      rootDir[i].inode = inode;
      write_blocks(DIRECTORY_LOCATION, DIRECTORY_SIZE, rootDir);

      //update inode
      inodeTable[inode].size = 0;
      inodeTable[inode].linkCount = 1;
      inodeTable[inode].mode = 1;
      inodeTable[inode].directptr[0] = writeLoc;
      write_blocks(INODE_TABLE,INODE_TABLE_SIZE,inodeTable);
      return entry;
    }
  }
  return -1;
}

int sfs_fclose(int fileID)
{
  //check file is open and has not already been closed
  if(fileID >= numFiles || descriptorTable[fileID] == NULL)
  {
    fprintf(stderr, "File has already been closed");
    return -1;
  }

  free(descriptorTable[fileID]);
  descriptorTable[fileID] = NULL;
  return 0;
}

int sfs_remove(char *file) //remove file from disk
{
  int i;  //seatch for desginated file
  for(i = 0; i < MAX_FILES; i++);
  {
    if(strncmp(rootDir[i].filename, file, MAXFILENAME + 1) == 0)  //if we find the file remove data and set space to free
    {
      directoryEntry *removeEntry = &(rootDir[i]);  //update root directory
      int inode = removeEntry->inode;
      strcpy(removeEntry->filename, "\0");
      inodeEntry *inodeRemove = &(inodeTable[inode]);
      removeEntry->inode = (int) NULL;
      int k;
        if(inodeRemove->linkCount > 12) //update i-node data
        {
          unsigned int *buff = malloc(BLOCKSIZE);
          read_blocks(inodeRemove->singleIndirectPtr, 1, buff);

          for(k = 0; k < inodeRemove->linkCount - 12; k++)
          {
            setFree(buff[k]);
          }
          free(buff);

          inodeRemove->linkCount = inodeRemove->linkCount - 12;
          inodeRemove->singleIndirectPtr = (int) NULL;
        }
        for(k = 0; k < 12; k++)
        {
          setFree(inodeRemove->directptr[k]);
          inodeRemove->directptr[k] = (int) NULL;
        }

        inodeRemove->mode = 0;
        inodeRemove->linkCount = 0;

        write_blocks(INODE_TABLE, INODE_TABLE_SIZE, inodeTable); //update inode data on disk
        return 0;
    }
  }
  fprintf(stderr, "Not file of that name found"); //if no mactching file return -1
  return -1;
}

int sfs_get_next_filename(char* filename) //get next filename located in directory
{
  if(dirLoc == MAX_FILES)   //if end of directory return 0
  {
    return 0;
  }
  strncpy(filename, rootDir[dirLoc].filename, MAXFILENAME + 1); //copy filename over and increment dirLoc
  dirLoc++;
  return 1;
}

int sfs_GetFileSize(const char* path) //returns file size of request file
{
  int i;
  for(i = 0; i < MAX_FILES; i++)
  {
    if(strncmp(rootDir[i].filename, path, MAXFILENAME + 1) == 0) //if request file name exists return size
    {
      unsigned int inode = rootDir[i].inode;
      unsigned int size = inodeTable[inode].size;
      return size;
    }
  }
  return -1;  //if not return -1;
}

int sfs_fseek(int fileID, int offset) //error if user tries to seek past eof or fileID dne
{
  if(fileID >= numFiles || descriptorTable[fileID] == NULL || inodeTable[descriptorTable[fileID]->inode].size < offset )
  {
    fprintf(stderr, "Error, seeking to requested location in requested file");
    return -1;
  }

  //shift read and write pointers to offset return 0 for success
  descriptorTable[fileID]->writePointer = offset;
  descriptorTable[fileID]->readPointer = offset;
  return 0;
}

int sfs_fwrite(int fileID, const char *buf, int length)
{
  //if invalid file or requested return -1
  if(fileID >= numFiles || descriptorTable[fileID] == NULL || buf == NULL || length < 0)
  {
    fprintf(stderr, "Error in write request");
    return -1; //return -1 on failure
  }
  int writeLength = length;

  fileDescriptorEntry *writeFile = descriptorTable[fileID];
  inodeEntry *inode = &(inodeTable[writeFile->inode]);

  char *diskBuffer = malloc(BLOCKSIZE);

  int block = (writeFile->writePointer)/BLOCKSIZE;  //get block location to write to
  int bytes = (writeFile->writePointer)/BLOCKSIZE;   //get exact byte location to write to

  //int i;
  unsigned int writeLoc;
  int offset = 0;
  if(block > 11)     //get location of block in located in single indirect pointers
  {
    unsigned int *indirectBuff = malloc(BLOCKSIZE);
    read_blocks(inode->singleIndirectPtr, 1, indirectBuff);
    writeLoc = indirectBuff[block - 12];
    free(indirectBuff);
  }else             //if not get location of block in direct pointers
    writeLoc = inode->directptr[block];

  while(length > 0)
  {
    read_blocks(START + writeLoc, 1, diskBuffer);
    int byteWrite;
    if(BLOCKSIZE - bytes < length)
    {
      byteWrite = BLOCKSIZE - bytes;
    }else
      byteWrite = length;

    memcpy(diskBuffer + bytes, buf+ offset, byteWrite);
    write_blocks(START + writeLoc, 1, diskBuffer);

    length -= (BLOCKSIZE - bytes);
    offset += (BLOCKSIZE - bytes);
    bytes = 0;

    if(length > 0)  //if there is data still to write update writeloc and allocate memory
    {
      int next = findFree();  //find next write location
      setAlloc(next);
      if(next == -1)
        return -1;
      writeLoc = next;
      block++;
      if(block > 11)          //update i-node associated with file
      {
        unsigned int *nextBuff = malloc(BLOCKSIZE);
        read_blocks(inode->singleIndirectPtr, 1, nextBuff);
        nextBuff[block - 12] = writeLoc;
        write_blocks(inode->singleIndirectPtr, 1, nextBuff);
        free(nextBuff);
      }else
        inode->directptr[block] = writeLoc;

      inode->linkCount++; //update link count
    }
  }
  //update size of file in inode entry
  if(writeFile->writePointer + writeLength > inode->size)
  {
    inode->size = writeFile->writePointer + writeLength;
  }
  //update writer pointer in file descriptor entry
  writeFile->writePointer += writeLength;
  //upodate inode table on disk
  write_blocks(INODE_TABLE, INODE_TABLE_SIZE, inodeTable);
  free(diskBuffer);
  return writeLength; //return length of written data
}

int sfs_fread(int fileID, char *buf, int length) //returns -1 for failure
{
  if(descriptorTable[fileID] == NULL || length < 0 || fileID >= numFiles)
  {
    fprintf(stderr, "Error in read request");
    return -1;
  }

  fileDescriptorEntry *readFile = descriptorTable[fileID];

  inodeEntry *inode = &(inodeTable[readFile->inode]);

  if(readFile->readPointer + length > inode->size)
  {
    length = inode->size - readFile->readPointer;
  }
  int readLength = length;
  char *diskBuffer = malloc(BLOCKSIZE);

  int block = (readFile->readPointer)/BLOCKSIZE;  //get block location to read from
  int bytes = (readFile->readPointer)/BLOCKSIZE;   //get exact byte location to read from

  unsigned int readLoc;
  int offset = 0;
  if(block > 11)     //get location of block in located in single indirect pointers
  {
    unsigned int *indirectBuff = malloc(BLOCKSIZE);
    read_blocks(inode->singleIndirectPtr, 1, indirectBuff);
    readLoc = indirectBuff[block - 12];
    free(indirectBuff);
  }else             //if not get location of block in direct pointers
    readLoc = inode->directptr[block];

  while(length > 0)
  {
    read_blocks(START + readLoc, 1, diskBuffer);
    int bytesRead;

    if(BLOCKSIZE - bytes < length)
    {
      bytesRead = BLOCKSIZE - bytes;
    }else
      bytesRead = length;

    memcpy(buf + offset, diskBuffer + bytes, bytesRead);

    length -= (BLOCKSIZE - bytes);
    offset += (BLOCKSIZE - bytes);
    bytes = 0;

    if(length > 0) //check to see if there is more to read;
    {
      block++;
      if(block > 11)          //update i-node associated with file
      {
        unsigned int *nextBuff = malloc(BLOCKSIZE);
        read_blocks(inode->singleIndirectPtr, 1, nextBuff);
        readLoc = nextBuff[block - 12];
        write_blocks(inode->singleIndirectPtr, 1, nextBuff);
        free(nextBuff);
      }else
        readLoc = inode->directptr[block];

      if(readLoc == (int) NULL) //if trying to read past file return -1;
        return -1;
    }
  }

  free(diskBuffer);
  //update read pointer in descriptor table
  readFile->readPointer += readLength;
  return readLength;
}

int createFreeList()
{
  unsigned int *buff = malloc(BLOCKSIZE);
  if (buff == 0)
  {
    return -1;
  }
  int i;
  for(i = 0; i < (BLOCKSIZE)/sizeof(unsigned int); i++)
  {
    buff[i] = ~0;
  }

  write_blocks(FREELIST, FREELIST_SIZE, buff);
  free(buff);
  return 0;
}

void setFree(unsigned int index)
{
  int byte = index / (8*sizeof(unsigned int));
  int bit = index % (8*sizeof(unsigned int));

  unsigned int *buff = malloc(BLOCKSIZE);

  if(buff == 0)
  {
    fprintf(stderr, "Error assigning free bit");
    return;
  }

  read_blocks(FREELIST, FREELIST_SIZE, buff);
  buff[byte] |= 1 << bit; //sets bit
  write_blocks(FREELIST, FREELIST_SIZE, buff);
}

void setAlloc(unsigned int index) //set index to allocated in FREELIST
{
  int byte = index / (8*sizeof(unsigned int));  //find byte to change
  int bit = index % (8*sizeof(unsigned int));   //find bit to change

  unsigned int *buff = malloc(BLOCKSIZE);

  if(buff == 0)
  {
    fprintf(stderr, "Error assigning allocated bit");
    return;
  }

  read_blocks(FREELIST, FREELIST_SIZE, buff);
  buff[byte] &= ~(1 << bit);
  write_blocks(FREELIST, FREELIST_SIZE, buff);
}

int findFree()
{
  unsigned int *buff = malloc(BLOCKSIZE);

  if(buff == 0)
  {
    fprintf(stderr, "Error finding free bit");
    return -1;
  }

  read_blocks(FREELIST, FREELIST_SIZE, buff);

  int i;
  for(i = 0; i < (BLOCKSIZE)/sizeof(unsigned int); i++)
  {
    int find = ffs(buff[i]);
    if(find != 0)
    {
      return find + i*8*sizeof(unsigned int) - 1;
    }
  }

  return -1;
}

int createSuperblock()
{
  unsigned int *buff = malloc(BLOCKSIZE);

  if(buff == 0)
  {
    return -1;
  }
  buff[0] = MAGIC_NUMBER;
  buff[1] = BLOCKSIZE;
  buff[2] = NUM_BLOCKS;
  buff[3] = FREELIST;
  buff[4] = DIRECTORY_LOCATION;
  buff[5] = DIRECTORY_SIZE;
  buff[6] = INODE_TABLE;
  buff[7] = INODE_TABLE_SIZE;
  buff[8] = START;

  write_blocks(SUPERBLOCK, SUPERBLOCK_SIZE, buff);
  free(buff);
  return 0;
}

int createRootDir()
{
  directoryEntry *buff = malloc(ALIGN(MAX_FILES*sizeof(directoryEntry)));

  if(buff == 0)
  {
    return -1;
  }

  int i;
  for(i = 0; i < MAX_FILES; i++)
  {
    buff[i] = (directoryEntry){.filename = "\0",.inode = (int) NULL};
  }

  write_blocks(DIRECTORY_LOCATION, DIRECTORY_SIZE, buff);
  free(buff);
  return 0;
}

int createInodeTable()
{
  inodeEntry *buff = malloc(ALIGN(MAX_FILES*sizeof(inodeEntry)));

  if(buff == 0)
  {
    return -1;
  }

  int i;
  for(i = 0; i < MAX_FILES; i++)
  {
    buff[i].mode = 0;                                 //mode
    buff[i].linkCount = 0;                            //link count
    buff[i].size = 0;                                 //size
    buff[i].singleIndirectPtr = (int) NULL;       //indirect pointers

    int j;
    for(j = 0; j < 12; j++)
    {
      buff[i].directptr[j] = (int) NULL;
    }
  }

  write_blocks(INODE_TABLE, INODE_TABLE_SIZE, buff);
  free(buff);
  return 0;
}