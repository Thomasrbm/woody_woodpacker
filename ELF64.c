#include "woody.h"

// meme schema que pour la verification du format ELF32
bool is_ELF64(Elf64_Ehdr *header)
{
	if (!header)
		return false;

	if (memcmp(header->e_ident, ELFMAG, SELFMAG) != 0)
	{
		dprintf(STDERR_FILENO, "Error: The binary must have an ELF format\n");
		return false;
	}

	if (header->e_ident[EI_CLASS] != ELFCLASS64)
		return false;

	return true;
}

t_woody64 *build_ELF64_data(t_map *map)
{
	t_woody64 *woody64 = calloc(1, sizeof(t_woody64));
	if (!woody64)
	{
		munmap(map->offset, map->size);
		return NULL;
	}

	woody64->size = map->size;
	woody64->offset = map->offset;
	woody64->ehdr = (Elf64_Ehdr *)map->offset;
	woody64->phdr = (Elf64_Phdr *)((char *)map->offset + woody64->ehdr->e_phoff);

	return woody64;
}

void handle_ELF64(t_map *map)
{
	t_woody64 *woody64 = build_ELF64_data(map);
	if (!woody64)
		return;

	Elf64_Phdr *code_segment = NULL;
	for (int i = 0; i < woody64->ehdr->e_phnum; i++)
	{
		Elf64_Phdr *tmp = &woody64->phdr[i];

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

	free(woody64);
}