;  main modif le e_entry du programme a exec pour lui donner l adresse de 
; ce programme en assembleur qui va s exec


; donc rsp point sur argc suivi de argv[], envp[]
; reste est indefini

; jamais push rsp mais push tout les autres 
; pour que le programme reprennet avec tout comme a valeur d origine


section .data
    string: db "....WOODY....", 0x0A


section .text
    global _start   


_start:
    ; 1. SAUVEGARDE des registres (push)
    push rax
    push rdi
    push rsi 
    push rdx
    push r9

    mov rax, 1
    mov rdi, 1 ;fd
    lea rsi, [rel string]
    mov rdx, 14
    syscall

    cmp rax, 0

    mov rax, 10
    mov rdi, 0x1111111111111111
    mov rsi, 0x2222222222222222
    mov rdx, 7                        ; perms = R|W|X = 1|2|4 = 7


    pop rax
    pop rdi
    pop rsi
    pop rdx
    pop r9    

    ; 4. BOUCLE DE DECHIFFREMENT XTEA
    ;    Pour chaque bloc de 8 bytes de .text :
    ;      - chiffrer le compteur avec XTEA → keystream
    ;      - XOR le bloc avec keystream
    ;      - incrémenter le compteur
        
    mov rax, 0x3333333333333333   ; placeholder original_entry
    jmp rax


    

section .note.GNU-stack noalloc noexec nowrite progbits
