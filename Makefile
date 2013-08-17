CFLAGS=-Wall
LDFLAGS=-lfcgi

all: calf

calf: calf.c

clean:
	rm -f calf
