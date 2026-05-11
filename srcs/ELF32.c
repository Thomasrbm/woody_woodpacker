#include "woody.h"
#include "stub32.h"

// placeholders 4 octets dans le stub32
#define PH32_DELTA_TEXT_A 0x11111111U // mp_addr   - stub_vaddr (mprotect)
#define PH32_MPROTECT_SIZE 0x22222222U
#define PH32_DELTA_ENTRY 0x33333333U  // orig_entry - stub_vaddr
#define PH32_DELTA_TEXT_B 0x44444444U // text_vaddr - stub_vaddr (decrypt)
#define PH32_DECRYPT_SIZE 0x55555555U

#define PAGE_SIZE_32 0x1000U

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

// ─── verif du format ELF + alloc des donnees ──────────────────────────────

bool is_ELF32(Elf32_Ehdr *header)
{
	if (!header)
		return false;

	// ELFMAG : magic ELF, SELFMAG : sa longueur
	if (memcmp(header->e_ident, ELFMAG, SELFMAG) != 0)
	{
		dprintf(STDERR_FILENO, "Error: The binary must have an ELF format\n");
		return false;
	}

	if (header->e_ident[EI_CLASS] != ELFCLASS32)
		return false;

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

static void free_woody32(t_woody32 *w)
{
	if (!w)
		return;
	free(w->key);
	free(w);
}

// ─── helpers bas-niveau ────────────────────────────────────────────────────

static void patch_u32_32(uint8_t *buf, size_t len, uint32_t needle, uint32_t value)
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

static uint32_t align_up_32(uint32_t x, uint32_t a) { return (x + a - 1) & ~(a - 1); }
static uint32_t align_down_32(uint32_t x, uint32_t a) { return x & ~(a - 1); }

static Elf32_Shdr *find_text_section32(t_woody32 *w)
{
	Elf32_Ehdr *eh = w->ehdr;
	if (eh->e_shoff == 0 || eh->e_shnum == 0)
		return NULL;
	if (eh->e_shstrndx == SHN_UNDEF || eh->e_shstrndx >= eh->e_shnum)
		return NULL;

	Elf32_Shdr *shdrs = (Elf32_Shdr *)((char *)w->offset + eh->e_shoff);
	const char *shstrtab = (const char *)w->offset + shdrs[eh->e_shstrndx].sh_offset;

	for (int i = 0; i < eh->e_shnum; i++)
	{
		if (strcmp(shstrtab + shdrs[i].sh_name, ".text") == 0)
			return &shdrs[i];
	}
	return NULL;
}

// ─── etapes du packing ─────────────────────────────────────────────────────

static bool find_segments32(t_woody32 *w, t_segs32 *s)
{
	s->text_phdr = NULL;
	s->note_phdr = NULL;
	s->highest_vaddr_end = 0;

	for (int i = 0; i < w->ehdr->e_phnum; i++)
	{
		Elf32_Phdr *p = &w->phdr[i];

		if (p->p_type == PT_LOAD && (p->p_flags & PF_X) && !s->text_phdr)
			s->text_phdr = p;
		if (p->p_type == PT_NOTE && !s->note_phdr)
			s->note_phdr = p;
		if (p->p_type == PT_LOAD)
		{
			uint32_t end = p->p_vaddr + p->p_memsz;
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

static void compute_layout32(t_woody32 *w, t_segs32 *s, t_map *map, t_layout32 *l)
{
	Elf32_Shdr *text_shdr = find_text_section32(w);
	uint32_t enc_size_full;

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

	l->enc_size = enc_size_full & ~7U;
	l->mp_addr = align_down_32(l->enc_vaddr, PAGE_SIZE_32);
	l->mp_size = align_up_32(l->enc_vaddr + l->enc_size, PAGE_SIZE_32) - l->mp_addr;

	l->stub_file_off = align_up_32(map->size, PAGE_SIZE_32);
	l->stub_vaddr = align_up_32(s->highest_vaddr_end, PAGE_SIZE_32) + 0x100000;
	l->out_size = l->stub_file_off + stub32_bin_len;
}

static void patch_stub32(uint8_t *stub, t_layout32 *l, uint32_t orig_entry, uint32_t *key)
{
	int32_t d_text = (int32_t)l->enc_vaddr - (int32_t)l->stub_vaddr;
	int32_t d_mp = (int32_t)l->mp_addr - (int32_t)l->stub_vaddr;
	int32_t d_entry = (int32_t)orig_entry - (int32_t)l->stub_vaddr;

	patch_u32_32(stub, stub32_bin_len, PH32_DELTA_TEXT_A, (uint32_t)d_mp);
	patch_u32_32(stub, stub32_bin_len, PH32_MPROTECT_SIZE, l->mp_size);
	patch_u32_32(stub, stub32_bin_len, PH32_DELTA_TEXT_B, (uint32_t)d_text);
	patch_u32_32(stub, stub32_bin_len, PH32_DECRYPT_SIZE, l->enc_size);
	patch_u32_32(stub, stub32_bin_len, PH32_DELTA_ENTRY, (uint32_t)d_entry);
	patch_u32_32(stub, stub32_bin_len, 0xAAAAAAAA, key[0]);
	patch_u32_32(stub, stub32_bin_len, 0xBBBBBBBB, key[1]);
	patch_u32_32(stub, stub32_bin_len, 0xCCCCCCCC, key[2]);
	patch_u32_32(stub, stub32_bin_len, 0xDDDDDDDD, key[3]);
}

static void inject_stub_phdr32(uint8_t *out, t_woody32 *w, Elf32_Phdr *note_phdr, t_layout32 *l)
{
	Elf32_Phdr *out_phdrs = (Elf32_Phdr *)(out + w->ehdr->e_phoff);
	Elf32_Phdr *out_note = &out_phdrs[note_phdr - w->phdr];

	out_note->p_type = PT_LOAD;
	out_note->p_flags = PF_R | PF_X;
	out_note->p_offset = l->stub_file_off;
	out_note->p_vaddr = l->stub_vaddr;
	out_note->p_paddr = l->stub_vaddr;
	out_note->p_filesz = stub32_bin_len;
	out_note->p_memsz = stub32_bin_len;
	out_note->p_align = PAGE_SIZE_32;

	((Elf32_Ehdr *)out)->e_entry = l->stub_vaddr;
}

static void write_woody32(uint8_t *out, size_t out_size, uint32_t *key)
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

// ─── orchestration ─────────────────────────────────────────────────────────

void handle_ELF32(t_map *map)
{
	t_woody32 *w = build_ELF32_data(map);
	if (!w)
		return;
	if (!w->key)
	{
		free_woody32(w);
		return;
	}

	// ET_EXEC et ET_DYN supportes : le stub32 est PIC (call/pop)
	if (w->ehdr->e_type != ET_EXEC && w->ehdr->e_type != ET_DYN)
	{
		dprintf(STDERR_FILENO, "Error: unsupported ELF type (need ET_EXEC or ET_DYN)\n");
		free_woody32(w);
		return;
	}

	t_segs32 segs;
	if (!find_segments32(w, &segs))
	{
		free_woody32(w);
		return;
	}

	t_layout32 lay;
	compute_layout32(w, &segs, map, &lay);

	uint8_t *stub = malloc(stub32_bin_len);
	if (!stub)
	{
		free_woody32(w);
		return;
	}
	memcpy(stub, stub32_bin, stub32_bin_len);
	patch_stub32(stub, &lay, w->ehdr->e_entry, w->key);

	uint8_t *out = calloc(1, lay.out_size);
	if (!out)
	{
		free(stub);
		free_woody32(w);
		return;
	}
	memcpy(out, map->offset, map->size);

	xtea_iter(out + lay.enc_file_off, lay.enc_size, w->key);
	memcpy(out + lay.stub_file_off, stub, stub32_bin_len);
	inject_stub_phdr32(out, w, segs.note_phdr, &lay);

	write_woody32(out, lay.out_size, w->key);

	free(out);
	free(stub);
	free_woody32(w);
}