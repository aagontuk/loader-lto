CSOURCES = getfunc.c ../freorder.c
CHEADERS = ../freorder.h

OBJ = ${CSOURCES:.c=.o}

CFLAGS = -Wall -g

all: getfunc

getfunc: ${OBJ}
	gcc $(CFLAGS) -o $@ $^

%.o: %.c ${CHEADERS}
	gcc $(CFLAGS) -c $< -o $@

clean:
	rm -rf ./getfunc ${OBJ}
