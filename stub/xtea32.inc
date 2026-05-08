%macro XTEA32_BLOCK 0
    mov eax, [esp + 0]              ; eax = kcb0
    mov ebx, [esp + 4]              ; ebx = kcb1
    xor ecx, ecx                    ; ecx = sum = 0
    mov dword [esp + 20], 32        ; rounds = 32

%%r:
    ; ─── kcb0 += (((kcb1 << 4) ^ (kcb1 >> 5)) + kcb1) ^ (sum + key[sum & 3]) ───
    mov edx, ebx                    ; edx = kcb1
    shl edx, 4                      ; edx = kcb1 << 4
    mov [esp + 24], ebx             ; tmp1 = kcb1
    shr dword [esp + 24], 5         ; tmp1 = kcb1 >> 5
    xor edx, [esp + 24]             ; edx = (kcb1<<4) ^ (kcb1>>5)
    add edx, ebx                    ; edx += kcb1                     [left]

    mov [esp + 28], edx             ; tmp2 = left

    mov edx, ecx                    ; edx = sum
    and edx, 3                      ; edx = sum & 3
    mov edx, [esi + edx*4]          ; edx = key[sum & 3]
    add edx, ecx                    ; edx = sum + key[sum & 3]        [right]

    xor edx, [esp + 28]             ; edx = left ^ right
    add eax, edx                    ; kcb0 += edx

    ; ─── sum += delta ───
    add ecx, 0x9E3779B9

    ; ─── kcb1 += (((kcb0 << 4) ^ (kcb0 >> 5)) + kcb0) ^ (sum + key[(sum >> 11) & 3]) ───
    mov edx, eax                    ; edx = kcb0
    shl edx, 4                      ; edx = kcb0 << 4
    mov [esp + 24], eax             ; tmp1 = kcb0
    shr dword [esp + 24], 5         ; tmp1 = kcb0 >> 5
    xor edx, [esp + 24]             ; edx = (kcb0<<4) ^ (kcb0>>5)
    add edx, eax                    ; edx += kcb0                     [left]

    mov [esp + 28], edx             ; tmp2 = left

    mov edx, ecx                    ; edx = sum
    shr edx, 11
    and edx, 3                      ; edx = (sum >> 11) & 3
    mov edx, [esi + edx*4]          ; edx = key[(sum>>11) & 3]
    add edx, ecx                    ; edx = sum + key[...]            [right]

    xor edx, [esp + 28]             ; edx = left ^ right
    add ebx, edx                    ; kcb1 += edx

    ; ─── boucle ───
    dec dword [esp + 20]
    jnz %%r

    mov [esp + 0], eax              ; block[0] = kcb0
    mov [esp + 4], ebx              ; block[1] = kcb1
%endmacro
