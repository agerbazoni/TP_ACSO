; /** defines bool y puntero **/
%define NULL 0
%define TRUE 1
%define FALSE 0

section .data
empty_str: db "", 0

section .text

global string_proc_list_create_asm
global string_proc_node_create_asm
global string_proc_list_add_node_asm
global string_proc_list_concat_asm

; FUNCIONES auxiliares que pueden llegar a necesitar:
extern malloc
extern free
extern str_concat


string_proc_list_create_asm:
    push rbp
    mov rbp, rsp

    mov rdi, 16
    call malloc

    test rax, rax
    je .return_null

    mov qword [rax], 0
    mov qword [rax + 8], 0

    jmp .end

.return_null:
    xor rax, rax
    ret


.end:
    pop rbp
    ret



string_proc_node_create_asm:
    push rbp
    mov rbp, rsp

    push r12
    push r13
    
    mov r12, rdi  ; r12 = type (primer parámetro)
    mov r13, rsi  ; r13 = hash (segundo parámetro)

    mov rdi, 32   ; tamaño para malloc
    call malloc

    test rax, rax
    je .return_null

    ; Inicializa los campos del nodo
    mov qword [rax], 0       ; node->next = NULL
    mov qword [rax + 8], 0   ; node->previous = NULL
    mov qword [rax + 16], r12 ; node->type = type (un byte)
    mov qword [rax + 24], r13 ; node->hash = hash

    jmp .end

.return_null:
    xor rax, rax
    pop r13
    pop r12
    pop rbp
    ret

.end:
    pop r13
    pop r12
    pop rbp
    ret




string_proc_list_add_node_asm:
    push rbp
    mov rbp, rsp

    push r12
    push r13        ; Guardar también r13 para usarlo como registro temporal

    mov r12, rdi    ; list

    test r12, r12
    je .return_null

    mov rdi, rsi    ; type
    mov rsi, rdx    ; hash

    call string_proc_node_create_asm

    test rax, rax
    je .return_null

    cmp qword [r12], 0    ; Verificar si list->first == NULL
    je .define_first_last_if_null

    ; Caso donde list->first != NULL
    mov r13, [r12 + 8]    ; r13 = list->last
    mov [r13], rax        ; list->last->next = node
    mov [rax + 8], r13    ; node->previous = list->last
    mov [r12 + 8], rax    ; list->last = node

    jmp .end

.return_null:
    xor rax, rax          ; rax = 0
    jmp .end

.define_first_last_if_null:
    mov [r12], rax        ; list->first = node
    mov [r12 + 8], rax    ; list->last = node
    pop r13
    pop r12
    pop rbp
    ret

.end:
    pop r13
    pop r12
    pop rbp
    ret


string_proc_list_concat_asm:
    push rbp
    mov rbp, rsp
    push r12
    push r13
    push r14
    push r15
    push rbx

    ; Check null pointers
    test rdi, rdi
    je .return_null
    test rsi, rsi
    je .return_null

    ; Store parameters in preserved registers
    mov r12, rdi        ; list
    mov r13, rsi        ; type
    mov r14, rdx        ; separator
    
    ; Initialize result with empty string + separator
    mov rdi, empty_str
    mov rsi, rdx
    call str_concat
    test rax, rax
    je .return_null

    ; Prepare for loop
    mov r15, rax        ; r15 = result (track current result)
    mov rbx, [r12]      ; rbx = list->first (current node)

.loop:
    test rbx, rbx       ; if (current == NULL)
    je .end_loop        ;     goto end

    ; Check if node matches type
    cmp byte [rbx + 16], r13b
    jne .next_node

    ; Check if hash field is not NULL
    cmp qword [rbx + 24], 0
    je .next_node

    ; Concatenate current result with node's hash
    mov rdi, r15        ; First arg: current result
    mov rsi, [rbx + 24] ; Second arg: node's hash
    
    ; Save current result pointer (to free later)
    mov r12, r15

    call str_concat     ; Call str_concat
    
    ; Check if concatenation failed
    test rax, rax
    je .cleanup_and_exit
    
    ; Update result pointer and free old result
    mov r15, rax        ; Update result pointer
    mov rdi, r12        ; Set old result as argument to free
    call free           ; Free old result

.next_node:
    mov rbx, [rbx + 8]  ; Move to next node
    jmp .loop

.cleanup_and_exit:
    ; We got an error, free the current result
    mov rdi, r15
    call free
    xor rax, rax        ; Return NULL
    jmp .return

.return_null:
    xor rax, rax        ; Return NULL
    jmp .return

.end_loop:
    mov rax, r15        ; Return final result

.return:
    ; Restore preserved registers
    pop rbx
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    ret
