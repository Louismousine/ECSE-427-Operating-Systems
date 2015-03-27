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
#define MAX_FILES 99

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
  unsigned int rwPointer;
} fileDescriptorEntry;

int createInodeTable();
int createRootDir();
int createSuperblock();
int createFreelist();               //create a freelist as bitmap
void setFree(unsigned int index);   //set index to free in FREELIST
void setAlloc(unsigned int index);  //set index to allocated in FREELIST
int findFree();                     //find next free block in sfs


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
      fprintf(stderr, "Error creating fresh file system\n");
      return -1;
    }

    if(createSuperblock() != 0)
    {
      fprintf(stderr, "Error creating superblock");
      return -1;
    }


    if(createFreeList() != 0)
    {
      fprintf(stderr, "Error creating free list\n");
      return -1;
    }
    //setAlloc(SUPERBLOCK);
    //setAlloc(FREELIST);

    if(createRootDir() != 0)
    {
      fprintf(stderr, "Error creating root directory\n");
      return -1;
    }

    if(createInodeTable() != 0)
    {
      fprintf(stderr, "Error creating i-node table\n");
      return -1;
    }

    inodeEntry *inode = malloc(ALIGN((MAX_FILES+1)*sizeof(inodeEntry)));
    read_blocks(INODE_TABLE, INODE_TABLE_SIZE, inode);
    if(inode == NULL)
    {
      return -1;
    }
    //set first inode to point to directory
    inode[0].size = DIRECTORY_SIZE*BLOCKSIZE;
    inode[0].linkCount = DIRECTORY_SIZE;
    inode[0].mode = 1;

    if(DIRECTORY_SIZE > 12)     //check to see if we need to use singleindirectptr
    {
      inode[0].singleIndirectPtr = findFree();
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
        //setAlloc(DIRECTORY_LOCATION + k);
      } else {
        inode[0].directptr[k] = DIRECTORY_LOCATION + k;
        //setAlloc(DIRECTORY_LOCATION + k);
      }
    }
    //update the inode and free main memory
    write_blocks(INODE_TABLE, INODE_TABLE_SIZE, inode);
    free(inode);

  }else if(fresh == 0) // initialize file system from an already existing file system
  {
    if(init_disk(FILENAME, BLOCKSIZE, NUM_BLOCKS) != 0)
    {
      fprintf(stderr, "Error initializing disk\n");
      return -1;
    }
  }
  //allocate main memory for filesystem data structures
  int *superblock = malloc(BLOCKSIZE*SUPERBLOCK_SIZE);

  if(superblock == NULL)
  {
    fprintf(stderr, "Error allocating main memory for superblock\n");
    return -1;
  }

  read_blocks(SUPERBLOCK, SUPERBLOCK_SIZE, superblock);

  rootDir = malloc(ALIGN(sizeof(directoryEntry)*MAX_FILES));


  if(rootDir == NULL)
  {
    fprintf(stderr, "Error allocating main memory for directory\n");
    return -1;
  }

  read_blocks(DIRECTORY_LOCATION, DIRECTORY_SIZE, rootDir);

  inodeTable = malloc(ALIGN(sizeof(inodeEntry)*(MAX_FILES+1)));

  if(inodeTable == NULL)
  {
    fprintf(stderr, "Error allocating main memory for i-node cache\n");
    return -1;
  }

  read_blocks(INODE_TABLE, INODE_TABLE_SIZE, inodeTable);

  numFiles = 0;
  descriptorTable = NULL;
  return 0;
}
//open or create file with given name
int sfs_fopen(char *name)
{
  // setAlloc(5000);
  if(strlen(name) > MAXFILENAME) //check to see if filename is of correct size
  {
    fprintf(stderr, "File name too long\n");
    return -1;
  }
  //Check to see if filesystem has been setup
  if(rootDir == NULL)
  {
    fprintf(stderr, "File system not initiallized\n");
    return -1;
  }


  //search for file
  int i;

  for(i = 0; i < MAX_FILES; i++)
  {
    if(strncmp(rootDir[i].filename, name, MAXFILENAME + 1) == 0 )
    {
      if(rootDir[i].inode == 5000)
      {
        fprintf(stderr, "Error bad inode link");
        return -1;
      }
      //fprintf(stderr, "file found, opening");
      int j,entry = -1;
      //Check to see if file is already open
      for(j = 0; j < numFiles; j++)
      {
        if(descriptorTable[j] && rootDir[i].inode == descriptorTable[j]->inode && rootDir[i].inode != 5000)
        {
          return j;
        }
      }
      //create a file descriptor slot for file
      for(j = 0; j < numFiles; j++)
      {
        if(descriptorTable[j] == NULL)
        {
          descriptorTable[j] = malloc(sizeof(fileDescriptorEntry));
          entry = j;
          break;
        }
      }
      //create a new descriptor entry if required
      if(entry == -1)
      {
        if (descriptorTable == NULL)
          descriptorTable = malloc(sizeof(fileDescriptorEntry*));
        else
          descriptorTable = realloc(descriptorTable, (1+numFiles)*(sizeof(fileDescriptorEntry*)));
        descriptorTable[numFiles] = (fileDescriptorEntry *) malloc(sizeof(fileDescriptorEntry));
        entry = numFiles;
        numFiles++;
      }
      //fill table slot with required infomation
      fileDescriptorEntry *update = descriptorTable[entry];
      if(update == NULL)
      {
        fprintf(stderr, "Error opening requested file\n");
        return -1;
      }

      update->rwPointer = inodeTable[rootDir[i].inode].size;
      update->inode = rootDir[i].inode;
      //fprintf(stderr, "inode:%d\n", update->inode);
      //fprintf(stderr, "filename:%s\n", rootDir[i].filename);
      //fprintf(stderr, "fileID%d\n", entry);
      return entry;
    }
  }

  //since file does not exist create new one
  for(i = 0; i < MAX_FILES; i++)
  {
    //find spot in directory
    if(strncmp(rootDir[i].filename, "\0", 1) == 0 && rootDir[i].inode == 5000)
    {
      int entry = -1;

      int j;
      //create a file descriptor slot for file
      for(j = 0; j < numFiles; j++)
      {
        if(descriptorTable[j] == NULL)
        {
          descriptorTable[j] = malloc(sizeof(fileDescriptorEntry));
          // assert(descriptorTable[j]);
          entry = j;
          break;
        }
      }
      //create a new descriptor entry if required
      if(entry == -1)
      {
        if (descriptorTable == NULL)
          descriptorTable = malloc(sizeof(fileDescriptorEntry*));
        else
          descriptorTable = realloc(descriptorTable, (1+numFiles)*(sizeof(fileDescriptorEntry*)));

        descriptorTable[numFiles] = (fileDescriptorEntry *) malloc(sizeof(fileDescriptorEntry));
        entry = numFiles;
        numFiles++;
      }

      //allocate inode for new entry
      fileDescriptorEntry *newEntry = descriptorTable[entry];

      if(newEntry == NULL)
      {
        fprintf(stderr, "Error creating new file\n");
        return -1;
      }
      int inode = -1;
      int k;
      for(k = 1; k < MAX_FILES+1; k++)
      {
        if(inodeTable[k].mode == 0)
        {
          inodeTable[k].mode == 1;
          inode = k;
          break;
        }
      }
      if(inode == -1)
      {
        fprintf(stderr, "Error, i-node Table full\n");
        return -1;
      }
      //find next free location to create new file in
      int writeLoc = findFree();
      if(writeLoc == -1)
        return -1;

      setAlloc(writeLoc);

      newEntry->rwPointer = 0;
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
      //fprintf(stderr, "inode:%d\n", newEntry->inode);
      //fprintf(stderr, "filename:%s\n", rootDir[i].filename);
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
    fprintf(stderr, "File has already been closed\n");
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
    if(strncmp(rootDir[i].filename, file, MAXFILENAME + 1) == 0 && rootDir[i].inode != 5000)  //if we find the file remove data and set space to free
    {
      directoryEntry *removeEntry = &(rootDir[i]);  //update root directory
      int inode = removeEntry->inode;
      strcpy(removeEntry->filename, "\0");
      inodeEntry *inodeRemove = &(inodeTable[inode]);
      removeEntry->inode = 5000;
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
          setFree(inodeRemove->singleIndirectPtr);
          inodeRemove->singleIndirectPtr = 5000;
        }
        for(k = 0; k < 12; k++)
        {
          setFree(inodeRemove->directptr[k]);
          inodeRemove->directptr[k] = 5000;
        }

        inodeRemove->mode = 0;
        inodeRemove->linkCount = 0;
        inodeRemove->size = 0;
        write_blocks(INODE_TABLE, INODE_TABLE_SIZE, inodeTable); //update inode data on disk
        return 0;
    }
  }
  fprintf(stderr, "Not file of that name found\n"); //if no mactching file return -1
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
    fprintf(stderr, "Error seeking to requested location in requested file\n");
    return -1;
  }

  //shift read and write pointers to offset return 0 for success
  descriptorTable[fileID]->rwPointer = offset;
  return 0;
}

int sfs_fwrite(int fileID, const char *buf, int length)
{
  //if invalid file or requested return -1
  if(fileID >= numFiles || descriptorTable[fileID] == NULL || buf == NULL || length < 0)
  {
    fprintf(stderr, "Error in write request\n");
    return -1; //return -1 on failure
  }
  int writeLength = length;

  fileDescriptorEntry *writeFile = descriptorTable[fileID];
  inodeEntry *inode = &(inodeTable[writeFile->inode]);
  //fprintf(stderr, "fileID %d\n numfiles %d\n length %d\n", fileID, numFiles, length);
  //fprintf(stderr, "inode:%d\n", writeFile->inode);
  //fprintf(stderr, "inode size:%d\n", inode->size);

  if(writeFile->inode == 5000)
  {
    fprintf(stderr, "Error, bad inode link");
    return -1;
  }
  char *diskBuffer = malloc(BLOCKSIZE);

  int block = (writeFile->rwPointer)/BLOCKSIZE;  //get block location to write to
  int bytes = (writeFile->rwPointer)%BLOCKSIZE;   //get exact byte location to write to
  int eofBlock = (inode->size)/BLOCKSIZE;
  //int i;
  unsigned int writeLoc;
  int offset = 0;
  if(block > 139)
  {
    fprintf(stderr, "Error, write exceeded max file size\n");
    return -1;
  }else if(block > 11)     //get location of block in located in single indirect pointers
  {
    unsigned int *indirectBuff = malloc(BLOCKSIZE);
    read_blocks(inode->singleIndirectPtr, 1, indirectBuff);
    writeLoc = indirectBuff[block - 12];
    free(indirectBuff);
  }else             //if not get location of block in direct pointers
    writeLoc = inode->directptr[block];

  if(writeLoc == -1)
  {
    fprintf(stderr, "Error, -1 writeLoc");
    return -1;
  }
  while(length > 0)
  {
    read_blocks(START + writeLoc, 1, diskBuffer);
    int byteWrite;
    if(BLOCKSIZE - bytes < length)
    {
      byteWrite = BLOCKSIZE - bytes;
    }else
      byteWrite = length;

    memcpy(&diskBuffer[bytes], &buf[offset], byteWrite);
    write_blocks(START + writeLoc, 1, diskBuffer);

    length -= (byteWrite);
    offset += (byteWrite);
    bytes = 0;
    block++;
    if(length > 0)  //if there is data still to write update writeloc and allocate memory
    {
      if(block > 139)
      {
        fprintf(stderr, "Error, write exceeded max file size\n");
        return -1;
      }else if(eofBlock < block)
      {
        if(block == 12 && inode->singleIndirectPtr == 5000)
        {
          int indirPtr = findFree();
          setAlloc(indirPtr);
          inode->singleIndirectPtr = indirPtr;
        }
        int next = findFree();  //find next write location
        setAlloc(next);
        if(next == -1)
          return -1;
        writeLoc = next;
        if(block > 11)         //update i-node associated with file
        {
          unsigned int *nextBuff = malloc(BLOCKSIZE);
          read_blocks(inode->singleIndirectPtr, 1, nextBuff);
          nextBuff[block - 12] = writeLoc;
          write_blocks(inode->singleIndirectPtr, 1, nextBuff);
          free(nextBuff);
        }else
          inode->directptr[block] = writeLoc;

        inode->linkCount++; //update link count
      }else
      {
        if(block > 11)          //update i-node associated with file
        {
          unsigned int *nextBuff = malloc(BLOCKSIZE);
          read_blocks(inode->singleIndirectPtr, 1, nextBuff);
          writeLoc = nextBuff[block - 12];
          free(nextBuff);
        }else
          writeLoc = inode->directptr[block];
      }

    }
  }
  //update size of file in inode entry
  if(writeFile->rwPointer + writeLength > inode->size)
  {
    inode->size = writeFile->rwPointer + writeLength;
  }
  //update writer pointer in file descriptor entry
  writeFile->rwPointer += writeLength;
  //upodate inode table on disk
  write_blocks(INODE_TABLE, INODE_TABLE_SIZE, inodeTable);
  free(diskBuffer);
  return writeLength; //return length of written data
}

int sfs_fread(int fileID, char *buf, int length) //returns -1 for failure
{
  if(descriptorTable[fileID] == NULL || length < 0 || fileID >= numFiles || buf == NULL)
  {
    fprintf(stderr, "fileID %d\n numfiles %d\n length %d\n", fileID,numFiles, length);
    fprintf(stderr, "Error in read request\n");
    return -1;
  }


  fileDescriptorEntry *readFile = descriptorTable[fileID];
  inodeEntry *inode = &(inodeTable[readFile->inode]);
  //fprintf(stderr, "fileID %d\n numfiles %d\n length %d\n", fileID, numFiles, length);
  //fprintf(stderr, "inode:%d\n", readFile->inode);
  //fprintf(stderr, "inode size:%d\n", inode->size);

  if(readFile->inode == 5000)
  {
    fprintf(stderr, "Error, bad inode request");
    return -1;
  }

  if(readFile->rwPointer + length > inode->size)
  {
    length = inode->size - readFile->rwPointer;
  }
  int readLength = length;
  char *diskBuffer = malloc(BLOCKSIZE);

  int block = (readFile->rwPointer)/BLOCKSIZE;  //get block location to read from
  int bytes = (readFile->rwPointer)%BLOCKSIZE;   //get exact byte location to read from
  int eofBlock = (inode->size)/BLOCKSIZE;
  unsigned int readLoc;
  int offset = 0;
  if(block > 139)
  {
    fprintf(stderr, "Error, read exceeded max file size\n");
    return -1;
  }else if(block > 11)     //get location of block in located in single indirect pointers
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

    memcpy(&buf[offset], &diskBuffer[bytes], bytesRead);

    length -= (bytesRead);
    offset += (bytesRead);
    bytes = 0;

    if(length > 0) //check to see if there is more to read;
    {
      block++;
      if(eofBlock < block) //if trying to read past file return -1;
        return -1;

      if(block > 139)
      {
        fprintf(stderr, "Error, read exceeded max file size\n");
        return -1;
      }else if(block > 11)
      {
        unsigned int *nextBuff = malloc(BLOCKSIZE);
        read_blocks(inode->singleIndirectPtr, 1, nextBuff);
        readLoc = nextBuff[block - 12];
        free(nextBuff);
      }else
        readLoc = inode->directptr[block];


    }
  }

  free(diskBuffer);
  //update r/w pointer in descriptor table
  readFile->rwPointer += readLength;
  return readLength;
}

int createFreeList()
{
  unsigned int *buff = malloc(BLOCKSIZE);
  if (buff == NULL)
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
  if(index > NUM_BLOCKS)
  {
    fprintf(stderr, "Error, bad allocation attempt");
    return;
  }
  int byte = index / (8*sizeof(unsigned int));
  int bit = index % (8*sizeof(unsigned int));

  unsigned int *buff = malloc(BLOCKSIZE);

  if(buff == NULL)
  {
    fprintf(stderr, "Error assigning free bit\n");
    return;
  }

  read_blocks(FREELIST, FREELIST_SIZE, buff);
  buff[byte] |= 1 << bit; //sets bit
  write_blocks(FREELIST, FREELIST_SIZE, buff);
  free(buff);
}

void setAlloc(unsigned int index) //set index to allocated in FREELIST
{

  int byte = index / (8*sizeof(unsigned int));  //find byte to change
  int bit = index % (8*sizeof(unsigned int));   //find bit to change

  unsigned int *buff = malloc(BLOCKSIZE);

  if(buff == NULL)
  {
    fprintf(stderr, "Error assigning allocated bit\n");
    return;
  }
  if(index > BLOCKSIZE)
  {
    fprintf(stderr, "Error, bad allocation attempt");
    return;
  }
  read_blocks(FREELIST, FREELIST_SIZE, buff);
  buff[byte] &= ~(1 << bit);
  write_blocks(FREELIST, FREELIST_SIZE, buff);
  free(buff);
}

int findFree()
{
  unsigned int *buff = malloc(BLOCKSIZE);

  if(buff == NULL)
  {
    fprintf(stderr, "Error finding free bit\n");
    return -1;
  }

  read_blocks(FREELIST, FREELIST_SIZE, buff);

  int i;
  for(i = 0; i < (BLOCKSIZE)/sizeof(unsigned int); i++)
  {
    int find = ffs(buff[i]);
    if(find)
    {
      return find + i*8*sizeof(unsigned int) - 1;
    }
  }

  return -1;
}

int createSuperblock()
{
  unsigned int *buff = malloc(BLOCKSIZE);

  if(buff == NULL)
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

  if(buff == NULL)
  {
    return -1;
  }

  int i;
  for(i = 0; i < MAX_FILES; i++)
  {
    buff[i] = (directoryEntry){.filename = "\0",.inode = 5000};
  }

  write_blocks(DIRECTORY_LOCATION, DIRECTORY_SIZE, buff);
  free(buff);
  return 0;
}

int createInodeTable()
{
  inodeEntry *buff = malloc(ALIGN((MAX_FILES+1)*sizeof(inodeEntry)));

  if(buff == NULL)
  {
    return -1;
  }

  int i;
  for(i = 0; i < MAX_FILES+1; i++)
  {
    buff[i].mode = 0;                                 //mode
    buff[i].linkCount = 0;                            //link count
    buff[i].size = 0;                                 //size
    buff[i].singleIndirectPtr = 5000;       //indirect pointers
    //setAlloc(INODE_TABLE + i);

    int j;
    for(j = 0; j < 12; j++)
    {
      buff[i].directptr[j] = 5000;
    }
  }

  write_blocks(INODE_TABLE, INODE_TABLE_SIZE, buff);
  free(buff);
  return 0;
}
