


%macro XTEA_BLOCK 0
    mov eax,[rsp]
    mov ebx,[rsp+4]
    xor ecx,ecx
    mov r8d,32
    mov r9d,0x9E3779B9

%%r:mov edx,ebx
    shl edx,4
    mov r10d,ebx
    shr r10d,5
    xor edx,r10d
    add edx,ebx
    mov r10d,ecx
    and r10d,3
    mov r11d,[r14+r10*4]
    add r11d,ecx
    xor edx,r11d
    add eax,edx
    add ecx,r9d
    mov edx,eax
    shl edx,4
    mov r10d,eax
    shr r10d,5
    xor edx,r10d
    add edx,eax
    mov r10d,ecx
    shr r10d,11
    and r10d,3
    mov r11d,[r14+r10*4]
    add r11d,ecx
    xor edx,r11d
    add ebx,edx
    dec r8d
    jnz %%r
    mov [rsp],eax
    mov [rsp+4],ebx
%endmacro


