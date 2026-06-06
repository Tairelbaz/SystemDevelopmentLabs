section .rodata
    hex_format db "%02hhx", 0
    newline_format db 10, 0
    input_flag db "-I", 0
    random_flag db "-R", 0
    MASK dw 0x002D

section .data
    x_struct db 5
    x_num db 0xaa, 1, 2, 0x44, 0x4f
    y_struct db 6
    y_num db 0xaa, 1, 2, 3, 0x44, 0x4f
    STATE dw 0xACE1

section .bss
    input_line resb 512

section .text
    global main
    global print_multi
    global getmulti
    global MaxMin
    global add_multi
    global rand_num
    global PRmulti
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
    jne .check_random_input

    call getmulti
    mov esi, eax
    call getmulti
    mov edi, eax
    jmp .print_operands_and_sum

.check_random_input:
    mov eax, [ebp + 12]
    push dword random_flag
    push dword [eax + 4]
    call strcmp
    add esp, 8

    cmp eax, 0
    jne .default_input

    call PRmulti
    mov esi, eax
    call PRmulti
    mov edi, eax
    jmp .print_operands_and_sum

.default_input:
    mov esi, x_struct
    mov edi, y_struct

.print_operands_and_sum:
    push esi
    call print_multi
    add esp, 4

    push edi
    call print_multi
    add esp, 4

    push edi
    push esi
    call add_multi
    add esp, 8

    push eax
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

.trim_loop:
    test ebx, ebx
    jz .print_loop
    cmp byte [esi + ebx + 1], 0
    jne .print_loop
    dec ebx
    jmp .trim_loop

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

; in: eax, ebx = struct pointers; out: eax = longer, ebx = other; clobbers ecx, edx
MaxMin:
    movzx ecx, byte [eax]
    movzx edx, byte [ebx]
    cmp ecx, edx
    jae .done
    xchg eax, ebx

.done:
    ret

add_multi:
    push ebp
    mov ebp, esp
    push ebx
    push esi
    push edi
    sub esp, 32

    mov eax, [ebp + 8]
    mov ebx, [ebp + 12]
    call MaxMin

    mov [ebp - 16], eax
    mov [ebp - 20], ebx

    movzx ecx, byte [eax]
    movzx edx, byte [ebx]
    mov [ebp - 24], ecx
    mov [ebp - 28], edx

    mov eax, ecx
    cmp eax, 255
    je .result_size_ready
    inc eax

.result_size_ready:
    mov [ebp - 32], eax
    lea eax, [eax + 1]
    push eax
    call malloc
    add esp, 4

    mov [ebp - 36], eax
    mov edx, [ebp - 32]
    mov [eax], dl

    mov dword [ebp - 40], 0
    mov dword [ebp - 44], 0

.add_loop:
    mov ecx, [ebp - 44]
    cmp ecx, [ebp - 24]
    jge .after_add_loop

    xor edx, edx
    mov esi, [ebp - 16]
    movzx eax, byte [esi + ecx + 1]
    add edx, eax
    add edx, [ebp - 40]

    cmp ecx, [ebp - 28]
    jge .store_sum_byte

    mov esi, [ebp - 20]
    movzx eax, byte [esi + ecx + 1]
    add edx, eax

.store_sum_byte:
    mov eax, edx
    shr eax, 8
    mov [ebp - 40], eax

    mov edi, [ebp - 36]
    mov [edi + ecx + 1], dl

    inc dword [ebp - 44]
    jmp .add_loop

.after_add_loop:
    mov eax, [ebp - 32]
    cmp eax, [ebp - 24]
    jle .return_result

    mov ecx, [ebp - 24]
    mov edx, [ebp - 40]
    mov edi, [ebp - 36]
    mov [edi + ecx + 1], dl

.return_result:
    mov eax, [ebp - 36]
    add esp, 32
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret

; out: eax = next LFSR output bit (0 or 1); advances STATE; preserves ecx, edx
rand_num:
    push ecx
    push edx

    xor edx, edx
    movzx eax, word [STATE]
    shr eax, 1
    movzx ecx, word [STATE]
    and cx, word [MASK]       ; PF = parity of the tap bits (valid because MASK fits in the low byte)
    jp .store_state           ; PF set = even parity -> feedback bit stays 0
    or eax, 0x8000
    inc edx                   ; odd parity -> feedback bit 1

.store_state:
    mov [STATE], ax
    mov eax, edx

    pop edx
    pop ecx
    ret

rand_byte:
    push ecx
    push edx

    mov ecx, 8
    xor edx, edx

.byte_loop:
    call rand_num
    shl dl, 1
    or dl, al
    dec ecx
    jnz .byte_loop

    movzx eax, dl
    pop edx
    pop ecx
    ret

PRmulti:
    push ebp
    mov ebp, esp
    push ebx
    push esi
    push edi

.length_loop:
    call rand_byte
    movzx ebx, al
    test ebx, ebx
    jz .length_loop

    lea eax, [ebx + 1]
    push eax
    call malloc
    add esp, 4

    mov edi, eax
    mov [edi], bl
    lea esi, [edi + 1]

.fill_loop:
    test ebx, ebx
    jz .done

    call rand_byte
    mov [esi], al
    inc esi
    dec ebx
    jmp .fill_loop

.done:
    mov eax, edi
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
