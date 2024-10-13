CC             = gcc
CFLAGS         = -Wall -g -std=c99 -D_XOPEN_SOURCE=500

all: producer consumer

producer: ../ringbuffer.c producer.c
	$(CC) $(CFLAGS) -o producer producer.c ../ringbuffer.c

consumer: ../ringbuffer.c consumer.c
	$(CC) $(CFLAGS) -o consumer consumer.c ../ringbuffer.c

clean:
	rm -f consumer producer
