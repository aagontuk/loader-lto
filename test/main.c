extern int hello(int);
extern int sum(int, int);
extern void other(void);

int main(int argc, char *argv[]){
    other();
    return hello(10) + sum(20, 30);
}
