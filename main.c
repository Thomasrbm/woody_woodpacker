#include "woody.h"

t_map *get_binary_assets(const char *binary)
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

	void *ptr = mmap(
		NULL,
		size,
		PROT_READ,
		MAP_PRIVATE,
		fd,
		0);

	close(fd);

	if (ptr == MAP_FAILED)
		return NULL;

	t_map *map = calloc(1, sizeof(t_map));
	if (!map)
		return NULL;

	map->size = size;
	map->offset = ptr;

	return map;
}

int main(int ac, char **av)
{
	if (ac != 2)
	{
		dprintf(STDERR_FILENO, "Usage: %s <ELF32/64 binary>\n", av[0]);
		return EXIT_FAILURE;
	}

	t_map *map = get_binary_assets(av[1]);
	if (!map)
		return EXIT_FAILURE;

	if (is_ELF32((Elf32_Ehdr *)map->offset))
		handle_ELF32(map);
	else if (is_ELF64((Elf64_Ehdr *)map->offset))
		handle_ELF64(map);
	else
	{
		munmap(map->offset, map->size);
		free(map);
		dprintf(STDERR_FILENO, "Error: The binary has an unknown offset size\n");
		return EXIT_FAILURE;
	}

	munmap(map->offset, map->size);
	free(map);

	return EXIT_SUCCESS;
}