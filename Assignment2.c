#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>

static sem_t mutex;
static sem_t mutex_rw;
static int readCount = 0;
static int target = 0;

static void *reader(void * args)
{
  int loops = *((int *) args);

  for(int i = 0; i < loops; i++)
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

  for (int i = 0; i < loops; i++)
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

  for (int i = 0; i < 10; i++)
  {
    printf("creating writer thread\n");
    s = pthread_create(&writers[i], NULL, &writer, &loops);
    if(s !=0)
    {
      printf("Error creating writer exiting...\n");
      exit(1);
    }

  }

  for (int i = 0; i < 100; i++)
  {
    printf("creating reader thread\n");
    s = pthread_create(&readers[i], NULL, &reader, &loops);
    if(s !=0)
    {
      printf("Error creating reader exiting...\n");
      exit(1);
    }

  }

  for (int i = 0; i < 10; i++)
  {
    s = pthread_join(writers[i], NULL);
    if (s != 0) {
      printf("Error, creating threads\n");
      exit(1);
    }

  }

  for (int i = 0; i < 100; i++)
  {
    s = pthread_join(readers[i], NULL);
    if (s != 0) {
      printf("Error, creating threads\n");
      exit(1);
    }

  }

}
