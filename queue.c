#include <stdio.h>
#include <stdlib.h>

#include "queue_internal.h"

void queue_init(queue_t *q, uint8_t *b, size_t s) {
    if(s <= sizeof(message_t)) {
        fprintf(stderr, "queue error: Tried to initialize queue backed by buffer of %lu bytes, but %lu a minimum of bytes are needed\n", s, sizeof(message_t));
        abort();
    }
    
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->readable, NULL);
    pthread_cond_init(&q->writeable, NULL);
    
    q->buffer = b;
    q->size = s;
    
    q->head = 0;
    q->tail = 0;
    q->head_seq = 0;
    q->tail_seq = 0;
    q->avail = 0;
}

void queue_destroy(queue_t *q) {
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->readable);
    pthread_cond_destroy(&q->writeable);
}

void queue_put(queue_t *q, uint8_t *buffer, size_t size) {
    pthread_mutex_lock(&q->lock);
    
    // wait for space to become available
    while(q->size - q->avail < size + sizeof(message_t)) {
        pthread_cond_wait(&q->writeable, &q->lock);
    }
    
    // construct header
    message_t m;
    m.len = size;
    m.seq = q->tail_seq++;
    
    // write header
    size_t i;
    for(i = 0; i < sizeof(message_t); i++) {
        q->buffer[q->tail] = ((uint8_t *) &m)[i];
        q->tail = (q->tail + 1) % q->size;
    }
    
    // write body
    for(i = 0; i < size; i++){
        q->buffer[q->tail] = buffer[i];
        q->tail = (q->tail + 1) % q->size;
    }
    
    q->avail += size + sizeof(message_t);
    
    pthread_cond_signal(&q->readable);
    pthread_mutex_unlock(&q->lock);
}

size_t queue_get(queue_t *q, uint8_t *buffer, size_t max) {
    pthread_mutex_lock(&q->lock);
    
    // wait for a message that we can successfully consume to reach the front of the queue
    message_t m;
    for(;;) {
    
        // wait for a message to arrive
        while(q->avail == 0){
            pthread_cond_wait(&q->readable, &q->lock);
        }
        
        // read message header
        size_t i;
        for(i = 0; i < sizeof(message_t); i++){
            ((uint8_t *) &m)[i] = q->buffer[(q->head + i) % q->size];
        }
        
        // message too long, wait for someone else to consume it
        if(m.len > max){
            while(q->head_seq == m.seq) {
                pthread_cond_wait(&q->writeable, &q->lock);
            }
            continue;
        }
        
        // we successfully consumed the header, increment the head pointer and proceed
        q->head = (q->head + i) % q->size;
        break;
    } 
    
    // read message body
    size_t i;
    for(i = 0; i < m.len; i++) {
        buffer[i] = q->buffer[q->head];
        q->head = (q->head + 1) % q->size;
    }
    
    q->avail -= m.len + sizeof(message_t);
    q->head_seq++;
    
    pthread_cond_signal(&q->writeable);
    pthread_mutex_unlock(&q->lock);
    
    return i;
}
