#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "../ringbuffer.h"

int main(void)
{
  int shm_fd = shm_open("/shared_test", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
  if (shm_fd == -1) {
    perror("shm_open");
    exit(1);
  }

  if (ftruncate(shm_fd, 1024) == -1) {
    perror("ftruncate");
    exit(1);
  }

  char* shm_ptr = mmap(0, 1024, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  if (shm_ptr == MAP_FAILED) {
    perror("mmap");
    exit(1);
  }

  ring_buffer_t* ring_buffer = (ring_buffer_t*)shm_ptr;

  printf("ring buffer ptr: %p\n", ring_buffer);
  printf("Buffer ptr: %p\n", ring_buffer->buffer);

  ring_buffer_init(ring_buffer, 32);

  for (int i = 0; i < 64; ++i) {
    ring_buffer_queue(ring_buffer, i);
  }

  /* peek thirf item */
  //char third; 
  //uint8_t cnt = ring_buffer_peek(ring_buffer, &third, 3);

  ///* Assert byte returned */
  //assert(cnt == 1);

  ///* Assert contents */
  //assert(third == 4);

  while (!ring_buffer_is_empty(ring_buffer)) { sleep(1); }

  printf("Ring buffer empty, exiting\n");
  printf("Producer done\n");

  munmap(shm_ptr, 1024);
  close(shm_fd);

  shm_unlink("/shared_test");
  return 0;
}