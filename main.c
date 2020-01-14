#include <stdio.h>
#include <stdlib.h>
#include "loader.h"
#include "freorder.h"

int main(int argc, char *argv[], char *envp[]){
    if(argc != 4){
        printf("usage: loader ELF_FILE SH_LIB_FILE FUN_FILE\n");
        exit(EXIT_FAILURE);
    }

    size_t fsize_elf;
    size_t fsize_lib;
    lib_func_t funcs[] = {{"hello", 0},
                          {"sum", 0}};
    func_t *functions;
    int length;
    
    char *elf_file = load_file(argv[1], &fsize_elf);
    char *sh_lib_file = load_file(argv[2], &fsize_lib);

    // current function order
    functions = get_func_list(sh_lib_file, &length);
    sort_func_list(functions, length);
    printf("Current function order:\n");
    print_func_list(functions, length);
    free(functions);
    functions = NULL;

    // reorder library functions in new order
    functions = get_func_from_file(argv[3], &length);
    reorder_seg_text(sh_lib_file, functions);
    update_sym_tab(sh_lib_file, functions, length);
    free(functions);
    functions = NULL;
    
    // updated function order
    functions = get_func_list(sh_lib_file, &length);
    sort_func_list(functions, length);
    printf("\nUpdated function order:\n");
    print_func_list(functions, length);
    free(functions);
    functions = NULL;

    // write modified library to file
    write_file(sh_lib_file, fsize_lib, "test/libtest-re.so");

    /* load library and get function addresses */
    load_sh_lib_image(sh_lib_file, fsize_lib, funcs, 2);

    printf("\nDynamic address of functions:\n");
    printf("%s: %x\n", funcs[0].name, funcs[0].addr);
    printf("%s: %x\n", funcs[1].name, funcs[1].addr);
    printf("\n");

    /* resolve ELF GOT */
    resolve_elf_got(elf_file, fsize_elf, funcs, 2);

    /* run elf */
    int (*entry)(int, char **, char **);
    entry = load_bin_image(elf_file, fsize_elf);
    printf("\nbin entry: %p\n", entry);

    free(elf_file);
    free(sh_lib_file);

    return entry(argc, argv, envp);
}
