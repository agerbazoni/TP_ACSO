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
extern strdup


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

.end:
    mov rsp, rbp
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
    mov byte [rax + 16], r12b
    mov qword [rax + 24], r13 ; node->hash = hash

    jmp .end

.return_null:
    xor rax, rax
    pop r13
    pop r12
    mov rsp, rbp
    pop rbp
    ret

.end:
    pop r13
    pop r12
    mov rsp, rbp
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
    mov rsp, rbp
    pop rbp
    ret

.end:
    pop r13
    pop r12
    mov rsp, rbp
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

    ; Bandera de depuración: inicializada en 0
    xor r8, r8

    ; Check null pointers
    test rdi, rdi
    jne .list_not_null
    mov r8, 1          ; Código 1: list == NULL
    jmp .return_null

.list_not_null:
    test rdx, rdx      ; Verificar si separator es NULL
    jne .separator_not_null
    mov r8, 2          ; Código 2: separator == NULL
    jmp .return_null

.separator_not_null:
    ; Store parameters in preserved registers
    mov r12, rdi        ; list
    mov r13, rsi        ; type
    mov r14, rdx        ; separator
    
    ; Initialize result with empty string
    mov rdi, empty_str
    call strdup         ; Necesitas esta función o implementarla
    test rax, rax
    jne .strdup_ok
    mov r8, 3          ; Código 3: strdup falló
    jmp .return_null

.strdup_ok:
    ; Prepare for loop
    mov r15, rax        ; r15 = result (track current result)
    mov rbx, [r12]      ; rbx = list->first (current node)
    
    ; Si la lista está vacía, saltar a añadir prefijo
    test rbx, rbx
    jne .loop
    mov r8, 4          ; Código 4: lista vacía
    jmp .add_prefix

.loop:
    test rbx, rbx       ; if (current == NULL)
    je .add_prefix

    ; Check if node matches type
    mov r8, 5          ; Código 5: verificando tipo
    cmp byte [rbx + 16], r13b
    jne .next_node

    ; Check if hash field is not NULL
    mov r8, 6          ; Código 6: verificando hash
    cmp qword [rbx + 24], 0
    je .next_node

    ; Concatenate current result with node's hash
    mov r8, 7          ; Código 7: antes de concatenar
    mov rdi, r15        ; First arg: current result
    mov rsi, [rbx + 24] ; Second arg: node's hash
    
    ; Save current result pointer (to free later)
    push r15
    
    call str_concat     ; Call str_concat
    
    ; Check if concatenation failed
    test rax, rax
    jne .concat_ok
    mov r8, 8          ; Código 8: str_concat falló
    pop rdi            ; Limpiar la pila
    jmp .cleanup_and_exit

.concat_ok:
    ; Update result pointer and free old result
    mov r15, rax        ; Update result pointer
    pop rdi             ; Get old result pointer
    call free           ; Free old result

.next_node:
    mov rbx, [rbx]      ; Move to next node
    jmp .loop

.add_prefix:
    ; Añadir prefijo al inicio del resultado
    mov r8, 9          ; Código 9: añadiendo prefijo
    mov rdi, r14        ; First arg: separator (prefix)
    mov rsi, r15        ; Second arg: current result
    
    push r15            ; Save current result pointer
    
    call str_concat     ; Call str_concat
    
    test rax, rax
    jne .prefix_ok
    mov r8, 10         ; Código 10: falló al añadir prefijo
    pop rdi            ; Limpiar la pila
    jmp .cleanup_and_exit

.prefix_ok:
    mov r15, rax        ; Update result pointer
    pop rdi             ; Get old result pointer
    call free           ; Free old result
    jmp .end

.cleanup_and_exit:
    ; We got an error, free the current result
    mov rdi, r15
    call free
    xor rax, rax        ; Return NULL
    jmp .return

.return_null:
    xor rax, rax        ; Return NULL
    jmp .return

.end:
    mov rax, r15        ; Return final result
    mov r8, 0           ; Código 0: éxito

.return:
    ; Agregar el código de error al bit más alto si hubo error
    test rax, rax
    jnz .skip_error_code
    
    ; Si rax es NULL, usamos r8 como código de error
    ; Puedes almacenar el código de error en algún lugar o
    ; retornarlo de alguna manera que puedas verificar después
    
    ; Opción: modificar un registro global o una variable global
    ; (necesitarías agregar esto a tu código)

.skip_error_code:
    ; Restore preserved registers
    pop rbx
    pop r15
    pop r14
    pop r13
    pop r12
    mov rsp, rbp
    pop rbp
    ret