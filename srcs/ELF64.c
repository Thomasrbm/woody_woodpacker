#include "woody.h"
#include "stub.h"

// placeholders 8 octets dans le stub
#define PH_DELTA_TEXT_A 0x1111111111111111ULL // mp_addr   - stub_vaddr (mprotect)
#define PH_MPROTECT_SIZE 0x2222222222222222ULL
#define PH_DELTA_ENTRY 0x3333333333333333ULL  // orig_entry - stub_vaddr
#define PH_DELTA_TEXT_B 0x4444444444444444ULL // text_vaddr - stub_vaddr (decrypt)
#define PH_DECRYPT_SIZE 0x5555555555555555ULL

#define PAGE_SIZE 0x1000UL

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
	woody64->key = get_encrypt_key();

	return woody64;
}

static void free_woody64(t_woody64 *w)
{
	if (!w)
		return;
	free(w->key);
	free(w);
}

// remplace la premiere occurence d'une valeur 64 bits dans le stub
static void patch_u64(uint8_t *buf, size_t len, uint64_t needle, uint64_t value)
{
	for (size_t i = 0; i + 8 <= len; i++)
	{
		if (memcmp(buf + i, &needle, 8) == 0)
		{
			memcpy(buf + i, &value, 8);
			return;
		}
	}
}

static void patch_u32(uint8_t *buf, size_t len, uint32_t needle, uint32_t value)
{
	for (size_t i = 0; i + 4 <= len; i++)
	{
		if (memcmp(buf + i, &needle, 4) == 0)
		{
			memcpy(buf + i, &value, 4);
			return;
		}
	}
}

static uint64_t align_up(uint64_t x, uint64_t a) { return (x + a - 1) & ~(a - 1); }
static uint64_t align_down(uint64_t x, uint64_t a) { return x & ~(a - 1); }

// trouve la section .text via la table des sections, sinon NULL
static Elf64_Shdr *find_text_section64(t_woody64 *w)
{
	Elf64_Ehdr *eh = w->ehdr;
	if (eh->e_shoff == 0 || eh->e_shnum == 0)
		return NULL;
	if (eh->e_shstrndx == SHN_UNDEF || eh->e_shstrndx >= eh->e_shnum)
		return NULL;

	Elf64_Shdr *shdrs = (Elf64_Shdr *)((char *)w->offset + eh->e_shoff);
	const char *shstrtab = (const char *)w->offset + shdrs[eh->e_shstrndx].sh_offset;

	for (int i = 0; i < eh->e_shnum; i++)
	{
		if (strcmp(shstrtab + shdrs[i].sh_name, ".text") == 0)
			return &shdrs[i];
	}
	return NULL;
}

// scan des phdrs : segment exec, PT_NOTE a hijacker, fin la plus haute en VMEM
static bool find_segments64(t_woody64 *w, t_segs64 *s)
{
	s->text_phdr = NULL;
	s->note_phdr = NULL;
	s->highest_vaddr_end = 0;

	for (int i = 0; i < w->ehdr->e_phnum; i++)
	{
		Elf64_Phdr *p = &w->phdr[i];

		if (p->p_type == PT_LOAD && (p->p_flags & PF_X) && !s->text_phdr)
			s->text_phdr = p;
		if (p->p_type == PT_NOTE && !s->note_phdr)
			s->note_phdr = p;
		if (p->p_type == PT_LOAD)
		{
			uint64_t end = p->p_vaddr + p->p_memsz;
			if (end > s->highest_vaddr_end)
				s->highest_vaddr_end = end;
		}
	}

	if (!s->text_phdr)
	{
		dprintf(STDERR_FILENO, "Error: no executable PT_LOAD\n");
		return false;
	}
	if (!s->note_phdr)
	{
		dprintf(STDERR_FILENO, "Error: no PT_NOTE to hijack\n");
		return false;
	}
	return true;
}

// calcule les offsets/tailles : zone a chiffrer (.text si dispo, sinon segment
// exec entier), zone mprotect page-alignee, et placement du stub appendu
static void compute_layout64(t_woody64 *w, t_segs64 *s, t_map *map, t_layout64 *l)
{
	Elf64_Shdr *text_shdr = find_text_section64(w);
	uint64_t enc_size_full;

	if (text_shdr)
	{
		l->enc_file_off = text_shdr->sh_offset;
		l->enc_vaddr = text_shdr->sh_addr;
		enc_size_full = text_shdr->sh_size;
	}
	else
	{
		l->enc_file_off = s->text_phdr->p_offset;
		l->enc_vaddr = s->text_phdr->p_vaddr;
		enc_size_full = s->text_phdr->p_filesz;
	}

	// xtea opere par blocs de 8 octets
	l->enc_size = enc_size_full & ~7ULL;
	l->mp_addr = align_down(l->enc_vaddr, PAGE_SIZE);
	l->mp_size = align_up(l->enc_vaddr + l->enc_size, PAGE_SIZE) - l->mp_addr;

	l->stub_file_off = align_up(map->size, PAGE_SIZE);
	l->stub_vaddr = align_up(s->highest_vaddr_end, PAGE_SIZE) + 0x1000000;
	l->out_size = l->stub_file_off + stub_bin_len;
}

// remplace les placeholders du stub. tous les deltas sont relatifs au
// stub_vaddr -> independant de load_bias (PIE)
static void patch_stub64(uint8_t *stub, t_layout64 *l, uint64_t orig_entry, uint32_t *key)
{
	int64_t d_text = (int64_t)l->enc_vaddr - (int64_t)l->stub_vaddr;
	int64_t d_mp = (int64_t)l->mp_addr - (int64_t)l->stub_vaddr;
	int64_t d_entry = (int64_t)orig_entry - (int64_t)l->stub_vaddr;

	patch_u64(stub, stub_bin_len, PH_DELTA_TEXT_A, (uint64_t)d_mp);
	patch_u64(stub, stub_bin_len, PH_MPROTECT_SIZE, l->mp_size);
	patch_u64(stub, stub_bin_len, PH_DELTA_TEXT_B, (uint64_t)d_text);
	patch_u64(stub, stub_bin_len, PH_DECRYPT_SIZE, l->enc_size);
	patch_u64(stub, stub_bin_len, PH_DELTA_ENTRY, (uint64_t)d_entry);
	patch_u32(stub, stub_bin_len, 0xAAAAAAAA, key[0]);
	patch_u32(stub, stub_bin_len, 0xBBBBBBBB, key[1]);
	patch_u32(stub, stub_bin_len, 0xCCCCCCCC, key[2]);
	patch_u32(stub, stub_bin_len, 0xDDDDDDDD, key[3]);
}

// PT_NOTE -> PT_LOAD pointant sur le stub, et redirige e_entry
static void inject_stub_phdr64(uint8_t *out, t_woody64 *w, Elf64_Phdr *note_phdr, t_layout64 *l)
{
	Elf64_Phdr *out_phdrs = (Elf64_Phdr *)(out + w->ehdr->e_phoff);
	Elf64_Phdr *out_note = &out_phdrs[note_phdr - w->phdr];

	out_note->p_type = PT_LOAD;
	out_note->p_flags = PF_R | PF_X;
	out_note->p_offset = l->stub_file_off;
	out_note->p_vaddr = l->stub_vaddr;
	out_note->p_paddr = l->stub_vaddr;
	out_note->p_filesz = stub_bin_len;
	out_note->p_memsz = stub_bin_len;
	out_note->p_align = PAGE_SIZE;

	((Elf64_Ehdr *)out)->e_entry = l->stub_vaddr;
}

// ecrit 'woody' sur disque + affiche la cle si tout est ok
static void write_woody64(uint8_t *out, size_t out_size, uint32_t *key)
{
	int fd = open("woody", O_WRONLY | O_CREAT | O_TRUNC, 0755);
	if (fd < 0)
	{
		dprintf(STDERR_FILENO, "Error: cannot open output file 'woody'\n");
		return;
	}

	ssize_t written = write(fd, out, out_size);
	close(fd);

	if (written != (ssize_t)out_size)
		dprintf(STDERR_FILENO, "Error: short write to 'woody'\n");
	else
		printf("key_value: %08x%08x%08x%08x\n",
			   key[0], key[1], key[2], key[3]);
}

void handle_ELF64(t_map *map)
{
	t_woody64 *w = build_ELF64_data(map);
	if (!w)
		return;
	if (!w->key)
	{
		free_woody64(w);
		return;
	}

	// ET_EXEC (no-pie) et ET_DYN (PIE) supportes : le stub est PIC
	if (w->ehdr->e_type != ET_EXEC && w->ehdr->e_type != ET_DYN)
	{
		dprintf(STDERR_FILENO, "Error: unsupported ELF type (need ET_EXEC or ET_DYN)\n");
		free_woody64(w);
		return;
	}

	t_segs64 segs;
	if (!find_segments64(w, &segs))
	{
		free_woody64(w);
		return;
	}

	t_layout64 lay;
	compute_layout64(w, &segs, map, &lay);

	uint8_t *stub = malloc(stub_bin_len);
	if (!stub)
	{
		free_woody64(w);
		return;
	}
	memcpy(stub, stub_bin, stub_bin_len);
	patch_stub64(stub, &lay, w->ehdr->e_entry, w->key);

	uint8_t *out = calloc(1, lay.out_size);
	if (!out)
	{
		free(stub);
		free_woody64(w);
		return;
	}
	memcpy(out, map->offset, map->size);

	xtea_iter(out + lay.enc_file_off, lay.enc_size, w->key);
	memcpy(out + lay.stub_file_off, stub, stub_bin_len);
	inject_stub_phdr64(out, w, segs.note_phdr, &lay);

	write_woody64(out, lay.out_size, w->key);

	free(out);
	free(stub);
	free_woody64(w);
}