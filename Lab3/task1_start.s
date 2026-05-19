section .data
    newline db 10
    Infile  dd 0
    Outfile dd 1
    err_open db "Error: cannot open file", 10
    err_open_len equ $ - err_open

section .bss
    buf resb 1
    key resd 1
    keypos resd 1

section .text
    global _start
    global system_call
    global main
    extern strlen

_start:
    pop    dword ecx
    mov    esi,esp
    mov     eax,ecx
    shl     eax,2
    add     eax,esi
    add     eax,4
    push    dword eax
    push    dword esi
    push    dword ecx

    call    main

    mov     ebx,eax
    mov     eax,1
    int     0x80
    nop

system_call:
    push    ebp
    mov     ebp, esp
    sub     esp, 4
    pushad

    mov     eax, [ebp+8]
    mov     ebx, [ebp+12]
    mov     ecx, [ebp+16]
    mov     edx, [ebp+20]
    int     0x80
    mov     [ebp-4], eax
    popad
    mov     eax, [ebp-4]
    add     esp, 4
    pop     ebp
    ret

main:
    push ebp
    mov ebp, esp

    mov edi, [ebp+8]
    mov esi, [ebp+12]

    xor ebx, ebx

.loop:
    cmp ebx, edi
    jge .encode_loop

    mov ecx, [esi + ebx*4]

    push ebx
    push ecx

    push ecx
    call strlen
    add esp, 4
    mov edx, eax

    pop ecx
    mov eax, 4
    mov ebx, 2
    int 0x80

    mov eax, 4
    mov ebx, 2
    mov ecx, newline
    mov edx, 1
    int 0x80

    pop ebx
    mov ecx, [esi + ebx*4]

    cmp byte [ecx], '+'
    jne .check_dash
    cmp byte [ecx+1], 'V'
    jne .next
    lea eax, [ecx+2]
    mov [key], eax
    mov [keypos], eax
    jmp .next

.check_dash:
    cmp byte [ecx], '-'
    jne .next
    cmp byte [ecx+1], 'i'
    je .open_input
    cmp byte [ecx+1], 'o'
    je .open_output
    jmp .next

.open_input:
    push ebx
    lea ebx, [ecx+2]
    mov eax, 5
    mov ecx, 0
    int 0x80
    cmp eax, 0
    jl .error_open
    mov [Infile], eax
    pop ebx
    jmp .next

.open_output:
    push ebx
    lea ebx, [ecx+2]
    mov eax, 5
    mov ecx, 577
    mov edx, 420
    int 0x80
    cmp eax, 0
    jl .error_open
    mov [Outfile], eax
    pop ebx
    jmp .next

.error_open:
    mov eax, 4
    mov ebx, 2
    mov ecx, err_open
    mov edx, err_open_len
    int 0x80
    mov eax, 1
    mov ebx, 1
    int 0x80

.next:
    inc ebx
    jmp .loop

.encode_loop:
    mov eax, 3
    mov ebx, [Infile]
    mov ecx, buf
    mov edx, 1
    int 0x80

    cmp eax, 0
    jle .done

    call encode_char

    mov eax, 4
    mov ebx, [Outfile]
    mov ecx, buf
    mov edx, 1
    int 0x80

    jmp .encode_loop

.done:
    mov eax, 1
    mov ebx, 0
    int 0x80

encode_char:
    mov al, [buf]
    cmp al, 'a'
    jl .no_encode
    cmp al, 'z'
    jg .no_encode

    mov ebx, [key]
    test ebx, ebx
    jz .no_encode

    mov ebx, [keypos]
    movzx ecx, byte [ebx]
    sub ecx, 'A'

    sub al, 'a'
    add al, cl
    mov ah, 0
    mov bl, 26
    div bl
    mov al, ah
    add al, 'a'
    mov [buf], al

    mov ebx, [keypos]
    inc ebx
    cmp byte [ebx], 0
    jne .save_keypos
    mov ebx, [key]
.save_keypos:
    mov [keypos], ebx

.no_encode:
    ret
