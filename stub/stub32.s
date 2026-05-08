%include "xtea32.inc"

section .text
global _start

_start:
    pushad

    ; ─── stub_runtime base via call/pop (PIC) ───
    ; en 32-bit on a pas de [rip] alors on utilise call/pop pour
    ; recuperer eip courant, puis on retire l'offset depuis _start.
    call .get_pc
.get_pc:
    pop ebp                                 ; ebp = adresse runtime de .get_pc
    sub ebp, .get_pc - _start               ; ebp = stub_runtime base

    ; ─── write(1, string, 14) ───
    mov eax, 4                              ; sys_write
    mov ebx, 1                              ; fd = stdout
    lea ecx, [ebp + (string - _start)]      ; buf = string runtime addr
    mov edx, 14                             ; count
    int 0x80

    ; ─── mprotect(text_addr, text_size, RWX) ───
    mov eax, 125                            ; sys_mprotect
    lea ebx, [ebp + 0x11111111]             ; placeholder: mp_addr - stub_vaddr (signed)
    mov ecx, 0x22222222                     ; placeholder: mprotect_size
    mov edx, 7                              ; PROT_READ | WRITE | EXEC
    int 0x80

    ; ─── setup boucle CTR ───
    sub esp, 32                             ; reserve scratch frame (32 octets)
    mov dword [esp + 8], 0                  ; counter low = 0
    mov dword [esp + 12], 0                 ; counter high = 0
    lea edi, [ebp + 0x44444444]             ; placeholder: text_vaddr - stub_vaddr (decrypt)
    mov eax, 0x55555555                     ; placeholder: text_size (multiple de 8)
    add eax, edi                            ; eax = end ptr
    mov [esp + 16], eax                     ; sauvegarde end ptr
    lea esi, [ebp + (key - _start)]         ; esi = key runtime addr

.loop:
    cmp edi, [esp + 16]
    jae .done                               ; jae: unsigned compare

    ; block[0..3] = counter low, block[4..7] = counter high
    mov eax, [esp + 8]
    mov [esp + 0], eax
    mov eax, [esp + 12]
    mov [esp + 4], eax

    ; XTEA(block, key) → keystream
    XTEA32_BLOCK

    ; data[edi..edi+7] ^= keystream
    mov eax, [esp + 0]
    xor [edi], eax
    mov eax, [esp + 4]
    xor [edi + 4], eax

    ; counter++ (64-bit add via add+adc)
    add dword [esp + 8], 1
    adc dword [esp + 12], 0

    add edi, 8                              ; avancer pointeur
    jmp .loop

.done:
    add esp, 32                             ; libere scratch frame

    ; ─── compute entry runtime tant que ebp est encore stub_runtime ───
    lea eax, [ebp + 0x33333333]             ; placeholder: orig_entry - stub_vaddr (signed)
    mov [esp + 28], eax                     ; ecrase saved-eax dans la zone pushad

    popad                                   ; restore tous les regs (eax = entry_runtime)
    jmp eax

string: db "....WOODY....", 0x0A

key:
    dd 0xAAAAAAAA
    dd 0xBBBBBBBB
    dd 0xCCCCCCCC
    dd 0xDDDDDDDD

section .note.GNU-stack noalloc noexec nowrite progbits
