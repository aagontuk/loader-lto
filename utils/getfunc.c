/* 
 * This program fetches all the functions from the text
 * section. And writes them to a file
 */

#include <stdlib.h>
#include <string.h>
#include "../freorder.h"
#include "../loader.h"

int main(int argc, char *argv[]){
    if(argc != 2){
        printf("usage: getfunc ELF_FILE");
        exit(EXIT_FAILURE);
    }

    char *name = malloc(strlen(argv[1]) + 4);
    strcpy(name, argv[1]);
    strcat(name, ".txt");
    
    char *elf_file = NULL;
    size_t fsize;
    func_t *functions;
    int func_len;

    elf_file = load_file(argv[1], &fsize);

    functions = get_func_list(elf_file, &func_len);
    sort_func_list(functions, func_len);
    write_funcs_to_file(functions, func_len, name);

    free(functions);
    free(name);

    return 0;
}
