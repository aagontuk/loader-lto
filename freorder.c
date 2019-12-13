#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freorder.h"

Elf32_Phdr get_seg_text_phdr(int fd, Elf32_Ehdr *ehdr){
    int i;
    Elf32_Shdr shdr;
    Elf32_Phdr phdr;

    shdr = get_sec_from_name(fd, ehdr, ".text");
    Elf32_Off text_offset = shdr.sh_offset;

    /* Go to segment offset */
    lseek(fd, ehdr->e_phoff, SEEK_SET);

    /* Find segment containing .text section */
    for(i = 0; i < ehdr->e_phnum; i++){
        read(fd, &phdr, sizeof(Elf32_Phdr)); 
        if(phdr.p_type == PT_LOAD){
            Elf32_Off off_start = phdr.p_offset;
            Elf32_Off off_end = off_start + phdr.p_filesz;

            if(text_offset >= off_start && text_offset <= off_end){
                break; 
            }
        }
    }

    return phdr;
}

Elf32_Shdr get_sec_from_name(int fd, Elf32_Ehdr *ehdr, const char *name){
    int i;
    Elf32_Shdr shdr;
    char *shstr_content;
    
    /* Read shstr section header */
    lseek(fd, ehdr->e_shoff + sizeof(Elf32_Shdr) * ehdr->e_shstrndx, SEEK_SET);
    read(fd, &shdr, sizeof(Elf32_Shdr));

    /* Read shstr section contents */
    shstr_content = malloc(shdr.sh_size);
    lseek(fd, shdr.sh_offset, SEEK_SET);
    read(fd, shstr_content, shdr.sh_size);
   
    /* Go to section offset */ 
    lseek(fd, ehdr->e_shoff, SEEK_SET);

    /* Find out section */
    for(i = 0; i < ehdr->e_shnum; i++){
        read(fd, &shdr, sizeof(Elf32_Shdr));
        char *sec_name = shstr_content + shdr.sh_name;
        if(strcmp(sec_name, name) == 0){
            break;     
        }
    }

    return shdr;
}

int reorder_seg_text(int fd, unsigned char *seg_text,
                        Elf32_Phdr *seg_text_phdr,
                        Elf32_Shdr *sec_text_shdr,
                        func_t *cur_func_order,
                        int *len, func_t *new_func_order){


    int fun_start, fun_end, fun_len;
    int seg_base =  seg_text_phdr->p_offset;
    int bytes_writen = 0;
    unsigned char *new_seg_text = malloc(seg_text_phdr->p_filesz);

    /* copy data before text section into new segment */
    bytes_writen = sec_text_shdr->sh_offset - seg_base;
    memcpy(new_seg_text, seg_text, bytes_writen);

    /* end of text section */
    int text_end = sec_text_shdr->sh_offset + sec_text_shdr->sh_size; 

    for(int i = 0; i < *len; i++){
        int index = search_func_in_list(cur_func_order, *len, new_func_order[i].name); 
        fun_start = cur_func_order[index].offset;
        
        if(index == (*len) - 1){
            fun_end = text_end - 1;
        }
        else{
            fun_end = cur_func_order[index + 1].offset - 1;
        }

        fun_len = fun_end - fun_start + 1;
        
        /* copy function data into new segment */
        memcpy(new_seg_text + bytes_writen, seg_text +
                (fun_start - seg_base), fun_len);

        /* Update function offset */
        new_func_order[i].offset = seg_base + bytes_writen;
        
        bytes_writen += fun_len;
    }

    /* copy rest of the data into new segment */
    fun_len = seg_text_phdr->p_offset +
            seg_text_phdr->p_filesz - text_end;
    
    memcpy(new_seg_text + bytes_writen,
            seg_text + (text_end - seg_base), fun_len);
    bytes_writen += fun_len;

    lseek(fd, seg_text_phdr->p_offset, SEEK_SET);
    int nbytes = write(fd, new_seg_text, bytes_writen);

    return nbytes;
}

void update_sym_tab(int fd, Elf32_Ehdr *ehdr,
                        func_t *new_funcs, int len){
    
    Elf32_Shdr sym_shdr = get_sec_from_name(fd, ehdr, ".symtab");
    Elf32_Shdr strtab_shdr = get_sec_from_name(fd, ehdr, ".strtab");
    Elf32_Shdr text_shdr = get_sec_from_name(fd, ehdr, ".text");
    Elf32_Sym sym;
    char *strtab = malloc(strtab_shdr.sh_size);
    
    /* text section boundary */
    int text_start = text_shdr.sh_offset;
    int text_end = text_start + text_shdr.sh_size;

    /* Go to strtab offset */
    lseek(fd, strtab_shdr.sh_offset, SEEK_SET);

    /* read strtab */
    read(fd, strtab, strtab_shdr.sh_size);

    /* Go to sym table offset */
    lseek(fd, sym_shdr.sh_offset, SEEK_SET);

    /* Iterate through symbols */
    int sym_num = sym_shdr.sh_size / sizeof(Elf32_Sym);

    for(int i = 0; i < sym_num; i++){
        read(fd, &sym, sizeof(Elf32_Sym));
        
        /* Only functions that are in text section */
        if(ELF32_ST_TYPE(sym.st_info) == STT_FUNC &&
                sym.st_value >= text_start &&
                sym.st_value <= text_end){

            /* Change offset of the symbol */
            char *fun_name = strtab + sym.st_name;
            int index = search_func_in_list(new_funcs, len,
                    fun_name);
            sym.st_value = new_funcs[index].offset;

            /* write new symbol to file */
            lseek(fd, -sizeof(Elf32_Sym), SEEK_CUR);
            write(fd, &sym, sizeof(Elf32_Sym));
         }
    }
}

func_t *get_func_list(int fd, Elf32_Ehdr *ehdr, int *len){
    Elf32_Shdr sym_shdr = get_sec_from_name(fd, ehdr, ".symtab");
    Elf32_Shdr strtab_shdr = get_sec_from_name(fd, ehdr, ".strtab");
    Elf32_Shdr text_shdr = get_sec_from_name(fd, ehdr, ".text");
    Elf32_Sym sym;
    char *strtab = malloc(strtab_shdr.sh_size);

    /* Go to strtab offset */
    lseek(fd, strtab_shdr.sh_offset, SEEK_SET);

    /* read strtab */
    read(fd, strtab, strtab_shdr.sh_size);

    /* Go to sym table offset */
    lseek(fd, sym_shdr.sh_offset, SEEK_SET);

    /* Iterate through symbols */
    int sym_num = sym_shdr.sh_size / sizeof(Elf32_Sym);
    func_t *funcs = malloc(sizeof(func_t) * sym_num);

    /* text section boundary */
    int text_start = text_shdr.sh_offset;
    int text_end = text_start + text_shdr.sh_size;

    for(int i = 0; i < sym_num; i++){
        read(fd, &sym, sizeof(Elf32_Sym));
        if(ELF32_ST_TYPE(sym.st_info) == STT_FUNC && sym.st_value >= text_start && sym.st_value <= text_end){
            (funcs + *len)->name = strtab + sym.st_name;
            (funcs + *len)->offset = sym.st_value;
            (*len)++;
        }
    }

    return funcs;
}

void print_func_list(func_t *funcs, int len){
    for(int i = 0; i < len; i++){
        printf("%s\t%x\n", (funcs + i)->name, (funcs + i)->offset);
    }
}

int search_func_in_list(func_t *funcs, int len, const char *key){
    for(int i = 0; i < len; i++){
        if(strcmp((funcs + i)->name, key) == 0){
            return i; 
        }
    }
    return -1;
}

void sort_func_list(func_t *funcs, int len){
    qsort(funcs, len, sizeof(func_t), compare_func);
}

int compare_func(const void *a, const void *b){
    func_t *f1 = (func_t *)a;
    func_t *f2 = (func_t *)b;

    if(f1->offset < f2->offset) return -1;
    if(f1->offset > f2->offset) return 1;

    return 0;
}

int write_file(int fd, int offset, const unsigned char *content, int len){
    lseek(fd, offset, SEEK_SET);
    return write(fd, content, len);
}

void print_hex(const unsigned char *content, int len){
   int i;

   for(i = 0; i < len; i++){
        printf("%02x", content[i]);
   }

   printf("\n");
}
