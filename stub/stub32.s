%include "xtea32.inc"

section .text
    global _start

_start:
    push eax
    push ebx
    push ecx
    push edx
    push esi
    push edi
    push ebp; (r8..r15 n'existent pas en 32-bit, on push juste ebp en plus)


    ; on peut pas lea pour l entry point car eip = pas accessible ici.
    ; "lea rbx, [rel _start]". 32 = pas acces au EIP (ou sait pas ou on est dans la ram)
    call .get_pc  ; push l adreese de la ft ex : 0x56550015
.get_pc:
    pop ebp ; recup adresse et 
    sub ebp, .get_pc - _start ; calcul l offset = adresse entry point.




    ; ─── write(1, string, 14) ───
    mov eax, 4                      ; pas les meme syscall order
    mov ebx, 1
    lea ecx, [ebp + (string - _start)]      ;  ebp = 0x56550000   et string = 187 - start a 0 = 187                           
    mov edx, 14                             ;  mov rdx, 14
    int 0x80    ; pas syscall donc int = interrupt,  switch mode kernel  ET syscall .  int 0x00 c est divi par zero etc etc.    

    ; ─── mprotect(text_addr, text_size, RWX) ───
    mov eax, 125                            ;  mov rax, 10  (sys_mprotect)
    mov ebx, ebp                            ;  adresse runtime cad  le binaire lui meme qu on modif pour en faire un woody
    mov esi, 0x11111111
    add ebx, esi
    mov ecx, 0x22222222                     ;  mov rsi, 0x222... (ecx = mprotect_size en 32-bit)
    mov edx, 7                              ;  mov rdx, 7
    int 0x80                                ;  syscall

    ; ─── Setup boucle CTR ───
    sub esp, 32                             ;  sub rsp, 16 (XTEA32 a besoin de 32 octets)
    mov edi, ebp                            ;  mov r12, rbx
    mov ecx, 0x44444444                     ;  mov rcx, 0x444...
    add edi, ecx                            ;  add r12, rcx
    mov eax, 0x55555555                     ;  mov r13, 0x555... (eax car pas de 8eme reg dispo)
    add eax, edi                            ;  add r13, r12
    mov [esp + 16], eax                     ; (irreductible: end_ptr sur stack, aucun reg preserve par XTEA dispo)
    mov dword [esp + 8], 0                  ;  xor r15, r15 (counter low — counter 64-bit en 2 mots)
    mov dword [esp + 12], 0                 ;                     (counter high)
    lea esi, [ebp + (key - _start)]         ;  lea r14, [rel key]

.loop:
    cmp edi, [esp + 16]                     ;  cmp r12, r13 (end_ptr est sur stack en 32-bit)
    jae .done                               ;  jae .done



    ; ─── block[0..3] = counter low, block[4..7] = counter high ───
    mov eax, [esp + 8]                      ;  mov [rsp], r15d
    mov [esp + 0], eax
    mov eax, [esp + 12]                     ;  mov rax, r15 / shr rax, 32 / mov [rsp+4], eax
    mov [esp + 4], eax

    ; ─── XTEA(block, key) → keystream ───
    XTEA32_BLOCK

    ; ─── data[i..i+7] ^= keystream ───
    mov eax, [esp + 0]                      ;  mov rax, [rsp]
    xor [edi], eax                          ;  xor [r12], rax (en 2 xor 32-bit, irreductible)
    mov eax, [esp + 4]
    xor [edi + 4], eax

    add edi, 8                              ;  add r12, 8
    add dword [esp + 8], 1                  ;  inc r15 (64-bit en add+adc, irreductible)
    adc dword [esp + 12], 0
    jmp .loop




.done:
    add esp, 32
    pop ebp                                 
    pop edi                                 
    pop esi                                 
    pop edx
    pop ecx                                 
    pop ebx                                 
    pop eax

    ; ─── Saut vers l'entry original (PIE-aware) ───
    mov eax, 0x33333333                     ;  mov rax, 0x333... (orig_entry offset)
    call .get_pc2                           ;  lea rcx, [rel _start] — call/pop a la place
.get_pc2:
    pop ecx
    sub ecx, .get_pc2 - _start              ; ecx = stub_runtime
    add eax, ecx                            ;  add rax, rcx
    jmp eax                                 ;  jmp rax

string: db "....WOODY....", 0x0A

key:
    dd 0xAAAAAAAA
    dd 0xBBBBBBBB
    dd 0xCCCCCCCC
    dd 0xDDDDDDDD

section .note.GNU-stack noalloc noexec nowrite progbits
