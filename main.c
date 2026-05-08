#include "woody.h"

void *get_header(const char *binary)
{
	int fd = open(binary, O_RDONLY);
	if (fd < 0)
	{
		dprintf(STDERR_FILENO, "Error: couldn't open binary\n");
		return NULL;
	}

	// on prends la taille du binaire pour recupere toutes sa memoire
	// avec mmap juste apres
	// lseek met l'offset du fichier a la fin avec ce flag
	long size = lseek(fd, 0, SEEK_END);
	if (size <= 0)
	{
		dprintf(STDERR_FILENO, "Error: coudn't get binary size\n");
		close(fd);
		return NULL;
	}
	// on remet l'offset du fichier au debut pour mmap
	lseek(fd, 0, SEEK_SET);

	void *map = mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE, fd, 0);
	close(fd);

	if (map == MAP_FAILED)
		return NULL;

	return map;
}

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
	// pas sur que sa soit utile sur le projet mais on verifie
	// l'architecture pour etre sur que sa soit la meme que le code
	// qu'on va injecter
	if (header->e_machine != EM_X86_64)
		return false;

	// indique la version (32 ou 64bits)
	// pour connaitre la taille des offsets et tous ta capte
	return true;
}

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

	if (header->e_machine != EM_X86_64)
		return false;

	return true;
}

void handle_ELF32(__attribute__((unused))Elf32_Ehdr *header)
{
}

void handle_ELF64(__attribute__((unused))Elf64_Ehdr *header)
{
}

int main(int ac, char **av)
{
	if (ac != 2)
	{
		dprintf(STDERR_FILENO, "Usage: %s <ELF32/64 binary>\n", av[0]);
		return EXIT_FAILURE;
	}

	void *map = get_header(av[1]);
	if (!map)
		return EXIT_FAILURE;

	if (is_ELF32((Elf32_Ehdr *)map))
		handle_ELF32((Elf32_Ehdr *)map);
	else if (is_ELF64((Elf64_Ehdr *)map))
		handle_ELF64((Elf64_Ehdr *)map);
	else
	{
		dprintf(STDERR_FILENO, "Error: The binary has an unknown offset size\n");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}