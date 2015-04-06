#ifndef _MALLOC_API_H_
#define _MALLOC_API_H_
void *my_malloc(int size);
void my_free(void *ptr);
void my_mallopt(int policy);
void my_mallinfo();
#endif
