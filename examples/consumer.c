#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

#include "../ringbuffer.h"

int main(void)
{
  int shm_fd = shm_open("/shared_test", O_RDWR, 0666);
  if (shm_fd == -1) {
    perror("shm_open");
    exit(1);
  }

  uint8_t* shm_ptr = mmap(0, 1024, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  if (shm_ptr == MAP_FAILED) {
    perror("mmap");
    exit(1);
  }

  ring_buffer_t* ring_buffer = (ring_buffer_t*)shm_ptr;

  printf("ring buffer ptr: %p\n", ring_buffer);
  printf("Buffer ptr: %p\n", ring_buffer->buffer);

  /* dequeue all elements */
  char data; 
  //for (int cnt = 0; ring_buffer_dequeue(ring_buffer, &data) > 0; cnt++) {
  //  assert(data == cnt);
  //  printf("Read: %d\n", data);
  //}

  uint8_t tmp = 0;
  while (1) {
    ring_buffer_dequeue(ring_buffer, &data);
    if (data != tmp) {
      printf("Read: %d\n", data);
      tmp = data;
    }

  }

  printf("Consumer done\n");


  munmap(shm_ptr, 1024);
  close(shm_fd);

  return 0;
}