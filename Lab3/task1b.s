section .data
    newline db 10
    Infile  dd 0
    Outfile dd 1

section .bss
    buf resb 1
    key resd 1
    keypos resd 1

section .text
    global main
    extern strlen

main:
    push ebp
    mov ebp, esp

    mov edi, [ebp+8]         ; edi = argc
    mov esi, [ebp+12]        ; esi = argv

    xor ebx, ebx             ; ebx = 0 (i = 0)

.loop:
    cmp ebx, edi
    jge .encode_loop

    mov ecx, [esi + ebx*4]   ; ecx = argv[i]

    push ebx
    push ecx

    push ecx
    call strlen
    add esp, 4
    mov edx, eax

    pop ecx
    mov eax, 4
    mov ebx, 2                ; stderr
    int 0x80

    mov eax, 4
    mov ebx, 2
    mov ecx, newline
    mov edx, 1
    int 0x80

    ; restore ecx = argv[i] for flag check
    mov ecx, [esp]            ; peek at saved ecx (now under ebx on stack)
    ; actually we need to get argv[i] again
    pop ebx                   ; restore loop counter
    mov ecx, [esi + ebx*4]   ; reload argv[i]

    cmp byte [ecx], '+'
    jne .next
    cmp byte [ecx+1], 'V'
    jne .next
    lea eax, [ecx+2]
    mov [key], eax
    mov [keypos], eax

.next:
    inc ebx
    jmp .loop

.encode_loop:
    mov eax, 3                ; sys_read
    mov ebx, [Infile]
    mov ecx, buf
    mov edx, 1
    int 0x80

    cmp eax, 0
    jle .done

    call encode_char

    mov eax, 4                ; sys_write
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
