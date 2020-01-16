#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include "loader.h"

void *load_image(char *elf_file, size_t size){
    Elf32_Ehdr *ehdr = NULL; 
    Elf32_Phdr *phdr = NULL;
    char *start = NULL;
    char *taddr = NULL;
    char *exec = NULL;

    ehdr = (Elf32_Ehdr *)elf_file;
    
    /* Check ELF validity */
    if(!(ehdr->e_ident[EI_MAG0] == ELFMAG0 &&
            ehdr->e_ident[EI_MAG1] == ELFMAG1 &&
            ehdr->e_ident[EI_MAG2] == ELFMAG2 &&
            ehdr->e_ident[EI_MAG3] == ELFMAG3)){

        printf("Not a valid ELF file!\n");
        exit(EXIT_FAILURE);
    }

    /* Request for memory */
    exec = mmap(NULL, size,
            PROT_READ | PROT_WRITE | PROT_EXEC,
            MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);

    /* Initialize memory with zero */
    memset(exec, 0, size);

    phdr = (Elf32_Phdr *)(elf_file + ehdr->e_phoff);
    for(int i = 0; i < ehdr->e_phnum; i++){
       if(phdr[i].p_type == PT_LOAD){
            start = elf_file + phdr[i].p_offset;
            taddr = exec + phdr[i].p_vaddr;
            memmove(taddr, start, phdr[i].p_filesz);
       }
    }
    
    return exec;
}

void *load_bin_image(char *elf_file, size_t size){
   Elf32_Ehdr *ehdr = NULL;
   Elf32_Shdr *shdr = NULL;
   char *exec = NULL; 
   char *strtbl = NULL;
   void *entry = NULL;

   ehdr = (Elf32_Ehdr *)elf_file;

   exec = load_image(elf_file, size);

   /* find main */ 
   shdr = (Elf32_Shdr *)(elf_file + ehdr->e_shoff);

   for(int i = 0; i < ehdr->e_shnum; i++){
       if(shdr[i].sh_type == SHT_SYMTAB){
           strtbl = elf_file + shdr[shdr[i].sh_link].sh_offset; 
           entry = find_sym("main", shdr + i, strtbl, elf_file, exec); 
           break;
       }
   }

   return entry; 
}

lib_func_t *load_sh_lib_image(char *lib_file, size_t size,
                                int *nfunc){
    
    Elf32_Ehdr *ehdr = NULL;
    Elf32_Shdr *shdr = NULL;
    Elf32_Shdr *dyn_shdr = NULL;
    Elf32_Sym *sym = NULL;
    char *exec = NULL;
    char *strtab = NULL;
    char *dyn_strtab = NULL;
    char *sym_name = NULL;
    Elf32_Addr addr = 0;
    int nsym = 0;

    ehdr = (Elf32_Ehdr *)lib_file;

    /* load library image to memory */
    exec = load_image(lib_file, size);

    /* Find fuctions */
    shdr = (Elf32_Shdr *)(lib_file + ehdr->e_shoff);

    // Load symble table
    int index = sec_index_from_name(lib_file, ehdr,
                                    ".symtab");
    
    strtab = (char *)(lib_file + shdr[shdr[index].sh_link].sh_offset);

    // Load dynamic symbols
    dyn_shdr = sec_from_name(lib_file, ".dynsym");
    sym = (Elf32_Sym *)(lib_file + dyn_shdr->sh_offset);
    nsym = dyn_shdr->sh_size / sizeof(Elf32_Sym);

    // Load dynamic string table
    dyn_shdr = sec_from_name(lib_file, ".dynstr");
    dyn_strtab = lib_file + dyn_shdr->sh_offset;

    *nfunc = 0;
    int j = 0;
    
    lib_func_t *funcs = (lib_func_t *)malloc(sizeof(lib_func_t) * nsym);

    for(int i = 0; i < nsym; i++){
        if(ELF32_ST_TYPE((sym + i)->st_info) == STT_FUNC &&
            ELF32_ST_BIND((sym + i)->st_info) == STB_GLOBAL){
        
            sym_name = dyn_strtab + (sym + i)->st_name;
            addr = (Elf32_Addr)find_sym(sym_name,
                                      shdr + index,
                                      strtab, lib_file, exec);

            if(addr){
                *nfunc += 1;
                funcs[*nfunc - 1].name = sym_name;
                funcs[*nfunc - 1].addr = addr;
            }

        }
    }

    return funcs;
}

lib_func_t *get_bin_functions(char *bin_file, int *nfunc){
    lib_func_t *functions = NULL;
    *nfunc = 0;

    return functions;
}

void resolve_elf_got(char *bin_file, size_t fsize,
                        lib_func_t *funcs, int funcs_len){

    Elf32_Ehdr *ehdr = NULL;
    Elf32_Shdr *rel_shdr = NULL;
    Elf32_Shdr *got_shdr = NULL;
    Elf32_Shdr *sym_shdr = NULL;
    Elf32_Shdr *str_shdr = NULL;
    Elf32_Sym *symtab = NULL;
    char *strtab = NULL;
    Elf32_Rel *rel = NULL;
    Elf32_Addr *addr = NULL;
    int rel_entry;

    /* program header */
    ehdr = (Elf32_Ehdr *)bin_file;

    /* load string table */
    int strtab_index = sec_index_from_name(bin_file, ehdr,
                                            ".dynstr");
    str_shdr = (Elf32_Shdr *)(bin_file + ehdr->e_shoff)
                    + strtab_index;
    strtab = (char *)(bin_file + str_shdr->sh_offset);

    /* load symbol table */
    int symtab_index = sec_index_from_name(bin_file, ehdr,
                                            ".dynsym");
    sym_shdr = (Elf32_Shdr *)(bin_file + ehdr->e_shoff)
                    + symtab_index;
    symtab = (Elf32_Sym *)(bin_file + sym_shdr->sh_offset);

    print_got(bin_file);

    /* index of .rel.plt section */
    int rel_index = sec_index_from_name(bin_file, ehdr,
                                        ".rel.plt");

    /* section header of .rel.plt section */
    rel_shdr = (Elf32_Shdr *)(bin_file + ehdr->e_shoff)
                    + rel_index;

    /* number of relocation entry in .rel.plt */
    rel_entry = rel_shdr->sh_size / sizeof(Elf32_Rel);
    
    /* iterate through all entry */
    rel = (Elf32_Rel *)(bin_file + rel_shdr->sh_offset);
    for(int i = 0; i < rel_entry; i++){
        int r_offset = (rel + i)->r_offset - 0x1000;
        int sym_index = ELF32_R_SYM((rel + i)->r_info);
        
        addr = (Elf32_Addr *)(bin_file + r_offset);
        char *name = strtab + (symtab + sym_index)->st_name;
               
        for(int j = 0; j < funcs_len; j++){
            if(strcmp(name, funcs[j].name) == 0){
                *addr = funcs[j].addr;
            }     
        }
    }
    
    print_got(bin_file);
}

void print_got(char *bin_file){
    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)bin_file;
    
    /* index of .got section */
    int got_index = sec_index_from_name(bin_file, ehdr, ".got");

    /* .got section header */
    Elf32_Shdr *got_shdr = 
        (Elf32_Shdr *)(bin_file + ehdr->e_shoff) + got_index;

    /* iterate through got */
    int got_entry = got_shdr->sh_size / sizeof(Elf32_Addr);
    
    Elf32_Addr *addr =
        (Elf32_Addr *)(bin_file + got_shdr->sh_offset);
    
    printf("### GOT IN: %x ###\n", got_shdr->sh_offset);
    for(int i = 0; i < got_entry; i++){
        printf("%x : %x\n", got_shdr->sh_offset + i * 4, *(addr + i));
    }
}

int sec_index_from_name(char *elf_file,
                        Elf32_Ehdr *ehdr, char *name){

    Elf32_Shdr *shdr = NULL;
    char *strtab = NULL;
    
    shdr = (Elf32_Shdr *)(elf_file + ehdr->e_shoff);

    /* section name string table */ 
    strtab = (char *)(elf_file +
            shdr[ehdr->e_shstrndx].sh_offset);

    for(int i = 0; i < ehdr->e_shnum; i++){
        if(strcmp(strtab + shdr[i].sh_name, name) == 0){
            return i; 
        }
    }

    return -1;
}

Elf32_Shdr *sec_from_name(char *elf_file, char *name){
    Elf32_Ehdr *ehdr = NULL;
    Elf32_Shdr *shdr = NULL;
    int index;

    ehdr = (Elf32_Ehdr *)elf_file;

    index = sec_index_from_name(elf_file, ehdr, name);
    shdr = (Elf32_Shdr *)(elf_file + ehdr->e_shoff);

    return shdr + index;
}

void *find_sym(const char *name, Elf32_Shdr *shdr,
        const char *strtbl, const char *elf, const char *mem){
        
    Elf32_Sym *sym = (Elf32_Sym *)(elf + shdr->sh_offset);
    int nsym = shdr->sh_size / sizeof(Elf32_Sym);

    for(int i = 0; i < nsym; i++){
        if(strcmp(name, strtbl + sym[i].st_name) == 0){
            return mem + sym[i].st_value;
        }
    }

    return NULL;
}

void *load_file(const char *name, size_t *fsize){
    /* Read the elf file */
    FILE *fp;
    
    if((fp = fopen(name, "rb")) == NULL){
        perror(name);
        exit(EXIT_FAILURE);
    }

    *fsize = file_sz(fp);
    char *buff = malloc(*fsize);
    fread(buff, 1, *fsize, fp);
    fclose(fp);

    return buff;
}

int write_file(char *elf_file, int fsize, char *name){
    FILE *fp;
    
    if((fp = fopen(name, "wb")) == NULL){
        perror(name);
        exit(EXIT_FAILURE);
    }

    int nbytes = fwrite(elf_file, 1, fsize, fp);
    fclose(fp);
    return nbytes;
}

size_t file_sz(FILE *fp){
    size_t fsize, curpos;

    curpos = ftell(fp);
    /* Go to file end */
    fseek(fp, 0, SEEK_END);
    /* Count number of bytes */
    fsize = ftell(fp);
    /* Restore position */
    fseek(fp, curpos, SEEK_SET);

    return fsize;
}
