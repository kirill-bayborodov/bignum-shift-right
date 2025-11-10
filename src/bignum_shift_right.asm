; -----------------------------------------------------------------------------
; @file    bignum_shift_right.asm
; @author  git@bayborodov.com
; @version 1.0.0
; @date    10.11.2025
;
; @brief   Низкоуровневая реализация логического сдвига bignum_t вправо.
;
; @history
;   - rev. 1-6: Провальные итерации с ошибками синтаксиса, логики и SIGSEGV.
;   - rev. 7 (11.08.2025): Провал. Попытка оптимизации привела к регрессии.
;   - rev. 8 (11.08.2025): Рабочая версия. Исправлена ошибка с перезаписью `cl`.
;   - rev. 9 (11.08.2025): Финальная оптимизация по результатам ревью:
;                          - Цикл нормализации переписан для минимизации ветвлений.
;                          - Улучшена адресная арифметика.
;   - rev. 10 (10.11.2025): Removed version control functions and .data section
; -----------------------------------------------------------------------------

section .text

; --- Публичные символы ---
global bignum_shift_right

; --- Константы ---
BIGNUM_WORDS_OFFSET equ 0
BIGNUM_LEN_OFFSET   equ 256

; =============================================================================
; @brief      Выполняет логический сдвиг большого числа вправо.
; @param      rdi: bignum_t* num - Указатель на bignum_t.
; @param      rsi: size_t shift_amount - Количество бит для сдвига.
; @return     rax: Код состояния: 0 (SUCCESS), 1 (ZEROED), -1 (ERROR_NULL_ARG).
; @note       Использует ABI System V AMD64.
; @version    1.0.0
; =============================================================================
bignum_shift_right:
    ; --- Пролог ---
    push    rbx
    push    r9
    push    r12
    push    r13
    push    r14

    ; --- Сохранение аргументов и проверка ---
    mov     r12, rdi
    mov     r13, rsi
    test    r12, r12
    jz      .error_null_arg
    mov     r14d, [r12 + BIGNUM_LEN_OFFSET]
    test    r14d, r14d
    jz      .success
    test    r13, r13
    jz      .success

    ; --- Вычисление сдвигов (без div) ---
    mov     rbx, r13
    shr     rbx, 6
    mov     r9b, r13b
    and     r9b, 63

    ; --- Проверка на полный сдвиг ---
    cmp     rbx, r14
    jae     .zero_out

    ; --- 1. Сдвиг на целые слова ---
    test    rbx, rbx
    jz      .bit_shift_part

.word_shift:
    mov     rdi, r12
    add     rdi, BIGNUM_WORDS_OFFSET
    lea     rsi, [rdi + rbx * 8]
    mov     rax, r14
    sub     rax, rbx
    mov     rcx, rax
    rep movsq

    mov     [r12 + BIGNUM_LEN_OFFSET], eax
    mov     r14d, eax

    lea     rdi, [r12 + r14 * 8]
    mov     rcx, rbx
    xor     rax, rax
    rep stosq

.bit_shift_part:
    ; --- 2. Сдвиг на биты ---
    mov     cl, r9b
    test    cl, cl
    jz      .normalize

    lea     rdi, [r12 + BIGNUM_WORDS_OFFSET]
    mov     rdx, r14
    dec     rdx
    jz      .bit_shift_last_word

.bit_shift_loop:
    mov     rax, [rdi]
    mov     rbx, [rdi + 8]
    shrd    rax, rbx, cl
    mov     [rdi], rax
    add     rdi, 8
    dec     rdx
    jnz     .bit_shift_loop

.bit_shift_last_word:
    shr     QWORD [rdi], cl

.normalize:
    ; --- 3. Оптимизированная нормализация длины ---
    mov     r14d, [r12 + BIGNUM_LEN_OFFSET]
    test    r14d, r14d
    jz      .zeroed_done
    lea     rdi, [r12 + r14 * 8 - 8]
    xor     eax, eax            ; eax = флаг is_nonzero = 0

.normalize_loop:
    mov     rbx, [rdi]
    test    rbx, rbx
    jnz     .nonzero_found
    dec     r14d
    sub     rdi, 8
    test    r14d, r14d
    jnz     .normalize_loop
    jmp     .zeroed_done

.nonzero_found:
    mov     eax, 1

.zeroed_done:
    mov     [r12 + BIGNUM_LEN_OFFSET], r14d
    test    eax, eax
    jnz     .success

    mov     rax, 1
    jmp     .exit

.zero_out:
    lea     rdi, [r12 + BIGNUM_WORDS_OFFSET]
    mov     rcx, r14
    xor     rax, rax
    rep stosq
    mov     DWORD [r12 + BIGNUM_LEN_OFFSET], 0
    mov     rax, 1
    jmp     .exit

.error_null_arg:
    mov     rax, -1
    jmp     .exit

.success:
    xor     rax, rax

.exit:
    ; --- Эпилог ---
    pop     r14
    pop     r13
    pop     r12
    pop     r9
    pop     rbx
    ret
