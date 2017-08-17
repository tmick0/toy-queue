#include "basic_queue.h"
#include "common/header.h"

#include <algorithm>

basic_queue::basic_queue(size_t size)
: m_size(size)
, m_head(0)
, m_tail(0)
, m_head_seq(0)
, m_tail_seq(0)
, m_avail(0)
, m_buffer(new uint8_t[size])
{
    // Initialize synchronization primitives
    pthread_mutex_init(&m_lock, NULL);
    pthread_cond_init(&m_read, NULL);
    pthread_cond_init(&m_write, NULL);
}

basic_queue::~basic_queue() {
    delete[] m_buffer;
    pthread_mutex_destroy(&m_lock);
    pthread_cond_destroy(&m_read);
    pthread_cond_destroy(&m_write);
}

void basic_queue::do_read(uint8_t *data, size_t offset, size_t size){
    // adjust offset because it might already be rolled over
    offset %= m_size;

    // find out how much we can read at offset before we wrap
    const size_t part1 = std::min(size, m_size - offset);
    
    // find how much we need to read at the beginning of the buffer
    const size_t part2 = size - part1;
    
    // do the two-part copy
    memcpy(data,         (uint8_t *) m_buffer + offset, part1);
    memcpy(data + part1, (uint8_t *) m_buffer         , part2);
    
}

void basic_queue::do_write(uint8_t *data, size_t offset, size_t size){
    // adjust offset because it might already be rolled over
    offset %= m_size;

    // find out how much we can write at offset before we wrap
    const size_t part1 = std::min(size,  m_size - offset);
    
    // find how much we need to write at the beginning of the buffer
    const size_t part2 = size - part1;
    
    // do the two-part copy
    memcpy((uint8_t *) m_buffer + offset, data,         part1);
    memcpy((uint8_t *) m_buffer         , data + part1, part2);
}

void basic_queue::put(uint8_t *data, size_t size){
    pthread_mutex_lock(&m_lock);
    
    // Wait for space to become available
    while(m_size - m_avail < size + sizeof(header_t)) {
        pthread_cond_wait(&m_write, &m_lock);
    }
    
    // Construct header
    header_t m (size, m_tail_seq++);
    
    // Write message
    do_write((uint8_t *) &m, m_tail, sizeof(header_t));
    do_write(data, m_tail + sizeof(header_t), size);
    
    // Increment write index
    m_tail = (m_tail + size + sizeof(header_t)) % m_size;
    m_avail += size + sizeof(header_t);
    
    pthread_cond_signal(&m_read);
    pthread_mutex_unlock(&m_lock);
}

size_t basic_queue::get(uint8_t *data, size_t max){
    pthread_mutex_lock(&m_lock);
    
    // Wait for a message that we can successfully consume to reach the front of the queue
    header_t m;
    for(;;) {
    
        // Wait for a message to arrive
        while(m_avail < sizeof(header_t)){
            pthread_cond_wait(&m_read, &m_lock);
        }
        
        // Read message header
        do_read((uint8_t *) &m, m_head, sizeof(header_t));
        
        // Message too long, wait for someone else to consume it
        if(m.len > max){
            while(m_head_seq == m.seq) {
                pthread_cond_wait(&m_write, &m_lock);
            }
            continue;
        }
        
        // We successfully consumed the header of a suitable message, so proceed
        break;
    } 
    
    // Read message body
    do_read(data, m_head + sizeof(header_t), m.len);
    
    // Consume the message by incrementing the read pointer
    m_head = (m_head + m.len + sizeof(header_t)) % m_size;
    m_avail -= m.len + sizeof(header_t);
    m_head_seq++;
    
    pthread_cond_signal(&m_write);
    pthread_mutex_unlock(&m_lock);
    
    return m.len;
}

