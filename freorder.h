#ifndef F_REORDER_H
#define F_REORDER_H

#include <elf.h>

/* struct containgin function name and offset in file*/
typedef struct{
   char *name;
   int offset; 
} func_t;

/* Returns program header of the segment containg text section */
Elf32_Phdr *get_seg_text_phdr(char *elf_file);

/** Re-orders text segment according to given function order
 *
 *  params: elf_file - ELF file to re-order
 *  params: new_function_order - Given function order
 *
 */
void reorder_seg_text(char *elf_file, func_t *new_func_order);

/* Update symble table with new function offset */
void update_sym_tab(char *elf_file, func_t *new_funcs, int len);

/** Returns a list of all the functions from text section
 *  
 *  param: elf_file - ELF file
 *  param: len  - Pointer to get number of functions
 *
 *  returns: pointer pointing to an array of func_t 
 */
func_t *get_func_list(char *elf_file, int *len);


/* Sorts list of functions according to offset */
void sort_func_list(func_t *funcs, int len);

/* Comparison function for sort_func_list() */
int compare_func(const void *a, const void *b);

/* Prints a list of functions in stdout */
void print_func_list(func_t *funcs, int len);

/* Search a function in a list of functions */
int search_func_in_list(func_t *funcs, int len, const char *key);

/* Prints raw bytes in stdout */
void print_hex(const unsigned char *content, int len);

#endif
