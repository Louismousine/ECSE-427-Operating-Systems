#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define _BSD_SOURCE

static sem_t mutex;
static sem_t mutex_rw;
static int readCount = 0;
static int target = 0;
static int currentReader = 0;

static float readerVal[100], writerVal[10];

static void *reader(void * args)
{
  int loops = *((int *) args);
  float dTime, time1, time2;

  int i;
  for(i = 0; i < loops; i++)
  {
    int r = rand();
    if(sem_wait(&mutex) == -1)
      exit(2);
    readCount++;
    if(readCount == 1)
    {
      if(sem_wait(&mutex_rw)==-1)
      {
        exit(2);
      }
    }
    if(sem_post(&mutex) == -1)
      exit(2);

    printf("Current target value %d\n There are %d readers currently\n", target, readCount);


    if(sem_wait(&mutex) == -1)
      exit(2);
    readCount--;
    if(readCount == 0)
    {
      if(sem_post(&mutex_rw) == -1)
      {
        exit(2);
      }
    }
    if(sem_post(&mutex) == -1)
      exit(2);
    usleep((float)(r%100));
  }
}

static void *writer(void * args)
{
  int loops = *((int *) args);
  int temp;

  int r = rand();

  int i;
  for (i = 0; i < loops; i++)
  {
    if (sem_wait(&mutex_rw) == -1)
      exit(2);

    printf("writing to target\n");
    temp = target;
    temp = temp+10;
    target = temp;
    if(sem_post(&mutex_rw) == -1)
      exit(2);
    usleep((float)(r%100));
  }
}

int main(int argc, char *argv[])
{
  pthread_t readers[100],writers[10];
  int s;
  int loops = 100;


  srand(time(NULL));

  if(sem_init(&mutex,0,1) == -1)
  {
    printf("Error initiating semaphore exiting...\n");
    exit(1);
  }

  if(sem_init(&mutex_rw,0,1) == -1)
  {
    printf("Error initiating semaphore exiting...\n");
    exit(1);
  }

  int i;
  for (i = 0; i < 10; i++)
  {
    printf("creating writer thread\n");
    s = pthread_create(&writers[i], NULL, &writer, &loops);
    if(s !=0)
    {
      printf("Error creating writer exiting...\n");
      exit(1);
    }

  }

  int j;
  for (j = 0; j < 100; j++)
  {
    printf("creating reader thread\n");
    s = pthread_create(&readers[j], NULL, &reader, &loops);
    if(s !=0)
    {
      printf("Error creating reader exiting...\n");
      exit(1);
    }

  }


  int k;
  for (k = 0; k < 10; k++)
  {
    s = pthread_join(writers[k], NULL);
    if (s != 0) {
      printf("Error, creating threads\n");
      exit(1);
    }

  }

  int p;
  for (p = 0; p < 100; p++)
  {
    s = pthread_join(readers[p], NULL);
    if (s != 0) {
      printf("Error, creating threads\n");
      exit(1);
    }

  }

}
