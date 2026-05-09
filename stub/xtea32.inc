%macro XTEA32_BLOCK 0
    ; ─── Init ───
    mov eax, [esp + 0]                      ;  mov eax, [rsp]               (kcb0 = block[0])
    mov ebx, [esp + 4]                      ;  mov ebx, [rsp + 4]           (kcb1 = block[1])
    xor ecx, ecx                            ;  xor ecx, ecx                 (sum = 0)
    mov dword [esp + 20], 32                ;  mov r8d, 32                  (rounds — irreductible: pas de r8d, sur stack)
                                            ;  mov r9d, 0x9E3779B9          (delta — irreductible: pas de r9d, inline plus bas)

%%r:
    ; ─── kcb0 += (((kcb1 << 4) ^ (kcb1 >> 5)) + kcb1) ^ (sum + key[sum & 3]) ───
    mov edx, ebx                            ;  mov edx, ebx                 (edx = kcb1)
    shl edx, 4                              ;  shl edx, 4                   (edx = kcb1 << 4)
    mov [esp + 24], ebx                     ;  mov r10d, ebx                (tmp1 sur stack ↔ r10d)
    shr dword [esp + 24], 5                 ;  shr r10d, 5                  (tmp1 = kcb1 >> 5)
    xor edx, [esp + 24]                     ;  xor edx, r10d                (edx = (kcb1<<4) ^ (kcb1>>5))
    add edx, ebx                            ;  add edx, ebx                 (edx += kcb1            [left])

    mov [esp + 28], edx                     ; (irreductible: preserve left sur stack, edx reaffecté pour key lookup)

    mov edx, ecx                            ;  mov r10d, ecx                (edx = sum)
    and edx, 3                              ;  and r10d, 3                  (edx = sum & 3)
    mov edx, [esi + edx*4]                  ;  mov r11d, [r14 + r10*4]      (edx = key[sum & 3])
    add edx, ecx                            ;  add r11d, ecx                (edx = sum + key[sum & 3]  [right])

    xor edx, [esp + 28]                     ;  xor edx, r11d                (edx = left ^ right)
    add eax, edx                            ;  add eax, edx                 (kcb0 += edx)

    ; ─── sum += delta ───
    add ecx, 0x9E3779B9                     ;  add ecx, r9d                 (delta inline: r9d n'existe pas en 32-bit)

    ; ─── kcb1 += (((kcb0 << 4) ^ (kcb0 >> 5)) + kcb0) ^ (sum + key[(sum >> 11) & 3]) ───
    mov edx, eax                            ;  mov edx, eax                 (edx = kcb0)
    shl edx, 4                              ;  shl edx, 4                   (edx = kcb0 << 4)
    mov [esp + 24], eax                     ;  mov r10d, eax                (tmp1 sur stack ↔ r10d)
    shr dword [esp + 24], 5                 ;  shr r10d, 5                  (tmp1 = kcb0 >> 5)
    xor edx, [esp + 24]                     ;  xor edx, r10d                (edx = (kcb0<<4) ^ (kcb0>>5))
    add edx, eax                            ;  add edx, eax                 (edx += kcb0            [left])

    mov [esp + 28], edx                     ; (irreductible: preserve left avant key lookup)

    mov edx, ecx                            ;  mov r10d, ecx                (edx = sum)
    shr edx, 11                             ;  shr r10d, 11
    and edx, 3                              ;  and r10d, 3                  (edx = (sum >> 11) & 3)
    mov edx, [esi + edx*4]                  ;  mov r11d, [r14 + r10*4]      (edx = key[(sum>>11) & 3])
    add edx, ecx                            ;  add r11d, ecx                (edx = sum + key[...]   [right])

    xor edx, [esp + 28]                     ;  xor edx, r11d                (edx = left ^ right)
    add ebx, edx                            ;  add ebx, edx                 (kcb1 += edx)

    ; ─── Loop control ───
    dec dword [esp + 20]                    ;  dec r8d                      (rounds-- sur stack en 32-bit)
    jnz %%r                                 ;  jnz %%r

    ; ─── Save back ───
    mov [esp + 0], eax                      ;  mov [rsp], eax               (block[0] = kcb0)
    mov [esp + 4], ebx                      ;  mov [rsp + 4], ebx           (block[1] = kcb1)
%endmacro
