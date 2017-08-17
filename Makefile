CXXFLAGS=-std=gnu++14 -Wall -Wpedantic -Werror -Wno-error=vla -D_GNU_SOURCE -I. -g -Ofast
LDFLAGS=-lpthread

all: build/benchmark build/check

build/benchmark: build/vm_queue.o build/basic_queue.o benchmark/benchmark.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

build/check: build/vm_queue.o build/basic_queue.o check/check.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

build/vm_queue.o: vm_queue/vm_queue.cpp | vm_queue/vm_queue.h vm_queue/memfd.h common/header.h common/queue.h
	$(CXX) $(CXXFLAGS) -o $@ -c $^

build/basic_queue.o: basic_queue/basic_queue.cpp | basic_queue/basic_queue.h common/header.h common/queue.h
	$(CXX) $(CXXFLAGS) -o $@ -c $^

clean:
	rm -f build/*
