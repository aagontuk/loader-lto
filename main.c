#include <unistd.h>
#include <fcntl.h>
#include <elf.h>
#include <stdio.h>
#include <stdlib.h>
#include "freorder.h"

func_t *get_func_from_file(char *name, int *len);

int main(int argc, char *argv[]){
    int fd; 
    int nbytes;
    Elf32_Ehdr ehdr;
    Elf32_Phdr seg_text_phdr;
    Elf32_Shdr *sec_text_shdr = malloc(sizeof(Elf32_Shdr));
    unsigned char *text_segment;

    if(argc == 3){
        if((fd = open(argv[1], O_RDWR)) != -1){
            nbytes = read(fd, &ehdr, sizeof(Elf32_Ehdr));   

            /* Read segment header containing text section */
            seg_text_phdr = get_seg_text_phdr(fd, &ehdr);

            /* Read section header of text section */
            *sec_text_shdr = get_sec_from_name(fd, &ehdr, ".text");
            
            /* Read contents of the segment containing text section */
            text_segment = malloc(seg_text_phdr.p_memsz);
            lseek(fd, seg_text_phdr.p_offset, SEEK_SET);
            read(fd, text_segment, seg_text_phdr.p_memsz);

            int funcs_len;
            func_t *funcs;
            
            funcs = get_func_list(fd, &ehdr, &funcs_len);
            sort_func_list(funcs, funcs_len);

            int new_funcs_len;
            func_t *new_funcs = get_func_from_file(argv[2],
                    &new_funcs_len);

            if(funcs_len != new_funcs_len){
                printf("Number of functions in %s", argv[2]);
                printf("match with number of functions in");
                printf("text section");
            }
            
            reorder_seg_text(fd, text_segment, &seg_text_phdr,
                    sec_text_shdr, funcs, &funcs_len, new_funcs);
            
            update_sym_tab(fd, &ehdr, new_funcs, funcs_len);

        }
        else{
            printf("can't open file: %s\n", argv[1]);
        }
    }
    else{
        printf("usage: loader ELF_FILE FUNC_FILE\n");
    }
}

func_t *get_func_from_file(char *name, int *len){
    FILE *fp;
    int fsize;
    func_t *funcs = NULL;

    if((fp = fopen(name, "r")) == NULL){
        perror(name);
        exit(EXIT_FAILURE);
    }

    /* calc file size */
    fseek(fp, 0, SEEK_END);
    fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    /* empty file */
    if(!fsize){
        printf("%s is empty\n", name);
        exit(EXIT_FAILURE);
    }

    /* read file */
    char *buf = malloc(fsize + 1);
    fread(buf, 1, fsize, fp);
    fclose(fp);
    buf[fsize] = '\0';
    
    /* 
     * calc number of functions in file
     *
     * replace new lines with null to use the file content
     * as string table.
     */
    int nfunc = 0;
    int index[1024];
    int i = 0;
    int j = 0;

    index[0] = 0;
    while(*buf){
        if(*buf == '\n'){
            index[++j] = i + 1;
            *buf = '\0';
            nfunc++; 
        }
        buf++;
        i++;
    }

    buf -= fsize;

    /* allocate memory for funcs */
    funcs = malloc(sizeof(func_t) * nfunc);
    
    for(i = 0; i < nfunc; i++){
        funcs[i].name = buf + index[i];
        funcs[i].offset = 0;
    }

    *len = nfunc;

    return funcs;
}
