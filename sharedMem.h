#include <stdio.h>
#include <time.h>

#define MAX_SIZE 1024

typedef struct sharedMem {
  char buffer[MAX_SIZE];
  time_t time;
} sharedMem_t;
