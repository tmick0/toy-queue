#ifndef queue_h_
#define queue_h_

#include <stdint.h>
#include <pthread.h>

/** Blocking queue data structure
 */
typedef struct __queue_t__ {
    // backing buffer and size
    uint8_t *buffer;
    size_t size;
    
    // read / write indices
    size_t head;
    size_t tail;
    
    // sequence number of next consumable message
    size_t head_seq;
    
    // sequence number of last written message
    size_t tail_seq;
    
    // number of bytes readable
    size_t avail;
    
    // synchronization primitives
    pthread_cond_t readable;
    pthread_cond_t writeable;
    pthread_mutex_t lock;
} queue_t;

/** Initialize a blocking queue *q*, backed by the given buffer *b* of size *s*
 */
void queue_init(queue_t *q, uint8_t *b, size_t s);

/** Destroy the blocking queue *q*
 */
void queue_destroy(queue_t *q);

/** Insert into queue *q* a message of *size* bytes from *buffer*
  *
  * Blocks until sufficient space is available in the queue.
  */
void queue_put(queue_t *q, uint8_t *buffer, size_t size);

/** Retrieves a message of at most *max* bytes from queue *q* and writes
  * it to *buffer*.
  *
  * Blocks until a message of no more than *max* bytes is available.
  *
  * Returns the number of bytes in the written message.
  */
size_t queue_get(queue_t *q, uint8_t *buffer, size_t max);

#endif
