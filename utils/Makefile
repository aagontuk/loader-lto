CSOURCES = getfunc.c ../freorder.c ../loader.c
CHEADERS = ../freorder.h ../loader.h

OBJ = ${CSOURCES:.c=.o}

CFLAGS = -Wall -g

all: getfunc

getfunc: ${OBJ}
	gcc $(CFLAGS) -o $@ $^

%.o: %.c ${CHEADERS}
	gcc $(CFLAGS) -c $< -o $@

clean:
	rm -rf ./getfunc ${OBJ}
