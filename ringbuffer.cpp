#include "../inc/ringbuffer.h"


/**
 * @file
 * Implementation of ring buffer functions.
 */

void ring_buffer_init(ring_buffer_t *buffer, size_t buf_size, uint8_t pattern) {
    RING_BUFFER_ASSERT(RING_BUFFER_IS_POWER_OF_TWO(buf_size) == 1);
    buffer->ctrl->buffer_mask = buf_size - 1;
    buffer->ctrl->tail_index = 0;
    buffer->ctrl->head_index = 0;
    RING_BUFFER_CTRL_SET_PATTERN(buffer, pattern);
    RING_BUFFER_CTRL_SET_STATUS(buffer, RING_BUFFER_INIT_STATUS);
}

void ring_buffer_deinit(ring_buffer_t* buffer) {
    memset(buffer->buffer, 0, buffer->ctrl->buffer_mask);
    buffer->ctrl->tail_index = 0;
    buffer->ctrl->head_index = 0;
    buffer->ctrl->buffer_mask = 0;
    RING_BUFFER_CTRL_SET_STATUS(buffer, RING_BUFFER_DEINIT_STATUS);
}

void ring_buffer_queue(ring_buffer_t *buffer, char data) {
  /* Is buffer full? */
  if(ring_buffer_is_full(buffer)) {
    /* Is going to overwrite the oldest byte */
    /* busy-wait for it to be read */
  }

  /* Place data in buffer */
  buffer->buffer[buffer->ctrl->head_index] = data;
  buffer->ctrl->head_index = ((buffer->ctrl->head_index + 1) & RING_BUFFER_MASK(buffer));
}

void ring_buffer_queue_arr(ring_buffer_t *buffer, const char *data, ring_buffer_size_t size) {
  /* Add bytes; one by one */
  ring_buffer_size_t i;
  for(i = 0; i < size; i++) {
    ring_buffer_queue(buffer, data[i]);
  }
}

ring_buffer_size_t ring_buffer_memcpy(ring_buffer_t* rb, const char* data, ring_buffer_size_t size) {
   if (size == 0) {
       return 0;
   }

   size_t available_space = rb->ctrl->buffer_mask - ring_buffer_num_items(rb);

   if (available_space < size) {
       return 0; // not enough space return 0
   }

   //while ((available_space = (rb->buffer_mask - ring_buffer_num_items(rb))) < size) {}
   size_t bytes_to_write = (size > available_space) ? available_space : size;

   size_t first_chunk = (rb->ctrl->buffer_mask) - rb->ctrl->head_index;
   if (bytes_to_write <= first_chunk) {
        memcpy(&rb->buffer[rb->ctrl->head_index], data, bytes_to_write);
        rb->ctrl->head_index = (rb->ctrl->head_index + bytes_to_write) & RING_BUFFER_MASK(rb);
   } else {
        memcpy(&rb->buffer[rb->ctrl->head_index], data, first_chunk);
        memcpy(rb->buffer, data + first_chunk, bytes_to_write - first_chunk);
        rb->ctrl->head_index = (rb->ctrl->head_index + bytes_to_write + 1) & RING_BUFFER_MASK(rb);
   }

   return bytes_to_write;
}


uint8_t ring_buffer_dequeue(ring_buffer_t *buffer, char *data) {
  if(ring_buffer_is_empty(buffer)) {
    /* No items */
    return 0;
  }
  
  *data = buffer->buffer[buffer->ctrl->tail_index];
  buffer->ctrl->tail_index = ((buffer->ctrl->tail_index + 1) & RING_BUFFER_MASK(buffer));
  return 1;
}

ring_buffer_size_t ring_buffer_dequeue_arr(ring_buffer_t *buffer, char *data, ring_buffer_size_t len) {
  if (ring_buffer_is_empty(buffer)) {
    /* No items */
      return 0;
  }

  char *data_ptr = data;
  ring_buffer_size_t cnt = 0;
  while((cnt < len) && ring_buffer_dequeue(buffer, data_ptr)) {
    cnt++;
    data_ptr++;
  }
  return cnt;
}

ring_buffer_size_t ring_buffer_read_memcpy(ring_buffer_t* rb, char* dst, ring_buffer_size_t len) {
   if (len == 0) {
       return 0;
   }

   if (ring_buffer_is_empty(rb)) {
       return 0;
   }

   size_t available_data = ring_buffer_num_items(rb);
   size_t bytes_to_read = (len > available_data) ? available_data : len;

   size_t first_chunk = (rb->ctrl->buffer_mask) - rb->ctrl->tail_index;
   if (bytes_to_read <= first_chunk) {
        memcpy(dst, &rb->buffer[rb->ctrl->tail_index], bytes_to_read);
        rb->ctrl->tail_index = (rb->ctrl->tail_index + bytes_to_read) & RING_BUFFER_MASK(rb);
   } else {
        memcpy(dst, &rb->buffer[rb->ctrl->tail_index], first_chunk);
        memcpy(dst + first_chunk, rb->buffer, bytes_to_read - first_chunk);
        rb->ctrl->tail_index = (rb->ctrl->tail_index + bytes_to_read + 1) & RING_BUFFER_MASK(rb);
   }

   return bytes_to_read;
}

ring_buffer_size_t ring_buffer_read_memload_wait(ring_buffer_t* rb, ring_buffer_size_t len) {
   if (len == 0) {
       return 0;
   }

   while (ring_buffer_num_items(rb) < len) {ZynqPerfHelpers::busy_wait_ns(100);}

   size_t available_data = ring_buffer_num_items(rb);
   size_t bytes_to_read = (len > available_data) ? available_data : len;

   size_t first_chunk = (rb->ctrl->buffer_mask) - rb->ctrl->tail_index;
   if (bytes_to_read <= first_chunk) {
        ZynqBench::memload64(&rb->buffer[rb->ctrl->tail_index], bytes_to_read);
        rb->ctrl->tail_index = (rb->ctrl->tail_index + bytes_to_read) & RING_BUFFER_MASK(rb);
   } else {
        ZynqBench::memload64(&rb->buffer[rb->ctrl->tail_index], first_chunk);
        ZynqBench::memload64(rb->buffer, bytes_to_read - first_chunk);
        rb->ctrl->tail_index = (rb->ctrl->tail_index + bytes_to_read + 1) & RING_BUFFER_MASK(rb);
   }

   return bytes_to_read;
}

ring_buffer_size_t ring_buffer_read_memload_nowait(ring_buffer_t* rb, ring_buffer_size_t len) {
   if (len == 0) {
       return 0;
   }

   if (ring_buffer_is_empty(rb)) {
       return 0;
   }

   size_t available_data = ring_buffer_num_items(rb);
   size_t bytes_to_read = (len > available_data) ? available_data : len;

   size_t first_chunk = (rb->ctrl->buffer_mask) - rb->ctrl->tail_index;
   if (bytes_to_read <= first_chunk) {
        ZynqBench::memload64(&rb->buffer[rb->ctrl->tail_index], bytes_to_read);
        rb->ctrl->tail_index = (rb->ctrl->tail_index + bytes_to_read) & RING_BUFFER_MASK(rb);
   } else {
        ZynqBench::memload64(&rb->buffer[rb->ctrl->tail_index], first_chunk);
        ZynqBench::memload64(rb->buffer, bytes_to_read - first_chunk);
        rb->ctrl->tail_index = (rb->ctrl->tail_index + bytes_to_read + 1) & RING_BUFFER_MASK(rb);
   }

   return bytes_to_read;
}


uint8_t ring_buffer_peek(ring_buffer_t *buffer, char *data, ring_buffer_size_t index) {
  if(index >= ring_buffer_num_items(buffer)) {
    /* No items at index */
    return 0;
  }
  
  /* Add index to pointer */
  ring_buffer_size_t data_index = ((buffer->ctrl->tail_index + index) & RING_BUFFER_MASK(buffer));
  *data = buffer->buffer[data_index];
  return 1;
}

extern inline uint8_t ring_buffer_is_empty(ring_buffer_t *buffer);
extern inline uint8_t ring_buffer_is_full(ring_buffer_t *buffer);
extern inline ring_buffer_size_t ring_buffer_num_items(ring_buffer_t *buffer);
