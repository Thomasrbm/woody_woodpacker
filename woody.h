#ifndef WOODY_H
#define WOODY_H

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <elf.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

typedef struct s_map
{
    size_t size;
    void *offset;
} t_map;

typedef struct s_woody32
{
    size_t size;
    void *offset;
    Elf32_Ehdr *ehdr;
    Elf32_Phdr *phdr;
} t_woody32;

typedef struct s_woody64
{
    size_t size;
    void *offset;
    Elf64_Ehdr *ehdr;
    Elf64_Phdr *phdr;
} t_woody64;

// ELF32.c
bool is_ELF32(Elf32_Ehdr *header);
t_woody32 *build_ELF32_data(t_map *map);
void handle_ELF32(t_map *map);

// ELF64.c
bool is_ELF64(Elf64_Ehdr *header);
t_woody64 *build_ELF64_data(t_map *map);
void handle_ELF64(t_map *map);

#endif