CCFLAGS=-Wall -Wpedantic -Werror -std=c99 -fsanitize=address
LDFLAGS=-lpthread -lasan

test: test.c queue.o | queue.h
	$(CC) $(CCFLAGS) -o $@ $^ $(LDFLAGS)

queue.o: queue.c | queue.h queue_internal.h
	$(CC) $(CCFLAGS) -o $@ -c $^

clean:
	rm -f queue.o test
