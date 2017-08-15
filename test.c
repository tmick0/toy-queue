#include <stdio.h>

#include "queue.h"

#define BUFFER_SIZE (4096)
#define NUM_THREADS (24)
#define MESSAGES_PER_THREAD (100)

void *consumer_loop(void *arg) {
    queue_t *q = (queue_t *) arg;
    size_t count = 0;
    size_t i;
    for(i = 0; i < MESSAGES_PER_THREAD; i++){
        size_t x;
        queue_get(q, (uint8_t *) &x, sizeof(size_t));
        count++;
    }
    return (void *) count;
}

void *publisher_loop(void *arg) {
    queue_t *q = (queue_t *) arg;
    size_t i;
    for(i = 0; i < NUM_THREADS * MESSAGES_PER_THREAD; i++){
        queue_put(q, (uint8_t *) &i, sizeof(size_t));
    }
    return (void *) i;
}

int main(int argc, char *argv[]){

    uint8_t buffer[BUFFER_SIZE];

    queue_t q;
    queue_init(&q, buffer, BUFFER_SIZE);

    pthread_t publisher;
    pthread_t consumers[NUM_THREADS];

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    
    pthread_create(&publisher, &attr, &publisher_loop, (void *) &q);
    
    intptr_t i;
    for(i = 0; i < NUM_THREADS; i++){
        pthread_create(&consumers[i], &attr, &consumer_loop, (void *) &q);
    }
    
    intptr_t sent;
    pthread_join(publisher, (void **) &sent);
    printf("publisher sent %ld messages\n", sent);
    
    intptr_t recd[NUM_THREADS];
    for(i = 0; i < NUM_THREADS; i++){
        pthread_join(consumers[i], (void **) &recd[i]);
        printf("consumer %ld received %ld messages\n", i, recd[i]);
    }
    
    pthread_attr_destroy(&attr);
    
    queue_destroy(&q);
    
    return 0;
}
