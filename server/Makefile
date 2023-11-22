CC = $(CROSS_COMPILE)gcc
CFLAGS = -g -Wall -Wpedantic -Werror

aesdsocket: aesdsocket.c
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LOADLIBS) $(LDLIBS) -o $@

all: aesdsocket

.PHONY: clean
clean:
	rm -f aesdsocket *.o

.PHONY: install
install:
	cp -f aesdsocket /usr/bin/aesdsocket
	cp -f aesdsocket-start-stop /etc/init.d/S99aesdsocket
