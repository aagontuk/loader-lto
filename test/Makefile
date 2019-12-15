CFLAGS = -m32 -Wall -g
LIBDIR = /home/sfinix/programming/repos/loader/test

all: libtest.so main

libtest.so: libtest.c
	gcc $(CFLAGS) -fPIC -shared $< -o $@

main: main.c
	gcc $(CFLAGS) -L$(LIBDIR) $< -o $@ -ltest
	
clean:
	rm -rf libtest.so main
