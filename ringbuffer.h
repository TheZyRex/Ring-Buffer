#include <inttypes.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>
#include "../inc/ZynqPerf.hpp"


/**
 * @file
 * Prototypes and structures for the ring buffer module.
 */

#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#ifdef __cplusplus
extern "C"
{
#endif

#define RING_BUFFER_ASSERT(x) assert(x)

/**
 * Checks if the buffer_size is a power of two.
 * Due to the design only <tt> RING_BUFFER_SIZE-1 </tt> items
 * can be contained in the buffer.
 * buffer_size must be a power of two.
*/
#define RING_BUFFER_IS_POWER_OF_TWO(buffer_size) ((buffer_size & (buffer_size - 1)) == 0)

/**
 * The type which is used to hold the size
 * and the indicies of the buffer.
 */
typedef size_t ring_buffer_size_t;

/**
 * Used as a modulo operator
 * as a % b = (a & (b − 1))
 * where 'a' is a positive index in the buffer and
 * 'b' is the (power of two) size of the buffer.
 */
#define RING_BUFFER_MASK(rb) (rb->ctrl->buffer_mask)

/**
 * Used as a modulo operator
 *
 */
#define WORD_MASK(rb) (RING_BUFFER_MASK(rb) >> 2)

/**
 * Computes the word-aligned index within a ring buffer for accessing 32-bit values.
 *
 * This macro is used to calculate the index for word-aligned access in a ring buffer
 * that uses byte-based indexing. It ensures efficient word access without performing
 * a costly division operation. The byte-based index is converted into a word-based
 * index by shifting right by 2 (equivalent to dividing by sizeof(uint32_t)).
 * The result is wrapped using the word-based buffer mask to ensure proper wrapping
 * around the buffer size, which must be a power of two.
 *
 * @param rb A pointer to the ring buffer structure.
 * @param index The current byte-based index (e.g., `head_index` or `tail_index`).
 * @return The word-aligned index within the ring buffer for 32-bit access.
 *
 * Note:
 * - The buffer mask for words is derived by dividing the byte-based buffer mask by 4,
 *   since 4 bytes make up one 32-bit word. This ensures proper wrapping for word-based access.
 * - The ring buffer size must be a power of two to enable efficient masking.
 * - This macro assumes `index` is a byte-based offset, such as `head_index` or `tail_index`.
 *
 * Example:
 *   size_t word_index = RING_BUFFER_WORD_INDEX(rb, rb->head_index);
 *   ((uint32_t*)rb->buffer)[word_index] = some_word_value;
 */
#define RING_BUFFER_WORD_INDEX(rb, index) (((index) >> 2) & ((rb->ctrl->buffer_mask) >> 2))

/**
 *
 */
#define RING_BUFFER_CTRL_SET_PATTERN(rb, pattern) \
    ((rb->ctrl->buffer_ctrl) &= 0xFF000000U, (rb->ctrl->buffer_ctrl) |= ((uint8_t)(pattern) << 24))

#define RING_BUFFER_CTRL_GET_PATTERN(rb) \
    ((uint8_t)(((rb->ctrl->buffer_ctrl) >> 24) & 0xFFU))

#define RING_BUFFER_CTRL_SET_STATUS(rb, value) \
    ((rb->ctrl->buffer_ctrl) = ((rb->ctrl->buffer_ctrl) & ~0xFF) | (value & 0xFF))

#define RING_BUFFER_CTRL_GET_STATUS(rb) \
    ((uint8_t)(rb->ctrl->buffer_ctrl) & 0xFFU)

#define RING_BUFFER_DEINIT_STATUS 	0x2
#define RING_BUFFER_INIT_STATUS		0x1


#define TRUE_MODULO(dividend, divisor) ({ \
    int64_t _dividend 	= (int64_t)(dividend);      \
    int64_t _divisor 	= (int64_t)(divisor);        \
    int64_t _result 	= _dividend % _divisor;   \
    if (_result < 0) {                    \
        _result += (_divisor > 0 ? _divisor : -_divisor); \
    }                                     \
    _result;                              \
})

/**
 * Simplifies the use of <tt>struct ring_buffer_t</tt>.
 */
typedef struct ring_buffer_t ring_buffer_t;

typedef struct {
    /**
     * buffer_ctrl upper 8bits are used as a initial pattern for writing data. So both
     * consumer and producer know about the pattern and the consumer can validate the read message
     *
     * Lower 8bits are used to control the initialization procedure
     */
    uint32_t buffer_ctrl;

    /** Buffer mask. */
    ring_buffer_size_t buffer_mask;

    /** Index of tail. */
    volatile ring_buffer_size_t tail_index;

    /** Index of head. */
    volatile ring_buffer_size_t head_index;
} ring_buffer_ctrl_t;

/**
 * Structure which holds a ring buffer.
 * The buffer contains a buffer array
 * as well as metadata for the ring buffer.
 */
struct ring_buffer_t {
    /* resides in ocm memory */
    ring_buffer_ctrl_t* ctrl;

    /** pointer to buffer location */
    char* buffer;
};

/**
 * Initializes the ring buffer pointed to by <em>buffer</em>.
 * This function can also be used to empty/reset the buffer.
 * The resulting buffer can contain <em>buf_size-1</em> bytes.
 * @param buffer The ring buffer to initialize.
 * @param buf_size The size of the allocated ringbuffer.
 */
void ring_buffer_init(ring_buffer_t *buffer, size_t buf_size, uint8_t init_vector);

/**
 * @brief ring_buffer_deinit
 * @param buffer
 */
void ring_buffer_deinit(ring_buffer_t* buffer);

/**
 * Adds a byte to a ring buffer.
 * @param buffer The buffer in which the data should be placed.
 * @param data The byte to place.
 */
void ring_buffer_queue(ring_buffer_t *buffer, char data);

/**
 * Adds an array of bytes to a ring buffer.
 * @param buffer The buffer in which the data should be placed.
 * @param data A pointer to the array of bytes to place in the queue.
 * @param size The size of the array.
 */
void ring_buffer_queue_arr(ring_buffer_t *buffer, const char *data, ring_buffer_size_t size);

/**
 * @brief ring_buffer_memcpy uses memcpy to write 'size' amount of bytes to a ring buffer
 * @param 	buffer The buffer in which the data should be placed.
 * @param 	data A pointer to the array of bytes to place in the queue.
 * @param 	size The size of the array.
 * @return	The number of bytes actually written.
 */
ring_buffer_size_t ring_buffer_memcpy(ring_buffer_t* buffer, const char* data, ring_buffer_size_t size);

/**
 * Returns the oldest byte in a ring buffer.
 * @param buffer The buffer from which the data should be returned.
 * @param data A pointer to the location at which the data should be placed.
 * @return 1 if data was returned; 0 otherwise.
 */
uint8_t ring_buffer_dequeue(ring_buffer_t *buffer, char *data);

/**
 * Returns the <em>len</em> oldest bytes in a ring buffer.
 * @param buffer The buffer from which the data should be returned.
 * @param data A pointer to the array at which the data should be placed.
 * @param len The maximum number of bytes to return.
 * @return The number of bytes returned.
 */
ring_buffer_size_t ring_buffer_dequeue_arr(ring_buffer_t *buffer, char *data, ring_buffer_size_t len);

/**
 * @brief ring_buffer_read_memcpy
 * @param rb
 * @param dst
 * @param len
 * @return
 */
ring_buffer_size_t ring_buffer_read_memcpy(ring_buffer_t* rb, char* dst, ring_buffer_size_t len);

/**
 * Reads 'len' bytes from the ring buffer, waiting if necessary until the requested number of bytes are available.
 * @param rb The ring buffer from which the data should be read.
 * @param len The number of bytes to read.
 * @return The number of bytes read.
 */
ring_buffer_size_t ring_buffer_read_memload_wait(ring_buffer_t* rb, ring_buffer_size_t len);

/**
 * Reads 'len' bytes from the ring buffer without waiting, returning immediately with the available bytes.
 * @param rb The ring buffer from which the data should be read.
 * @param len The number of bytes to read.
 * @return The number of bytes read.
 */
ring_buffer_size_t ring_buffer_read_memload_nowait(ring_buffer_t* rb, ring_buffer_size_t len);


/**
 * Peeks a ring buffer, i.e. returns an element without removing it.
 * @param buffer The buffer from which the data should be returned.
 * @param data A pointer to the location at which the data should be placed.
 * @param index The index to peek.
 * @return 1 if data was returned; 0 otherwise.
 */
uint8_t ring_buffer_peek(ring_buffer_t *buffer, char *data, ring_buffer_size_t index);


/**
 * Returns whether a ring buffer is empty.
 * @param buffer The buffer for which it should be returned whether it is empty.
 * @return 1 if empty; 0 otherwise.
 */
inline uint8_t ring_buffer_is_empty(ring_buffer_t *buffer) {
  return (buffer->ctrl->head_index == buffer->ctrl->tail_index);
}

/**
 * Returns whether a ring buffer is full.
 * @param buffer The buffer for which it should be returned whether it is full.
 * @return 1 if full; 0 otherwise.
 */
inline uint8_t ring_buffer_is_full(ring_buffer_t *buffer) {
  return ((buffer->ctrl->head_index - buffer->ctrl->tail_index) & RING_BUFFER_MASK(buffer)) == RING_BUFFER_MASK(buffer);
}

/**
 * Returns the number of items in a ring buffer.
 * @param buffer The buffer for which the number of items should be returned.
 * @return The number of items in the ring buffer.
 */
inline ring_buffer_size_t ring_buffer_num_items(ring_buffer_t *buffer) {
    return TRUE_MODULO(buffer->ctrl->head_index - buffer->ctrl->tail_index, RING_BUFFER_MASK(buffer));
}

#ifdef __cplusplus
}
#endif

#endif /* RINGBUFFER_H */
