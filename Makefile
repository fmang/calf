CFLAGS=-Wall
LDFLAGS=-lfcgi -lsystemd-daemon

all: calf

calf: calf.c

clean:
	rm -f calf
