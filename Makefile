C_SOURCES = main.c loader.c freorder.c
C_HEADERS = loader.h freorder.h

OBJ = ${C_SOURCES:.c=.o}

CFLAGS = -Wall -g -m32

all: loader-lto

loader-lto: ${OBJ}
	gcc ${CFLAGS} -o $@ $^

%.o: %.c ${C_HEADERS}
	gcc ${CFLAGS} -c $< -o $@

clean:
	rm -rf *.o loader-lto
