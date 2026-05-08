%macro XTEA_BLOCK 0
    ; ─── Init ───
    mov eax, [rsp]              ; kcb0 = block[0]
    mov ebx, [rsp + 4]           ; kcb1 = block[1]
    xor ecx, ecx                 ; sum = 0
    mov r8d, 32                  ; rounds = 32
    mov r9d, 0x9E3779B9          ; delta

%%r:
    ; ─── kcb0 += (((kcb1 << 4) ^ (kcb1 >> 5)) + kcb1) ^ (sum + key[sum & 3]) ───
    mov edx, ebx                 ; edx = kcb1
    shl edx, 4                   ; edx = kcb1 << 4
    mov r10d, ebx                ; r10 = kcb1
    shr r10d, 5                  ; r10 = kcb1 >> 5
    xor edx, r10d                ; edx = (kcb1 << 4) ^ (kcb1 >> 5)
    add edx, ebx                 ; edx += kcb1                     [left]

    mov r10d, ecx                ; r10 = sum
    and r10d, 3                  ; r10 = sum & 3
    mov r11d, [r14 + r10*4]      ; r11 = key[sum & 3]
    add r11d, ecx                ; r11 = sum + key[sum & 3]      [right]

    xor edx, r11d                ; edx = left ^ right
    add eax, edx                 ; kcb0 += edx

    ; ─── sum += delta ───
    add ecx, r9d

    ; ─── kcb1 += (((kcb0 << 4) ^ (kcb0 >> 5)) + kcb0) ^ (sum + key[(sum >> 11) & 3]) ───
    mov edx, eax                 ; edx = kcb0
    shl edx, 4                   ; edx = kcb0 << 4
    mov r10d, eax                ; r10 = kcb0
    shr r10d, 5                  ; r10 = kcb0 >> 5
    xor edx, r10d                ; edx = (kcb0 << 4) ^ (kcb0 >> 5)
    add edx, eax                 ; edx += kcb0                     [left]

    mov r10d, ecx                ; r10 = sum
    shr r10d, 11                 ; r10 = sum >> 11
    and r10d, 3                  ; r10 = (sum >> 11) & 3
    mov r11d, [r14 + r10*4]      ; r11 = key[(sum >> 11) & 3]
    add r11d, ecx                ; r11 = sum + key[...]          [right]

    xor edx, r11d                ; edx = left ^ right
    add ebx, edx                 ; kcb1 += edx

    ; ─── Loop control ───
    dec r8d
    jnz %%r

    ; ─── Save back ───
    mov [rsp], eax               ; block[0] = kcb0
    mov [rsp + 4], ebx           ; block[1] = kcb1
%endmacro