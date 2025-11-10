/**
 * @file    bignum_shift_right.h
 * @author  git@bayborodov.com
 * @version 1.0.0
 * @date    10.11.2025
 *
 * @brief   Публичный API для логического сдвига bignum_t вправо.
 *
 * @details
 *   Выполняет in-place (на месте) логический сдвиг большого числа на
 *   заданное количество бит. Нормализация (удаление ведущих нулей)
 *   выполняется автоматически.
 *
 *   Функция является потокобезопасной при условии, что разные потоки
 *   работают с разными, не пересекающимися объектами `bignum_t`.
 *
 *   **Алгоритм:**
 *    1. Проверка аргументов.
 *    2. Нулевой сдвиг — быстрый выход.
 *    3. Разбиение `shift_amount` на сдвиг по словам (`word_shift`) и битам (`bit_shift`).
 *    4. Если сдвиг по словам превышает длину числа, обнулить его и вернуть BIGNUM_SHIFT_RIGHT_ZEROED.
 *    5. Сдвиг по словам, затем побитовый сдвиг с переносами между словами.
 *    6. Обновление `len` и нормализация результата.
 *
 * @see     bignum.h
 * @since   1.0.0
 *
 * @history
 *   - rev. 1 (10.08.2025): Первоначальное создание API по аналогии с bignum_shift_left.
 *   - rev. 2 (10.08.2025): Улучшен API по результатам ревью: добавлен код возврата
 *                         BIGNUM_SHIFT_RIGHT_ZEROED, уточнена документация
 *                         потокобезопасности и поведения при обнулении.
 *   - rev. 3 (10.11.2025): Removed version control functions.
 */

#ifndef BIGNUM_SHIFT_RIGHT_H
#define BIGNUM_SHIFT_RIGHT_H

#include <bignum.h>
#include <stddef.h>
#include <stdint.h>

// Проверка на наличие определения BIGNUM_CAPACITY из общего заголовка
#ifndef BIGNUM_CAPACITY
#  error "bignum.h must define BIGNUM_CAPACITY"
#endif

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Коды состояния для функции bignum_shift_right.
 */
typedef enum {
    BIGNUM_SHIFT_RIGHT_SUCCESS         =  0, /**< Успех. Сдвиг выполнен. */
    BIGNUM_SHIFT_RIGHT_ERROR_NULL_ARG  = -1, /**< Указатель `num` равен NULL. */
    BIGNUM_SHIFT_RIGHT_ZEROED          =  1, /**< Сдвиг больше длины числа, результат обнулен. */
} bignum_shift_right_status_t;

/**
 * @brief      Выполняет логический сдвиг большого числа вправо.
 *
 * @details
 *   **Алгоритм:**
 *   1.  Проверка аргументов на NULL.
 *   2.  Если `shift_amount` равен 0, немедленно вернуть успех.
 *   3.  Вычислить сдвиг в целых словах (`word_shift`) и в битах внутри слова (`bit_shift`).
 *   4.  Если `word_shift` больше или равен `len`, обнулить число и вернуть `BIGNUM_SHIFT_RIGHT_ZEROED`.
 *   5.  Выполнить сдвиг по словам, перемещая данные в младшие позиции.
 *   6.  Выполнить побитовый сдвиг внутри слов, распространяя биты-переносы
 *       из старших слов в младшие.
 *   7.  Обновить поле `len` и нормализовать результат (удалить ведущие нули).
 *
 * @param[in,out] num           Указатель на число для модификации.
 * @param[in]     shift_amount  Количество бит для сдвига вправо.
 *
 * @return     Код состояния `bignum_shift_right_status_t`.
 *   - `BIGNUM_SHIFT_RIGHT_SUCCESS` (0) – сдвиг выполнен успешно.
 *   - `BIGNUM_SHIFT_RIGHT_ERROR_NULL_ARG` (-1) – передан NULL вместо числа.
 *   - `BIGNUM_SHIFT_RIGHT_ZEROED` (1) – сдвиг был настолько велик, что все значащие
 *     биты были потеряны и число стало равно 0.
 */
bignum_shift_right_status_t bignum_shift_right(bignum_t* restrict num, size_t shift_amount);


#ifdef __cplusplus
}
#endif

#endif /* BIGNUM_SHIFT_RIGHT_H */
