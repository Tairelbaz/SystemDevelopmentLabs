section .data
    newline db 10

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
    cmp ebx, edi              ; if i >= argc
    jge .done                 ; we're done

    mov ecx, [esi + ebx*4]   ; ecx = argv[i]

    push ebx                  ; save loop counter
    push ecx                  ; save argv[i]

    push ecx                  ; push argument for strlen
    call strlen
    add esp, 4                ; clean up stack
    mov edx, eax              ; edx = string length

    pop ecx                   ; restore ecx = argv[i]
    mov eax, 4                ; sys_write
    mov ebx, 2                ; stderr
    int 0x80

    mov eax, 4                ; sys_write
    mov ebx, 2                ; stderr
    mov ecx, newline          ; newline char
    mov edx, 1                ; length 1
    int 0x80

    pop ebx                   ; restore loop counter
    inc ebx                   ; i++
    jmp .loop

.done:
    mov eax, 1                ; sys_exit
    mov ebx, 0                ; exit code 0
    int 0x80
