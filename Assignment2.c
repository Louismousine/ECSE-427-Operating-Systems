#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

static sem_t mutex;
static sem_t mutex_rw;
static int readCount = 0;
static int target = 0;
static int currentReader = 0;
static int currentWriter = 0;

static float readerVal[100], writerVal[10];

float findMax(float array[], size_t size);
float findMin(float array[], size_t size);
float findAvg(float array[], size_t size);

static void *reader(void * args)
{
  int loops = *((int *) args);
  struct timeval tv;
  time_t dTime;
  dTime = 0;

  int i;

  for(i = 0; i < loops; i++)
  {
    int r = rand();
    gettimeofday(&tv, NULL);
    dTime = dTime + tv.tv_usec;
    if(sem_wait(&mutex) == -1)
      exit(2);
    gettimeofday(&tv, NULL);
    dTime = dTime + tv.tv_usec;
    readCount++;
    if(readCount == 1)
    {
      gettimeofday(&tv, NULL);
      dTime = dTime + tv.tv_usec;
      if(sem_wait(&mutex_rw)==-1)
      {
        exit(2);
      }
      gettimeofday(&tv, NULL);
      dTime = dTime + tv.tv_usec;
    }
    if(sem_post(&mutex) == -1)
      exit(2);

    printf("Current target value %d\n There are %d readers currently\n", target, readCount);

    gettimeofday(&tv, NULL);
    dTime = dTime + tv.tv_usec;
    if(sem_wait(&mutex) == -1)
      exit(2);
    gettimeofday(&tv, NULL);
    dTime = dTime + tv.tv_usec;

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
  readerVal[currentReader] = dTime;
  currentReader++;
}

static void *writer(void * args)
{
  int loops = *((int *) args);
  int temp;
  struct timeval tv;
  time_t dTime;
  dTime = 0;

  int r = rand();

  int i;
  for (i = 0; i < loops; i++)
  {
    gettimeofday(&tv, NULL);
    dTime = dTime + tv.tv_usec;
    if (sem_wait(&mutex_rw) == -1)
      exit(2);
    gettimeofday(&tv, NULL);
    dTime = dTime + tv.tv_usec;

    printf("writing to target\n");
    temp = target;
    temp = temp+10;
    target = temp;
    if(sem_post(&mutex_rw) == -1)
      exit(2);
    usleep((float)(r%100));
  }
  writerVal[currentWriter];
  currentWriter++;
}

int main(int argc, char *argv[])
{
  pthread_t readers[100],writers[10];
  int s;
  int loops = 100;

  float  readMax, readMin, readAvg,
      writeMax, writeMin, writeAvg;

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

  readMax = findMax(readerVal, 100);
  readMin = findMin(readerVal, 100);
  readAvg = findAvg(readerVal, 100);

  writeMax = findMax(writerVal, 10);
  writeMin = findMin(writerVal, 10);
  writeAvg = findAvg(writerVal, 10);

  printf("The maximum waiting time for the readers is: %d\n", readMax);
  printf("The minimum waiting time for the readers is: %d\n", readMin);
  printf("The average waiting time for the readers is: %d\n", readAvg);

  printf("The maximum waiting time for the writers is: %d\n", writeMax);
  printf("The minimum waiting time for the writers is: %d\n", writeMin);
  printf("The average waiting time for the writers is: %d\n", writeAvg);

}

float findMax(float array[], size_t size)
{
  float max = array[0];
  int i;
  for(i = 1; i < size; i++)
  {
    if(max < array[i])
      max = array[i];
  }

  return max;
}

float findMin(float array[], size_t size)
{
  float min = array[0];
  int i;
  for(i = 1; i < size; i++)
  {
    if(min > array[i])
      min = array[i];
  }

  return min;
}

float findAvg(float array[], size_t size)
{
  float avg = 0;
  float sum = 0;
  int i;
  for(i = 0; i < size; i++)
  {
    sum = sum + array[i];
  }
  avg = sum/((float) size);
  return avg;
}
