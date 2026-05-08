%include "xtea.inc"

section .text
global _start

_start:
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ; ─── stub_runtime base via lea RIP-relative (PIC) ───
    ; rbx = adresse runtime du stub. utilise sous PIE/ASLR pour
    ; reconstruire les addresses absolues a partir d'offsets relatifs.
    lea rbx, [rel _start]

    ; ─── write(1, string, 14) ───
    mov rax, 1                          ; syscall write
    mov rdi, 1                          ; fd = stdout
    lea rsi, [rel string]
    mov rdx, 14
    syscall

    ; ─── mprotect(text_addr, text_size, RWX) ───
    mov rax, 10                         ; syscall mprotect
    mov rdi, rbx                        ; rdi = stub_runtime
    mov rcx, 0x1111111111111111         ; placeholder: text_vaddr - stub_vaddr (signed)
    add rdi, rcx                        ; rdi = text_runtime
    mov rsi, 0x2222222222222222         ; placeholder: mprotect_size
    mov rdx, 7                          ; PROT_READ | WRITE | EXEC
    syscall

    ; ─── Setup boucle CTR ───
    sub rsp, 16                         ; add place sur la stack pour les operation
    mov r12, rbx                        ; r12 = stub_runtime
    mov rcx, 0x4444444444444444         ; placeholder: text_vaddr - stub_vaddr (deja meme valeur)
    add r12, rcx                        ; r12 = text_runtime (decrypt)
    mov r13, 0x5555555555555555         ; placeholder text_size
    add r13, r12                        ; r13 = pointeur de fin
    xor r15, r15                        ; counter = 0
    lea r14, [rel key]                  ; r14 = pointeur sur la clé

.loop:
    cmp r12, r13
    jae .done                           ; jae = unsigned, safe sous PIE

    ; ─── block[0..3] = counter low, block[4..7] = counter high ───
    mov [rsp], r15d         ; r15d prend que a droite, block[0..3] = counter low (32 bits bas du counter)
    mov rax, r15            ; rax = counter (copie 64 bits)
    shr rax, 32             ; rax = counter >> 32 (décalage, garde les 32 bits hauts)
    mov [rsp + 4], eax      ; block[4..7] = counter high (32 bits hauts du counter)

    ; ─── XTEA(block, key) → keystream ───
    XTEA_BLOCK

    ; ─── data[i..i+7] ^= keystream ───
    mov rax, [rsp]
    xor [r12], rax

    add r12, 8                          ; avancer pointeur
    inc r15                             ; counter++
    jmp .loop

.done:
    add rsp, 16                         ; libérer scratch
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    ; ─── Saut vers l'entry original (PIE-aware) ───
    mov rax, 0x3333333333333333         ; placeholder: orig_entry - stub_vaddr (signed)
    lea rcx, [rel _start]               ; rcx = stub_runtime
    add rax, rcx                        ; rax = entry_runtime
    jmp rax

string: db "....WOODY....", 0x0A

key:
    dd 0xAAAAAAAA  ; placeholder aussi
    dd 0xBBBBBBBB
    dd 0xCCCCCCCC
    dd 0xDDDDDDDD

section .note.GNU-stack noalloc noexec nowrite progbits
