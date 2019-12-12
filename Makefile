C_SOURCES = $(wildcard *.c)
C_HEADERS = $(wildcard *.h)

OBJ = ${C_SOURCES:.c=.o}

CFLAGS = -Wall -g

all: loader

loader: ${OBJ}
	gcc $(CFLAGS) -o $@ $^ 

%.o: %.c ${C_HEADERS}
	gcc $(CFLAGS) -c $< -o $@

check:
	cp libtest32.so.bak libtest32.so
	./loader libtest32.so

clean:
	rm -rf *.o loader
