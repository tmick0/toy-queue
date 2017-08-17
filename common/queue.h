#ifndef queue_h_
#define queue_h_

#include <cstdint>
#include <cstring>

class queue {
public:
    virtual ~queue() = default;
    virtual void   put(uint8_t *data, size_t size) = 0;
    virtual size_t get(uint8_t *data, size_t max)  = 0;
};

#endif
