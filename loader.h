#ifndef LOADER_H
#define LOADER_H

#include <elf.h>

/* struct containgin function name and offset in file*/
typedef struct{
   char *name;
   int offset; 
} func_t;

/* Returns program header of the segment containg text section */
Elf32_Phdr get_seg_text_phdr(int fd, Elf32_Ehdr *ehdr);

/* Returns a section header from a section name */
Elf32_Shdr get_sec_from_name(int fd, Elf32_Ehdr *ehdr,
                                const char *name);

/** Re-orders text segment according to given function order
 *
 *  params: fd - file descriptor of the ELF binary file
 *  params: seg_text - Current text segment
 *  params: seg_text_phdr - text segment program header
 *  params: sec_text_shdr - text section header
 *  params: cur_function_order - Current function order
 *  params: len - Number of functions
 *  params: new_function_order - Given function order
 *
 *  returns: Re-ordered text segment
 */
int reorder_seg_text(int fd, unsigned char *seg_text,
                        Elf32_Phdr *seg_text_phdr,
                        Elf32_Shdr *sec_text_shdr,
                        func_t *cur_func_order,
                        int *len, char *new_func_order[]);

/** Returns a list of all the functions from text section
 *  
 *  param: fd - file descriptor of the ELF binary file
 *  param: ehdr - pointer pointing to ELF header
 *  param: len  - Pointer to get number of functions
 *
 *  returns: pointer pointing to an array of func_t 
 */
func_t *get_func_list(int fd, Elf32_Ehdr *ehdr, int *len);


/* Sorts list of functions according to offset */
void sort_func_list(func_t *funcs, int len);

/* Comparison function for sort_func_list() */
int compare_func(const void *a, const void *b);

/* Prints a list of functions in stdout */
void print_func_list(func_t *funcs, int len);

/* Search a function in a list of functions */
int search_func_in_list(func_t *funcs, int len, const char *key);

/* Write data to a file. Same as write(2) system with
 * offset parameter added. Will write to that offset.
 */
int write_file(int fd, int offset, const unsigned char *content, int len);

/* Prints raw bytes in stdout */
void print_hex(const unsigned char *content, int len);

#endif
