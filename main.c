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
    lib_func_t *lib_funcs;
    int func_len;

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
    
    // collect call graph for current order
    printf("\nCall graph:\n");
    call_graph_t *cgraph = get_call_graph(sh_lib_file);
    for(int i = 0; i < length; i++){
        if(cgraph[i].ins_val){
            printf("name: %s\tcalls: %s\toff: %x\tval: %x\n",
                    cgraph[i].fname,
                    cgraph[i].cfname,
                    cgraph[i].ins_off,
                    cgraph[i].ins_val);
        } 
    }
    printf("\n");

    // reorder library functions in new order
    functions = get_func_from_file(argv[3], &length);
    reorder_seg_text(sh_lib_file, functions);
    update_sym_tab(sh_lib_file, functions, length);
    printf("Resolve function:\n");
    resolve_func_calls(sh_lib_file, cgraph);
    free(functions);
    functions = NULL;
    free(cgraph);
    cgraph = NULL;
    
    // updated function order
    functions = get_func_list(sh_lib_file, &length);
    sort_func_list(functions, length);
    printf("\nUpdated function order:\n");
    print_func_list(functions, length);
    free(functions);
    functions = NULL;

    // write modified library to file
    write_file(sh_lib_file, fsize_lib, "test/libtest-re.so");

    // load library and get function addresses
    lib_funcs = load_sh_lib_image(sh_lib_file, fsize_lib, &func_len);

    printf("\nDynamic address of functions:\n");
    for(int i = 0; i < func_len; i++){
        printf("%s: %x\n", lib_funcs[i].name, lib_funcs[i].addr);
    }
    printf("\n");

    // resolve ELF GOT
    resolve_elf_got(elf_file, fsize_elf, lib_funcs, func_len);

    // run elf
    int (*entry)(int, char **, char **);
    entry = load_bin_image(elf_file, fsize_elf);
    printf("\nbin entry: %p\n", entry);

    free(lib_funcs);
    free(elf_file);
    free(sh_lib_file);

    return entry(argc, argv, envp);
}
