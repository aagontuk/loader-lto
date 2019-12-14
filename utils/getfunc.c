/* 
 * This program fetches all the functions from the text
 * section. And writes them to a file
 */

#include <unistd.h>
#include <fcntl.h>
#include <elf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../freorder.h"

int write_funcs_to_file(func_t *funcs, int len, char *fname);

int main(int argc, char *argv[]){
    if(argc != 2){
        printf("usage: getfunc ELF_FILE");
        exit(EXIT_FAILURE);
    }

    Elf32_Ehdr ehdr;
    func_t *funcs;
    int funcs_len;
    int nbytes;

    /* open elf file */
    int fd = open(argv[1], O_RDONLY);

    if(fd == -1){
        perror("getfunc");
        exit(EXIT_FAILURE);
    }

    /* read elf header */
    nbytes = read(fd, &ehdr, sizeof(Elf32_Ehdr));
    
    if(nbytes == -1){
        perror("getfunc");
        exit(EXIT_FAILURE);
   }

   funcs = get_func_list(fd, &ehdr, &funcs_len);
   sort_func_list(funcs, funcs_len);
   print_func_list(funcs, funcs_len);
   write_funcs_to_file(funcs, funcs_len, "funcs.txt");
}

int write_funcs_to_file(func_t *funcs, int len, char *fname){
    int fd = open(fname, O_CREAT | O_WRONLY, 0644);
    char *buf;
    int nbytes = 0;

    if(fd == -1){
        perror("getfunc");
        return -1;
    }

    for(int i = 0; i < len; i++){
        buf = funcs[i].name;
        nbytes += write(fd, buf, strlen(buf));
        nbytes += write(fd, "\n", 1);
    }

    return nbytes;
}
