section .rodata
    hex_format db "%02hhx", 0
    newline_format db 10, 0
    input_flag db "-I", 0

section .data
    x_struct db 5
    x_num db 0xaa, 1, 2, 0x44, 0x4f

section .bss
    input_line resb 512

section .text
    global main
    global print_multi
    global getmulti
    extern printf
    extern fgets
    extern strcmp
    extern malloc
    extern stdin

main:
    push ebp
    mov ebp, esp
    push ebx
    push esi
    push edi

    cmp dword [ebp + 8], 2
    jl .default_input

    mov eax, [ebp + 12]
    push dword input_flag
    push dword [eax + 4]
    call strcmp
    add esp, 8

    cmp eax, 0
    jne .default_input

    call getmulti
    mov esi, eax
    call getmulti
    mov edi, eax

    push esi
    call print_multi
    add esp, 4

    push edi
    call print_multi
    add esp, 4
    jmp .done

.default_input:
    push dword x_struct
    call print_multi
    add esp, 4

.done:
    xor eax, eax
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret

print_multi:
    push ebp
    mov ebp, esp
    push ebx
    push esi
    push edi

    mov esi, [ebp + 8]
    movzx ebx, byte [esi]
    test ebx, ebx
    jz .print_newline
    dec ebx

.print_loop:
    movzx eax, byte [esi + ebx + 1]
    push eax
    push dword hex_format
    call printf
    add esp, 8

    test ebx, ebx
    jz .print_newline
    dec ebx
    jmp .print_loop

.print_newline:
    push dword newline_format
    call printf
    add esp, 4

    pop edi
    pop esi
    pop ebx
    pop ebp
    ret

getmulti:
    push ebp
    mov ebp, esp
    push ebx
    push esi
    push edi

    push dword [stdin]
    push dword 512
    push dword input_line
    call fgets
    add esp, 12

    test eax, eax
    jz .empty_input

    xor ecx, ecx

.length_loop:
    mov al, [input_line + ecx]
    cmp al, 0
    je .length_done
    cmp al, 10
    je .length_done
    cmp al, 13
    je .length_done
    inc ecx
    jmp .length_loop

.length_done:
    test ecx, ecx
    jz .empty_input

    mov esi, ecx
    lea ebx, [esi + 1]
    shr ebx, 1

    lea eax, [ebx + 1]
    push eax
    call malloc
    add esp, 4

    mov [eax], bl
    push eax
    lea edi, [eax + 1]
    dec esi

.parse_loop:
    test esi, esi
    js .parse_done

    mov al, [input_line + esi]
    call hex_to_nibble
    mov bl, al
    dec esi

    test esi, esi
    js .store_byte

    mov al, [input_line + esi]
    call hex_to_nibble
    shl al, 4
    or bl, al
    dec esi

.store_byte:
    mov [edi], bl
    inc edi
    jmp .parse_loop

.parse_done:
    pop eax
    jmp .done

.empty_input:
    push dword 2
    call malloc
    add esp, 4
    mov byte [eax], 1
    mov byte [eax + 1], 0

.done:
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret

hex_to_nibble:
    cmp al, '0'
    jb .zero
    cmp al, '9'
    jbe .digit
    cmp al, 'A'
    jb .zero
    cmp al, 'F'
    jbe .upper
    cmp al, 'a'
    jb .zero
    cmp al, 'f'
    jbe .lower

.zero:
    xor eax, eax
    ret

.digit:
    sub al, '0'
    ret

.upper:
    sub al, 'A' - 10
    ret

.lower:
    sub al, 'a' - 10
    ret
