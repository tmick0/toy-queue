#ifndef basic_queue_h_
#define basic_queue_h_

#include "common/queue.h"

#include <cstring>
#include <cstdint>

#include <pthread.h>

class basic_queue : public queue {
public:

    basic_queue(size_t size);
    ~basic_queue();
    
    void   put(uint8_t *data, size_t size) override;
    size_t get(uint8_t *data, size_t max) override;
    
private:

    void do_read(uint8_t *data, size_t offset, size_t size);
    void do_write(uint8_t *data, size_t offset, size_t size);

    size_t   m_size;
    
    volatile size_t   m_head;
    volatile size_t   m_tail;
    
    volatile size_t   m_head_seq;
    volatile size_t   m_tail_seq;
    
    volatile size_t   m_avail;
    volatile uint8_t *m_buffer;
    
    pthread_cond_t  m_read;
    pthread_cond_t  m_write;
    pthread_mutex_t m_lock;
    
};

#endif
