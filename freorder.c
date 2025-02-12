#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "loader.h"
#include "freorder.h"

void find_call(char *elf_file, int start, int end,
                int *nxt_ins_off, int *fun_off);

char *sym_from_offset(char *elf_file, int offset);

call_graph_t *search_func_in_graph(const char *fname,
                                    call_graph_t *cgraph,
                                    int nfunc);

Elf32_Phdr *get_seg_text_phdr(char *elf_file){
    int i;
    Elf32_Ehdr *ehdr = NULL;
    Elf32_Shdr *shdr = NULL;
    Elf32_Phdr *phdr = NULL;

    /* elf header */
    ehdr = (Elf32_Ehdr *)elf_file;

    /* find offset of text section */
    int index = sec_index_from_name(elf_file, ehdr, ".text");
    shdr = (Elf32_Shdr *)(elf_file + ehdr->e_shoff) + index;
    Elf32_Off text_offset = shdr->sh_offset;

    /* Go to segment offset */
    phdr = (Elf32_Phdr *)(elf_file + ehdr->e_phoff);

    /* Find segment containing .text section */
    for(i = 0; i < ehdr->e_phnum; i++){
        if((phdr + i)->p_type == PT_LOAD){
            Elf32_Off off_start = (phdr + i)->p_offset;
            Elf32_Off off_end = off_start +
                                  (phdr + i)->p_filesz;

            if(text_offset >= off_start &&
                    text_offset <= off_end){
                return phdr + i; 
            }
        }
    }

    return NULL;
}

call_graph_t *get_call_graph(char *elf_file){
    Elf32_Shdr *text_shdr;
    func_t *funcs;
    int funcs_len;
    int fstart;
    int textend;
    int fend;
    int ins_off = 0;
    int ins_val = 0;

    // find end of text sections
    text_shdr = sec_from_name(elf_file, ".text");
    textend = text_shdr->sh_offset + text_shdr->sh_size - 1;

    funcs = get_func_list(elf_file, &funcs_len);
    sort_func_list(funcs, funcs_len);
    
    call_graph_t *cgraph = malloc(sizeof(call_graph_t) *
                                        funcs_len);

    // find calls in functions in text section
    for(int i = 0; i < funcs_len; i++){
        if(i < (funcs_len - 1)){
            fstart = funcs[i].offset;
            fend = funcs[i + 1].offset - 1;
        }
        else{
            fstart = funcs[i].offset;
            fend = textend;
        }
         
        find_call(elf_file, fstart, fend, &ins_off, &ins_val);
        
        cgraph[i].fname = funcs[i].name;
        cgraph[i].ins_off = ins_off; // 0 if no call
        cgraph[i].ins_val = ins_val; // 0 if no call

        if(ins_val){
            cgraph[i].cfname =
                sym_from_offset(elf_file, fstart + ins_off + ins_val + 4);
        }
        else{
            cgraph[i].cfname  = "";
        }
    }

    free(funcs);
    return cgraph;
}

void find_call(char *elf_file, int start, int end,
                    int *ins_off, int *ins_val){
    
    unsigned char *byte;
    *ins_off = 0;
    *ins_val = 0;
    
    for(int i = start; i <= end; i++){
        byte = (unsigned char *)elf_file + i;
        if((int)(*byte) == 0xe8){
            *ins_val = *(int *)(elf_file + i + 1);
            *ins_off = (i + 1) - start;
        }
    }
}

char *sym_from_offset(char *elf_file, int offset){
    Elf32_Shdr *shdr = NULL;
    Elf32_Sym *sym = NULL;
    char *strtab = NULL;
    int nsym = 0;

    shdr = sec_from_name(elf_file, ".symtab");
    sym = (Elf32_Sym *)(elf_file + shdr->sh_offset);
    nsym = shdr->sh_size / sizeof(Elf32_Sym);

    shdr = sec_from_name(elf_file, ".strtab");
    strtab = elf_file + shdr->sh_offset;

    for(int i = 0; i < nsym; i++){
        if((sym[i].st_value) == offset){
            return strtab + sym[i].st_name;
        }
    }

    return NULL;
}

void resolve_func_calls(char *elf_file, call_graph_t *cgraph){
    func_t *funcs = NULL;
    int nfunc = 0;
    call_graph_t *fcall = NULL;
    int index = 0;
    int offset = 0;

    funcs = get_func_list(elf_file, &nfunc);
    for(int i = 0; i < nfunc; i++){
        // find the function's call graph
        fcall = search_func_in_graph(funcs[i].name, cgraph,
                                        nfunc);

        // calculate offset of the called function
        if(fcall->ins_val){
            index = search_func_in_list(funcs, nfunc,
                                            fcall->cfname);
            offset = funcs[index].offset - (funcs[i].offset + fcall->ins_off + 4);
        
            // fix offset
            if(offset != fcall->ins_val){
                printf("changing offset of %s from %x to %x\n", fcall->fname, fcall->ins_val, offset);
                *(int *)(elf_file + (funcs[i].offset + fcall->ins_off)) = offset;
            }
        }
    }

    free(funcs);
}

call_graph_t *search_func_in_graph(const char *fname,
                                    call_graph_t *cgraph,
                                    int nfunc){

    for(int i = 0; i < nfunc; i++){
        if(strcmp(cgraph[i].fname, fname) == 0){
            return cgraph + i;
        } 
    }

    return NULL;
}

void reorder_seg_text(char *elf_file, func_t *new_func_order){

    Elf32_Ehdr *ehdr = NULL;
    Elf32_Phdr *seg_text_phdr = NULL;
    Elf32_Shdr *sec_text_shdr = NULL;
    char *seg_text = NULL;
    func_t *cur_func_order = NULL;
    int func_len;

    // program header
    ehdr = (Elf32_Ehdr *)elf_file;
    
    // text segment
    seg_text_phdr = get_seg_text_phdr(elf_file);
    seg_text = elf_file + seg_text_phdr->p_offset;
    
    // text section header
    int index = sec_index_from_name(elf_file, ehdr, ".text");
    sec_text_shdr = (Elf32_Shdr *)(elf_file + ehdr->e_shoff)
                    + index;

    int fun_start, fun_end, fun_len;
    int seg_base =  seg_text_phdr->p_offset;
    int bytes_writen = 0;
    char *new_seg_text = malloc(seg_text_phdr->p_filesz);

    // copy data before text section into new segment
    bytes_writen = sec_text_shdr->sh_offset - seg_base;
    memcpy(new_seg_text, seg_text, bytes_writen);

    // end of text section
    int text_end = sec_text_shdr->sh_offset + sec_text_shdr->sh_size; 

    // functions in current order
    cur_func_order = get_func_list(elf_file, &func_len);
    sort_func_list(cur_func_order, func_len);

    for(int i = 0; i < func_len; i++){
        index = search_func_in_list(cur_func_order,
                    func_len, new_func_order[i].name); 
        fun_start = cur_func_order[index].offset;
        
        if(index == (func_len - 1)){
            fun_end = text_end - 1;
        }
        else{
            fun_end = cur_func_order[index + 1].offset - 1;
        }

        fun_len = fun_end - fun_start + 1;
        
        // copy function data into new segment
        memcpy(new_seg_text + bytes_writen, seg_text +
                (fun_start - seg_base), fun_len);

        // Update function offset
        new_func_order[i].offset = seg_base + bytes_writen;
        bytes_writen += fun_len;
    }

    // copy rest of the data into new segment
    fun_len = seg_text_phdr->p_offset +
            seg_text_phdr->p_filesz - text_end;
    
    memcpy(new_seg_text + bytes_writen,
            seg_text + (text_end - seg_base), fun_len);
    bytes_writen += fun_len;

    // copy re-ordered segment into the original segment
    memcpy(seg_text, new_seg_text, bytes_writen);
    
    // free up memory
    free(new_seg_text);
    free(cur_func_order);
}

void update_sym_tab(char *elf_file, func_t *new_funcs, int len){

    Elf32_Shdr *sym_shdr = NULL;
    Elf32_Shdr *strtab_shdr = NULL;
    Elf32_Shdr *text_shdr = NULL;
    Elf32_Sym *sym = NULL;
    char *strtab = NULL;
    
    sym_shdr = sec_from_name(elf_file, ".symtab");
    strtab_shdr = sec_from_name(elf_file, ".strtab");
    text_shdr = sec_from_name(elf_file, ".text");

    // Load symbol table
    sym = (Elf32_Sym *)(elf_file + sym_shdr->sh_offset);
    
    // text section boundary
    int text_start = text_shdr->sh_offset;
    int text_end = text_start + text_shdr->sh_size;

    // Load string table
    strtab = elf_file + strtab_shdr->sh_offset;
    
    // Iterate through symbols
    int sym_num = sym_shdr->sh_size / sizeof(Elf32_Sym);

    for(int i = 0; i < sym_num; i++){
        // Only functions that are in text section
        if(ELF32_ST_TYPE(sym[i].st_info) == STT_FUNC &&
                sym[i].st_value >= text_start &&
                sym[i].st_value <= text_end){

            // Change offset of the symbol
            char *fun_name = strtab + sym[i].st_name;
            int index = search_func_in_list(new_funcs, len,
                    fun_name);
            sym[i].st_value = new_funcs[index].offset;
         }
    }
    
    // Change offset in dynamic symbol table
    sym_shdr = sec_from_name(elf_file, ".dynsym");
    strtab_shdr = sec_from_name(elf_file, ".dynstr");
    sym = (Elf32_Sym *)(elf_file + sym_shdr->sh_offset);
    strtab = elf_file + strtab_shdr->sh_offset;
    sym_num = sym_shdr->sh_size / sizeof(Elf32_Sym);

    for(int i = 0; i < sym_num; i++){
        char *fun_name = strtab + sym[i].st_name; 
        int index = search_func_in_list(new_funcs, len,
                                            fun_name);

        if(index >= 0){
            sym[i].st_value = new_funcs[index].offset;
        }
    }
}

func_t *get_func_list(char *elf_file, int *len){
    Elf32_Ehdr *ehdr; 
    Elf32_Shdr *shdr;
    Elf32_Shdr *sym_shdr;
    Elf32_Shdr *strtab_shdr;
    Elf32_Shdr *text_shdr;
    Elf32_Sym *sym;
    int index;
    
    ehdr = (Elf32_Ehdr *)elf_file;
    shdr = (Elf32_Shdr *)(elf_file + ehdr->e_shoff);

    // Load symbol table
    index = sec_index_from_name(elf_file, ehdr, ".symtab");
    sym_shdr = shdr + index;
    sym = (Elf32_Sym *)(elf_file + sym_shdr->sh_offset);
    
    // Load string table
    index = sec_index_from_name(elf_file, ehdr, ".strtab");
    strtab_shdr = shdr + index;
    char *strtab = elf_file + strtab_shdr->sh_offset;
    
    // Text section header
    index = sec_index_from_name(elf_file, ehdr, ".text");
    text_shdr = shdr + index;

    // Iterate through symbols
    int sym_num = sym_shdr->sh_size / sizeof(Elf32_Sym);
    func_t *funcs = malloc(sizeof(func_t) * sym_num);

    // text section boundary
    int text_start = text_shdr->sh_offset;
    int text_end = text_start + text_shdr->sh_size;

    // initial length
    *len = 0;

    // Search functions in text section
    for(int i = 0; i < sym_num; i++){
        if(ELF32_ST_TYPE(sym[i].st_info) == STT_FUNC &&
                        sym[i].st_value >= text_start &&
                        sym[i].st_value <= text_end){
            
            (funcs + *len)->name = strtab + sym[i].st_name;
            (funcs + *len)->offset = sym[i].st_value;
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

void print_hex(const unsigned char *content, int len){
   int i;

   for(i = 0; i < len; i++){
        printf("%02x", content[i]);
   }

   printf("\n");
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

int write_funcs_to_file(func_t *funcs, int len, char *fname){
    int fd = open(fname, O_CREAT | O_WRONLY, 0644);
    char *buf;
    int nbytes = 0;

    if(fd == -1){
        perror("freorder");
        return -1;
    }

    for(int i = 0; i < len; i++){
        buf = funcs[i].name;
        nbytes += write(fd, buf, strlen(buf));
        nbytes += write(fd, "\n", 1);
    }

    return nbytes;
}
