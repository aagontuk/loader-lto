#include <stdio.h>

extern void hello(char *);
extern int sum(int, int);

int main(int argc, char *argv[]){
    hello("hello, world"); 
    printf("%d\n", sum(10, 20));
    return 0;
}
