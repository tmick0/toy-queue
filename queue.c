#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include <linux/memfd.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <sys/types.h>

#include "queue_internal.h"

static inline int memfd_create(const char *name, unsigned int flags) {
    return syscall(__NR_memfd_create, name, flags);
}

void queue_init(queue_t *q, size_t s) {

    /* we're going to use a trick where we mmap two adjacent pages (in virtual memory) that point to the
     * same physical memory. this lets us optimize memory access, by virtue of the fact that we don't need
     * to even worry about wrapping our pointers around until we go through the entire buffer. too bad this
     * isn't portable, because it's so fun.
     */ 

    if(s % getpagesize() != 0) {
        fprintf(stderr, "queue error: Requested size (%lu) is not a multiple of the page size (%d)\n", s, getpagesize());
        abort();
    }
    
    // create an anonymous file backed by memory
    if((q->fd = memfd_create("queue_region", 0)) == -1){
        fprintf(stderr, "queue error: Could not obtain anonymous file (errno %d)\n", errno);
        abort();
    }
    
    // set buffer size
    if(ftruncate(q->fd, s) != 0){
        fprintf(stderr, "queue error: Could not set size of anonymous file (errno %d)\n", errno);
        abort();
    }
    
    // mmap first region
    if((q->buffer = mmap(NULL, s, PROT_READ | PROT_WRITE, MAP_SHARED, q->fd, 0)) == MAP_FAILED){
        fprintf(stderr, "queue error: Could not map buffer into virtual memory (errno %d)\n", errno);
        abort();
    }
    
    // mmap second region, with exact address
    if(mmap(q->buffer + s, s, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, q->fd, 0) == MAP_FAILED){
        fprintf(stderr, "queue error: Could not map buffer into virtual memory (errno %d)\n", errno);
        abort();
    }
    
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->readable, NULL);
    pthread_cond_init(&q->writeable, NULL);
    
    q->size = s;
    q->head = 0;
    q->tail = 0;
    q->head_seq = 0;
    q->tail_seq = 0;
}

void queue_destroy(queue_t *q) {
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->readable);
    pthread_cond_destroy(&q->writeable);
    munmap(q->buffer + q->size, q->size);
    munmap(q->buffer, q->size);
    close(q->fd);
}

void queue_put(queue_t *q, uint8_t *buffer, size_t size) {
    pthread_mutex_lock(&q->lock);
    
    // wait for space to become available
    while(q->size - (q->tail - q->head) < size + sizeof(message_t)) {
        pthread_cond_wait(&q->writeable, &q->lock);
    }
    
    // construct header
    message_t m;
    m.len = size;
    m.seq = q->tail_seq++;
    
    // write message
    memcpy(&q->buffer[q->tail                    ], &m,     sizeof(message_t));
    memcpy(&q->buffer[q->tail + sizeof(message_t)], buffer, size             );
    
    // increment write index
    q->tail  += size + sizeof(message_t);
    
    pthread_cond_signal(&q->readable);
    pthread_mutex_unlock(&q->lock);
}

void queue_peek_header(queue_t const *q, message_t *m) {
    memcpy(m, &q->buffer[q->head], sizeof(message_t));
}

void queue_peek_body(queue_t const *q, uint8_t *buffer, size_t len) {
    memcpy(buffer, &q->buffer[q->head + sizeof(message_t)], len);
}

size_t queue_get(queue_t *q, uint8_t *buffer, size_t max) {
    pthread_mutex_lock(&q->lock);
    
    // wait for a message that we can successfully consume to reach the front of the queue
    message_t m;
    for(;;) {
    
        // wait for a message to arrive
        while((q->tail - q->head) == 0){
            pthread_cond_wait(&q->readable, &q->lock);
        }
        
        // read message header
        queue_peek_header(q, &m);
        
        // message too long, wait for someone else to consume it
        if(m.len > max){
            while(q->head_seq == m.seq) {
                pthread_cond_wait(&q->writeable, &q->lock);
            }
            continue;
        }
        
        // we successfully consumed the header of a suitable message, so proceed
        break;
    } 
    
    // read message body
    queue_peek_body(q, buffer, m.len);
    
    // consume the message by incrementing the read pointer
    q->head += m.len + sizeof(message_t);
    
    // when read buffer moves into 2nd memory region, we can reset to the 1st region
    if(q->head >= q->size) {
        q->head -= q->size;
        q->tail -= q->size;
    }
    
    q->head_seq++;
    
    pthread_cond_signal(&q->writeable);
    pthread_mutex_unlock(&q->lock);
    
    return m.len;
}
