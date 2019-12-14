#include <stdio.h>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE _GNU_SOURCE
#endif
#include <pthread.h>
int main(void) {
  pthread_t _thread;
  pthread_setname_np(_thread, "Hello");
  return 0;
}
