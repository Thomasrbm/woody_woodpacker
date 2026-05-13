# Woody Woodpacker

A self-decrypting ELF binary packer for Linux. Woody Woodpacker takes an
existing ELF executable, encrypts its code section with a randomly generated
key, and prepends a small assembly stub that transparently decrypts the
original code back into memory at runtime — producing a new binary, `woody`,
that behaves identically to the original.

The project is a 42 school exercise focused on the low-level ELF format,
program loading, and position-independent shellcode.

---

## Features

- **ELF32 and ELF64** — both 32-bit and 64-bit Linux executables are supported.
- **ET_EXEC and ET_DYN** — works on classical static binaries as well as
  PIE binaries. The stub is fully position-independent.
- **XTEA encryption** — the `.text` section (or the executable segment as a
  fallback) is encrypted in counter mode with a 128-bit key read from
  `/dev/urandom`.
- **PT_NOTE → PT_LOAD hijacking** — the stub is appended to the file and
  loaded via an unused `PT_NOTE` program header repurposed as `PT_LOAD`,
  avoiding any change to the existing segment layout.
- **Runtime `mprotect`** — the stub re-maps the encrypted region as writable
  before decryption, then restores the original page permissions before
  jumping to the original entry point.

---

## Build

The project has no runtime dependencies. Building requires only `gcc`, GNU
`make`, `nasm`, `objcopy`, and `xxd`.

```sh
make
```

The build pipeline assembles the two stubs (`stub/stub.s` for x86-64 and
`stub/stub32.s` for i386) with NASM, extracts the raw `.text` section with
`objcopy`, and converts each blob into a C header via `xxd -i`. The resulting
arrays are linked directly into the packer.

Standard targets are available:

| Target | Description |
| ------ | ----------- |
| `make` / `make all` | Build the `woody_woodpacker` binary. |
| `make clean` | Remove intermediate objects. |
| `make fclean` | Remove objects, the packer, and any generated `woody`. |
| `make re` | Full rebuild. |

---

## Usage

```sh
./woody_woodpacker <ELF32 or ELF64 binary>
```

On success, the packer writes a new file named `woody` in the current
directory and prints the encryption key on stdout:

```sh
$ ./woody_woodpacker /bin/ls
key_value: 9f3a1c4d2e7b88016a4cdc2f51e83a90
$ ./woody
# ... runs exactly like /bin/ls
```

The output file is marked executable (mode `0755`) and can be run directly.

---

## How it works

The packer performs the following steps:

1. **Map the input.** The target binary is opened read-only and mapped into
   memory with `mmap`. The ELF class (`EI_CLASS`) is checked to dispatch
   between the 32-bit and 64-bit code paths.
2. **Generate a key.** Four 32-bit words are read from `/dev/urandom` to form
   the XTEA key.
3. **Locate the code.** The section table is parsed to find `.text`. If the
   section table is unavailable, the first executable `PT_LOAD` is used as a
   fallback. The encryption range is rounded down to a multiple of the XTEA
   block size (8 bytes).
4. **Compute the new layout.** The stub is placed at a page-aligned file
   offset past the end of the original file, and at a virtual address well
   above the highest existing `PT_LOAD`, ensuring no overlap with the
   original mapping.
5. **Patch the stub.** Five sentinel placeholders inside the stub are
   replaced with the actual deltas (encrypted region, `mprotect` region,
   original entry point) and the four key words. All deltas are computed
   relative to the stub's own virtual address, which keeps the stub
   position-independent under PIE/ASLR.
6. **Encrypt.** The code region is encrypted with XTEA in CTR mode, where the
   counter is the block index. CTR mode lets the decrypt loop run forward
   over the data without keeping intermediate state.
7. **Inject.** The unused `PT_NOTE` program header is rewritten as a new
   `PT_LOAD` segment (`PF_R | PF_X`) pointing at the appended stub, and the
   ELF header's `e_entry` is redirected to the stub's virtual address.
8. **Write.** The new image is written to `./woody`.

At runtime, the stub:

1. Calls `mprotect` to make the encrypted page range writable.
2. Decrypts the code in place with the same XTEA-CTR routine.
3. Calls `mprotect` again to restore `R-X` permissions.
4. Jumps to the saved original entry point.

---

## Project layout

```
.
├── Makefile
├── includes/
│   └── woody.h               # Public types, placeholders, prototypes
├── srcs/
│   ├── main.c                # CLI, mmap, key generation, dispatch
│   ├── ELF32.c               # 32-bit pipeline
│   ├── ELF64.c               # 64-bit pipeline
│   └── xtea.c                # XTEA-CTR encryption routine
├── stub/
│   ├── stub.s                # x86-64 decrypt stub (NASM)
│   ├── stub32.s              # i386 decrypt stub (NASM)
│   ├── xtea.inc              # 64-bit XTEA implementation (shared with C)
│   └── xtea32.inc            # 32-bit XTEA implementation
└── DOC/                      # Development notes
```

---

## Constraints and limitations

- Linux ELF binaries only. Mach-O and PE are not handled.
- The host binary must contain a `PT_NOTE` program header (true for the vast
  majority of toolchain-produced ELFs).
- Statically stripped binaries with no section table fall back to encrypting
  the full executable segment, which is conservative but functional.
- The packer is a school exercise: it is *not* designed to evade modern
  anti-malware, anti-debug, or static analysis tooling.

---

## License

Educational project — no warranty. Use on binaries you own or have permission
to modify.
