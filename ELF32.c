#include "woody.h"

bool is_ELF32(Elf32_Ehdr *header)
{
	if (!header)
		return false;
	// indique le type de binaire , ici ELF
	// on regarde les 4 premiers charactere qui nous indique
	// si c'est un ELF ou non
	// ELFMAG == la chaine de caractere a match
	// SELFMAG == la longueur de cette chaine
	if (memcmp(header->e_ident, ELFMAG, SELFMAG) != 0)
	{
		dprintf(STDERR_FILENO, "Error: The binary must have an ELF format\n");
		return false;
	}

	if (header->e_ident[EI_CLASS] != ELFCLASS32)
		return false;

	// indique la version (32 ou 64bits)
	// pour connaitre la taille des offsets et tous ta capte
	return true;
}

t_woody32 *build_ELF32_data(t_map *map)
{
	t_woody32 *woody32 = calloc(1, sizeof(t_woody32));
	if (!woody32)
	{
		munmap(map->offset, map->size);
		return NULL;
	}

	woody32->size = map->size;
	woody32->offset = map->offset;
	woody32->ehdr = (Elf32_Ehdr *)map->offset;
	woody32->phdr = (Elf32_Phdr *)((char *)map->offset + woody32->ehdr->e_phoff);
	woody32->key = get_encrypt_key();

	return woody32;
}

void handle_ELF32(t_map *map)
{
	t_woody32 *woody32 = build_ELF32_data(map);
	if (!woody32)
		return ;

	Elf32_Phdr *code_segment = NULL;
	for (int i = 0; i < woody32->ehdr->e_phnum; i++)
	{
		Elf32_Phdr *tmp = &woody32->phdr[i];

		if (tmp->p_type == PT_LOAD && (tmp->p_flags & PF_X))
		{
			code_segment = tmp;
			break;
		}
	}

	
	mprotect(
		(void *)((char *)map + code_segment->p_vaddr),
		code_segment->p_memsz,
		PROT_READ | PROT_WRITE | PROT_EXEC);
	
	xtea_iter((uint8_t *)map + code_segment->p_vaddr, code_segment->p_memsz, woody32->key);

	free(woody32->key);
	free(woody32);
}