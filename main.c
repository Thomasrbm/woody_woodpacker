#include "woody.h"

Elf64_Ehdr	*get_header(const char *binary)
{
	int fd = open(binary, O_RDONLY);
	if (fd < 0)
		return NULL;
	long size = lseek(fd, 0, SEEK_END);

	Elf64_Ehdr *header = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (header == MAP_FAILED)
		return NULL;

	return header;
}

bool	is_ELF64(Elf64_Ehdr *header)
{
	if (!header)
		return false;
	
	// indique le type de binaire , ici ELF
	if (memcmp(header->e_ident, ELFMAG, 4) != 0)
		return false;

	// indique la version (32 ou 64bits)
	// pour connaitre la taille des offsets et tous ta capte
	if (header->e_ident[EI_CLASS] != ELFCLASS64)
		return false;

	return true;
}

int	main(int ac, char **av)
{	
	if (ac != 2)
	{
		dprintf(2, "usage: %s <ELF64 binary>", av[0]);
		return EXIT_FAILURE;
	}

	// structure qui contient le "header du binaire"
	// le header du binaire contient plusieurs info importante pour le projet
	// comme le type du binaire lui meme , la taille des offsets et aussi l'entrypoint
	Elf64_Ehdr	*header = get_header(av[1]);
	if (!is_ELF64(header))
	{
		dprintf(2, "binary must be an ELF64");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}