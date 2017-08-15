#ifndef queue_internal_h_
#define queue_internal_h_

#include "queue.h"

/** Metadata (header) for a message in the queue
 */
typedef struct __message_t__ {
    size_t  len;
    size_t  seq;
} message_t;

/** Reads a header from the queue without consuming it. Undefined behavior if
  * the queue is empty. Lock must be held prior to calling.
  */
void queue_peek_header(queue_t const *q, message_t *m);

/** Reads a body from the queue without consuming it. Skips past the header.
  * Undefined behavior if the queue is empty. Lock must be held prior to
  * calling.
  */
void queue_peek_body(queue_t const *q, uint8_t *buffer, size_t len);

#endif
