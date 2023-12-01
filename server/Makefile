EXE = aesdsocket
SOURCES = $(wildcard *.c)
HEADERS = $(wildcard *.h)
OBJECTS = $(SOURCES:%.c=%.o)

CC = $(CROSS_COMPILE)gcc
CFLAGS = -g -Wall -Wpedantic -Werror
LDLIBS = -lpthread -lrt

all: $(EXE)

$(EXE): $(OBJECTS)

$(EXE).o: $(HEADERS)

%.o: %.c %.h

$(HEADERS):

.PHONY: clean
clean:
	rm -f $(EXE) $(OBJECTS)

.PHONY: install
install:
	cp -f aesdsocket /usr/bin/aesdsocket
	cp -f aesdsocket-start-stop.sh /etc/init.d/S99aesdsocket
