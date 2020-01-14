#ifndef LOADER_H
#define LOADER_H

#include <elf.h>
#include <stdio.h>

/* Structure containing function name and address */
typedef struct{
    char *name;
    Elf32_Addr addr;
} lib_func_t;

/* Returns size of a fiel */
size_t file_sz(FILE *fp);

/* Loads file into memory */
void *load_file(const char *name, size_t *fsize);

/* Write data to file */
int write_file(char *elf_file, int fsize, char *name);

/* 
 * Maps loadable segments into memory and returns
 * pointer to memory
 */
void *load_image(char *elf_file, size_t size);

/* Loads an ELF binary into memory and returns entry point */
void *load_bin_image(char *elf_file, size_t size);

/* 
 * Loads an ELF shared library into memory and returns loaded
 * address of all the functions
 */
void load_sh_lib_image(char *lib_file, size_t size,
                            lib_func_t *funcs, int nfunc);

/* Resolves function addresses in GOT */
void resolve_elf_got(char *bin_file, size_t size,
                        lib_func_t *funcs, int funcs_len);

/* Returns memory address of a symbol */
void *find_sym(const char *name, Elf32_Shdr *shdr,
                const char *strtbl, const char *elf,
                const char *mem);

/* Returns section index */
int sec_index_from_name(char *elf_file,
                        Elf32_Ehdr *ehdr,
                        char *name);

/* Returns section header */
Elf32_Shdr *sec_from_name(char *elf_file, char *name);

/* Prints GOT */
void print_got(char *bin_file);

#endif
