#include "vm_queue/vm_queue.h"
#include "basic_queue/basic_queue.h"

#include <iostream>
#include <chrono>
#include <future>

bool consumer(queue *q, int num_runs, size_t message_size) {
    uint8_t message[message_size];
    bool good = true;
    for(int i = 0; i < num_runs; i++){
        q->get(message, message_size);
        for(size_t j = 0; j < message_size; j++){
            good &= (message[j] == (uint8_t) j);
        }
    }
    return good;
}

int main(int argc, char *argv[]) {
    
    if(argc != 5) {
        std::cerr << "usage: " << argv[0] << " queue_type num_runs buffer_size message_size" << std::endl;
        return 1;
    }
    
    const std::string impl (argv[1]);
    const int num_runs (atoi(argv[2]));
    const size_t buffer_size (atoi(argv[3]));
    const size_t message_size (atoi(argv[4]));
    
    uint8_t message[message_size];
    for(size_t i = 0; i < message_size; i++){
        message[i] = i;
    }
    
    queue *q;
    if(impl == "vm_queue"){
        q = new vm_queue(buffer_size);
    }
    else if(impl == "basic_queue"){
        q = new basic_queue(buffer_size);
    }
    else {
        std::cerr << "invalid queue type '" << impl << "'" << std::endl;
        return 1;
    }
    
    std::future<bool> consumer_future = std::async(std::launch::async, consumer, q, num_runs, message_size);
    
    for(int i = 0; i < num_runs; i++){
        q->put(message, message_size);
    }
    
    std::cout << "success: " << consumer_future.get() << std::endl;
    
    delete q;
    
    return 0;
}
