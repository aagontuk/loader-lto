CFLAGS = -Wall -g -m32

all: loader

loader: main.c loader.c
	gcc $(CFLAGS) -o $@ $^

clean:
	rm -rf loader
