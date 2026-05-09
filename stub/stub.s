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

    lea rbx, [rel _start] ; start point de stub


    ;  (PAGE_SIZE memoire = 4096) :
    ;   .text                 = 1000
    ;   .text size              = 4000
    ;   .text entry point      = 1030
    ;   stub entry point        = 5000
    ;
    ;   mp_addr = align_down(1000, 4096) = 0
    ;   mp_size = align_up(1000+4000, 4096) - 0 = align_up(5000, 4096) = 8192
    ;
    ; Placeholders patches par le packer (cf. srcs/ELF64.c) :
    ;
    ;   0x1111111111111111 = mp_addr    - stub =    0 - 5000         = -5000   (delta mprotect, page-aligned)    => debut page memoire
    ;   0x2222222222222222 = mp_size                                 =  8192   (taille mprotect, arrondie page)  => fin page memoire
    ;   0x3333333333333333 = .text entry point  - stub = 1030 - 5000 = -3970   (delta vers entry original)       => entry de sample
    ;   0x4444444444444444 = .text  - stub = 1000 - 5000             = -4000   (delta .text et stub, pour XTEA)  => adresse de .text
    ;   0x5555555555555555 = size encrypted                          =  4000   (taille .text, arrondie 8 octets) => taille positive.

    ; ─── write(1, string, 14) ───
    mov rax, 1                          ; syscall write
    mov rdi, 1                          ; fd = stdout
    lea rsi, [rel string]
    mov rdx, 14
    syscall

    ; ─── mprotect(text_addr, text_size, RWX) ───  ========> va mettre le vrai .text du pack en modifiable pdt l exec. 
    mov rax, 10                         ; syscall mprotect
    mov rdi, rbx                        ; rdi = stub_runtime,  adresse runtime cad  le binaire lui meme qu on modif pour en faire un woody
    mov rcx, 0x1111111111111111         ; debut de la page memoire pour tout modif
    add rdi, rcx                        ; [...header...] [.text CHIFFRÉ du vrai code] [...autres sections...] [STUB injecté]     en realite rcx est negatif donc on recul a la section .text
    mov rsi, 0x2222222222222222         ; fin de la page (size aussi) 
    mov rdx, 7                          ; PROT_READ | WRITE | EXEC
    syscall

    ; ─── Setup boucle CTR ─── void xtea_iter(uint8_t *data, size_t len, const uint32_t key[4])
    sub rsp, 16                         ; place pour operation sur stack, car certainnes en ont besoin  (counter low et hith mov [rsp], r15d    etc)
    mov r12, rbx                        ; ptr ver entry point stub (_start)
    mov rcx, 0x4444444444444444         ; offset negatif pour aller a  .text sample
    add r12, rcx                        ; ptr maj vers zone a desecnrypter .text  (sub) 
    mov r13, 0x5555555555555555         ; taille du code .text a desencrypter. 
    add r13, r12                        ; r13 = pointeur de fin de  section .txt 
    xor r15, r15                        ; counter = 0
    lea r14, [rel key]                  ; ptr sur la clé

.loop:
    cmp r12, r13                    ; arrive a la fin ? 
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
    add rsp, 16
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
    mov rax, 0x3333333333333333         ; rax a entry point de .text de sample (offset -4000)
    lea rcx, [rel _start]               ; recup entry point _start
    add rax, rcx                        ; maj rax a entry 
    jmp rax                             ; retourne a entry point reel

string: db "....WOODY....", 0x0A

key:
    dd 0xAAAAAAAA  ; placeholder aussi
    dd 0xBBBBBBBB
    dd 0xCCCCCCCC
    dd 0xDDDDDDDD

section .note.GNU-stack noalloc noexec nowrite progbits
