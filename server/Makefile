EXE = aesdsocket
OBJECTS = aesdsocket.o aesdsocket_connectionhandler.o

CC = $(CROSS_COMPILE)gcc
CFLAGS = -g -Wall -Wpedantic -Werror
LDLIBS = -lpthread -lrt

all: $(EXE)

$(EXE): $(OBJECTS)

aesdsocket.o: aesdsocket.c aesdsocket.h aesdsocket_connectionhandler.h
aesdsocket_connectionhandler.o: aesdsocket_connectionhandler.c aesdsocket_connectionhandler.h

aesdsocket.h aesdsocket_connectionhandler.h:

.PHONY: clean
clean:
	rm -f $(EXE) $(OBJECTS)

.PHONY: install
install:
	cp -f aesdsocket /usr/bin/aesdsocket
	cp -f aesdsocket-start-stop.sh /etc/init.d/S99aesdsocket
