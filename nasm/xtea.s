%include "xtea.inc"

global xtea_encrypt_block
xtea_encrypt_block:
    push rbx
    push r14
    mov r14, rsi      ; key dans r14 pour la macro
    sub rsp, 8
    
    mov eax, [rdi]
    mov [rsp], eax
    mov eax, [rdi+4]
    mov [rsp+4], eax
    
    XTEA_BLOCK        ; expand la macro
    
    mov eax, [rsp]
    mov [rdi], eax
    mov eax, [rsp+4]
    mov [rdi+4], eax
    
    add rsp, 8
    pop r14
    pop rbx
    ret