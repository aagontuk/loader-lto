#include <stdio.h>
#include <stdlib.h>
#include "loader.h"

int main(int argc, char *argv[], char *envp[]){
    if(argc != 3){
        printf("usage: loader ELF_FILE SH_LIB_FILE\n");
        exit(EXIT_FAILURE);
    }

    size_t fsize_elf;
    size_t fsize_lib;
    lib_func_t funcs[] = {{"hello", 0},
                          {"sum", 0}};
    
    char *elf_file = load_file(argv[1], &fsize_elf);
    char *sh_lib_file = load_file(argv[2], &fsize_lib);

    /* load library and get function addresses */
    load_sh_lib_image(sh_lib_file, fsize_lib, funcs, 2);

    printf("%s: %x\n", funcs[0].name, funcs[0].addr);
    printf("%s: %x\n", funcs[1].name, funcs[1].addr);

    /* resolve ELF GOT */
    resolve_elf_got(elf_file, fsize_elf, funcs, 2);

    /* run elf */
    int (*entry)(int, char **, char **);
    entry = load_bin_image(elf_file, fsize_elf);

    return entry(argc, argv, envp);
}
