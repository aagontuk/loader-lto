#include <unistd.h>
#include <fcntl.h>
#include <elf.h>
#include <stdio.h>
#include <stdlib.h>
#include "loader.h"

int main(int argc, char *argv[]){
    int fd; 
    int nbytes;
    Elf32_Ehdr ehdr;
    Elf32_Phdr seg_text_phdr;
    Elf32_Shdr *sec_text_shdr = malloc(sizeof(Elf32_Shdr));
    unsigned char *text_segment;

    if(argc == 2){
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

            func_t new_funcs[] = {{"__x86.get_pc_thunk.bx", 0},
                {"deregister_tm_clones", 0}, {"register_tm_clones", 0},
                {"__do_global_dtors_aux", 0}, {"frame_dummy", 0},
                {"__x86.get_pc_thunk.dx", 0}, {"main", 0}, {"t_func_2", 0},
                {"t_func_1", 0}, {"__x86.get_pc_thunk.ax", 0}};

            reorder_seg_text(fd, text_segment, &seg_text_phdr,
                    sec_text_shdr, funcs, &funcs_len, new_funcs);
            
            update_sym_tab(fd, &ehdr, new_funcs, funcs_len);

        }
        else{
            printf("can't open file: %s\n", argv[1]);
        }
    }
    else{
        printf("usage: loader ELF_FILE\n");
    }
}
