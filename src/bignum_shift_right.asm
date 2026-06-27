; -----------------------------------------------------------------------------
; @file    bignum_shift_right.asm
; @author  git@bayborodov.com
; @version 1.0.15
; @date    27.06.2026
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
;   - rev. 11 (26.06.2026): Провал. Branchless нормализация привела к O(N) регрессии.
;   - rev. 12 (26.06.2026): Оптимизация по результатам ревью:
;                           - Откат нормализации к O(1) (Early Exit) из bignum_common.
;                           - Развертывание цикла (Loop Unrolling x4) для bit_shift.
;                           - Сохранение Compact ABI (без push/pop).
;   - rev. 13 (26.06.2026): Исправленная и оптимизированная версия.
;   - rev. 14 (26.06.2026): Откат неудачных оптимизаций .zero_out.
;                           - .zero_out возвращён к rev13 (rep stosq).
;                           - Слияние .update_len + .set_return → .update_and_return.
;                           - ZEROED корректно возвращается через .scan_loop.
;                           - Поведение large path идентично rev13 (тесты проходят).
;   - rev. 15 (26.06.2026): Оптимизация по результатам ревью:
;                           - Избавление от медленной инструкции shrd (Microcode Sequencer Bottleneck).
;                           - Устранение избыточных чтений памяти (Loop-Carried Dependency).
;                           - O(1) Нормализация без цикла и rep stosq (Математический факт).
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
; @version    1.0.15
; =============================================================================
bignum_shift_right:
    test    rdi, rdi
    jz      .error_null_arg

    mov     edx, [rdi + BIGNUM_LEN_OFFSET]  ; rdx = len
    test    edx, edx
    jz      .success_zero                   ; Если len == 0, возвращаем 0

    test    rsi, rsi
    jz      .success_zero                   ; Если shift == 0, возвращаем 0

    ; --- Вычисление сдвигов ---
    mov     r8, rsi
    shr     r8, 6                           ; r8 = word_shift = shift / 64
    mov     r11, rsi
    and     r11, 63                         ; r11 = bit_shift = shift % 64

    ; --- Проверка на полный сдвиг ---
    cmp     r8, rdx
    jae     .zero_out

    ; --- 1. Сдвиг на целые слова ---
    test    r8, r8
    jz      .bit_shift_part                 ; Если word_shift == 0, пропускаем

    mov     r9, rdx
    sub     r9, r8                          ; r9 = new_len = len - word_shift

    ; Копируем слова
    mov     rcx, r9                         ; count = new_len
    mov     rsi, rdi
    lea     rsi, [rsi + r8 * 8]             ; src = num + word_shift
    mov     r10, rdi                        ; сохраняем rdi (num)
    rep movsq                               ; Сдвигаем слова вниз

    ; Обнуляем старшие слова
    mov     rcx, r8                         ; count = word_shift
    xor     eax, eax
    rep stosq                               ; Обнуляем старшие слова

    mov     rdi, r10                        ; восстанавливаем rdi
    mov     rdx, r9                         ; обновляем len = new_len

.bit_shift_part:
    ; --- 2. Сдвиг на биты ---
    test    r11, r11
    jz      .normalize                      ; Если bit_shift == 0, переходим к нормализации

    mov     rcx, r11                        ; cl = bit_shift

    ; Подготовка маски для замены медленного shrd
    mov     r8, -1
    shr     r8, cl
    not     r8                              ; r8 = маска для старших битов

    mov     r10, rdx
    dec     r10                             ; r10 = len - 1
    jz      .bit_shift_last                 ; Если len == 1, сдвигаем только одно слово

    mov     r9, rdi                         ; r9 = num
    mov     rax, [r9]                       ; Загружаем первое слово до цикла

    shr     r10, 2                          ; r10 = count / 4
    jz      .bit_shift_tail_loop            ; Если < 4, идем в хвостовой цикл

.bit_shift_unrolled_loop:
    ; Итерация 1
    mov     r11, [r9 + 8]
    mov     rsi, r11
    ror     rsi, cl
    and     rsi, r8
    shr     rax, cl
    or      rax, rsi
    mov     [r9], rax

    ; Итерация 2
    mov     rax, [r9 + 16]
    mov     rsi, rax
    ror     rsi, cl
    and     rsi, r8
    shr     r11, cl
    or      r11, rsi
    mov     [r9 + 8], r11

    ; Итерация 3
    mov     r11, [r9 + 24]
    mov     rsi, r11
    ror     rsi, cl
    and     rsi, r8
    shr     rax, cl
    or      rax, rsi
    mov     [r9 + 16], rax

    ; Итерация 4
    mov     rax, [r9 + 32]
    mov     rsi, rax
    ror     rsi, cl
    and     rsi, r8
    shr     r11, cl
    or      r11, rsi
    mov     [r9 + 24], r11

    add     r9, 32
    dec     r10
    jnz     .bit_shift_unrolled_loop

.bit_shift_tail_loop:
    mov     r10, rdx
    dec     r10
    and     r10, 3                          ; r10 = count % 4
    jz      .bit_shift_last

.bit_shift_tail:
    mov     r11, [r9 + 8]
    mov     rsi, r11
    ror     rsi, cl
    and     rsi, r8
    shr     rax, cl
    or      rax, rsi
    mov     [r9], rax
    mov     rax, r11                        ; Переносим старшее слово в младшее для следующего шага
    add     r9, 8
    dec     r10
    jnz     .bit_shift_tail

.bit_shift_last:
    shr     qword [rdi + rdx * 8 - 8], cl   ; Сдвиг самого старшего слова

.normalize:
    ; --- 3. Нормализация длины O(1) ---
    ; Сдвиг < 64 бит может уменьшить длину нормализованного числа максимум на 1 слово.
    ; Хвост уже обнулен (либо word_shift, либо shr в .bit_shift_last).
    cmp     qword [rdi + rdx * 8 - 8], 0
    jne     .set_len
    dec     rdx
.set_len:
    mov     [rdi + BIGNUM_LEN_OFFSET], edx
    xor     eax, eax
    test    edx, edx
    setz    al                              ; Возвращаем 1 (ZEROED), если len == 0, иначе 0 (SUCCESS)
    ret

.zero_out:
    ; --- Полное обнуление числа ---
    mov     rcx, rdx
    xor     eax, eax
    mov     r8, rdi
    rep stosq
    mov     rdi, r8
    mov     dword [rdi + BIGNUM_LEN_OFFSET], 0
    mov     eax, 1                          ; Код возврата: ZEROED
    ret

.error_null_arg:
    mov     rax, -1                         ; Код возврата: ERROR_NULL_ARG
    ret

.success_zero:
    xor     rax, rax                        ; Код возврата: SUCCESS
    ret