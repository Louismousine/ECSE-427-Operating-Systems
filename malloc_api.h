#ifndef _MALLOC_API_H_
#define _MALLOC_API_H_

extern char *my_malloc_error;

void *my_malloc(int size);
void my_free(void *ptr);
void my_mallopt(int policy);
void my_mallinfo();
int my_free_space();
int my_bytes();
int my_largestSpace();

#endif
