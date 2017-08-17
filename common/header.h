#ifndef message_h_
#define message_h_

#include <string.h>

struct header_t {

    header_t() = default;

    header_t(size_t len, size_t seq)
    : len(len)
    , seq(seq)
    {}

    size_t len;
    size_t seq;
    
};

#endif
