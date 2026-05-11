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

// placeholders 4 octets dans le stub32
#define PH32_DELTA_TEXT_A 0x11111111U // mp_addr   - stub_vaddr (mprotect)
#define PH32_MPROTECT_SIZE 0x22222222U
#define PH32_DELTA_ENTRY 0x33333333U  // orig_entry - stub_vaddr
#define PH32_DELTA_TEXT_B 0x44444444U // text_vaddr - stub_vaddr (decrypt)
#define PH32_DECRYPT_SIZE 0x55555555U

#define PAGE_SIZE_32 0x1000U

typedef struct s_woody32
{
    size_t size;
    void *offset;
    Elf32_Ehdr *ehdr;
    Elf32_Phdr *phdr;
    uint32_t *key;
} t_woody32;

typedef struct s_segs32
{
	Elf32_Phdr *text_phdr;
	Elf32_Phdr *note_phdr;
	uint32_t highest_vaddr_end;
} t_segs32;

typedef struct s_layout32
{
	uint32_t enc_file_off;
	uint32_t enc_vaddr;
	uint32_t enc_size;
	uint32_t mp_addr;
	uint32_t mp_size;
	uint32_t stub_file_off;
	uint32_t stub_vaddr;
	size_t out_size;
} t_layout32;

// placeholders 8 octets dans le stub
#define PH_DELTA_TEXT_A 0x1111111111111111ULL // mp_addr   - stub_vaddr (mprotect)
#define PH_MPROTECT_SIZE 0x2222222222222222ULL
#define PH_DELTA_ENTRY 0x3333333333333333ULL  // orig_entry - stub_vaddr
#define PH_DELTA_TEXT_B 0x4444444444444444ULL // text_vaddr - stub_vaddr (decrypt)
#define PH_DECRYPT_SIZE 0x5555555555555555ULL

#define PAGE_SIZE 0x1000UL

typedef struct s_woody64
{
    size_t size;
    void *offset;
    Elf64_Ehdr *ehdr;
    Elf64_Phdr *phdr;
    uint32_t *key;
} t_woody64;

// resultats du scan des phdrs
typedef struct s_segs64
{
	Elf64_Phdr *text_phdr;
	Elf64_Phdr *note_phdr;
	uint64_t highest_vaddr_end;
} t_segs64;

// emplacement de la zone chiffree, de la zone mprotect, et du stub
typedef struct s_layout64
{
	uint64_t enc_file_off;
	uint64_t enc_vaddr;
	uint64_t enc_size;
	uint64_t mp_addr;
	uint64_t mp_size;
	uint64_t stub_file_off;
	uint64_t stub_vaddr;
	size_t out_size;
} t_layout64;

// ELF32.c
bool is_ELF32(Elf32_Ehdr *header);
t_woody32 *build_ELF32_data(t_map *map);
void handle_ELF32(t_map *map);

// ELF64.c
bool is_ELF64(Elf64_Ehdr *header);
t_woody64 *build_ELF64_data(t_map *map);
void handle_ELF64(t_map *map);

// main.c
uint32_t *get_encrypt_key(void);

// xtea.c
void xtea_iter(uint8_t *data, size_t len, const uint32_t key[4]);

#endif