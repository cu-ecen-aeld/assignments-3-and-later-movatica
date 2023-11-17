CC = $(CROSS_COMPILE)gcc
CFLAGS = -g

aesdsocket: aesdsocket.c
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LOADLIBS) $(LDLIBS) -o $@

all: aesdsocket

clean:
	rm -f aesdsocket *.o

.PHONY: clean
