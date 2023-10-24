CC = $(CROSS_COMPILE)gcc
CFLAGS = -g

# the following two rules are implicit in GNU make
# make them explicit for learning purposes
writer: writer.o
	$(CC) $(LDFLAGS) $^ $(LOADLIBS) $(LDLIBS) -o $@

%.o: %.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $^ -o $@

clean:
	rm -f writer *.o
