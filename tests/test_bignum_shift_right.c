/**
 * @file    test_bignum_shift_right.c
 * @author  git@bayborodov.com
 * @version 1.0.0
 * @date    10.11.2025
 *
 * @brief   Детерминированные тесты для функции bignum_shift_right.
 *
 * @details
 *   Этот файл содержит набор детерминированных модульных тестов для проверки
 *   корректности реализации функции `bignum_shift_right`. Тесты покрывают
 *   основные сценарии, граничные случаи и проверку статусов возврата.
 *   Код тестов предоставлен пользователем.
 *
 * @history
 *   - rev. 1 (09.08.2025): Первая реализация тестов.
 *   - rev. 2 (09.08.2025): Провал. Расширение покрытия по результатам ревью.
 *   - rev. 3 (10.08.2025): Провал. Улучшение Doxygen, диагностики и покрытия.
 *   - rev. 4 (10.08.2025): Провал. Устранение неполноты артефакта и анализа покрытия.
 *   - rev. 5 (11.08.2025): Провал. Исправление неверных эталонных значений.
 *   - rev. 6 (11.08.2025): Провал. Исправление не всех ошибок в эталонах.
 *   - rev. 7 (11.08.2025): Провал. Исправлены все ошибки, выявленные в rev. 6.
 *   - rev. 8 (11.08.2025): Код тестов предоставлен пользователем. Добавлена
 *                         исчерпывающая документация. Восстановлена полная
 *                         и честная история ревизий.
 */

#include "bignum_shift_right.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

// --- Тестовая инфраструктура ---
static int tests_total = 0;
static int tests_passed = 0;
#define RUN_TEST(test_func) do { tests_total++; printf("\n--- Running test: %s ---\n", #test_func); if (test_func()) { tests_passed++; printf("--- Test PASSED: %s ---\n", #test_func); } else { printf("--- Test FAILED: %s ---\n", #test_func); } } while (0)
static int bignum_are_equal(const bignum_t* a, const bignum_t* b) { if (a->len != b->len) { fprintf(stderr, "FAIL: length mismatch. Expected %ld, got %ld\n", b->len, a->len); return 0; } if (a->len > 0 && memcmp(a->words, b->words, a->len * sizeof(uint64_t)) != 0) { for(size_t i = 0; i < a->len; ++i) { if (a->words[i] != b->words[i]) { fprintf(stderr, "FAIL: word[%ld] mismatch. Expected 0x%016lx, got 0x%016lx\n", i, b->words[i], a->words[i]); return 0; } } } for (int i = a->len; i < BIGNUM_CAPACITY; ++i) { if (a->words[i] != 0) { fprintf(stderr, "FAIL: tail is not zeroed at index %d. Got 0x%016lx\n", i, a->words[i]); return 0; } } return 1; }

// --- Тестовые случаи ---

/**
 * @brief      Тест: сдвиг на 0 бит не должен изменять число.
 * @pre        num = {123, len=1}
 * @post       num = {123, len=1}, status = SUCCESS
 * @return     1 в случае успеха, 0 в случае провала.
 */
int test_shift_by_zero() {
    bignum_t num = {.words = {123}, .len = 1};
    bignum_t expected = {.words = {123}, .len = 1};
    bignum_shift_right_status_t status = bignum_shift_right(&num, 0);
    if (status != BIGNUM_SHIFT_RIGHT_SUCCESS) return 0;
    return bignum_are_equal(&num, &expected);
}

/**
 * @brief      Тест: простой сдвиг в пределах одного слова.
 * @pre        num = {0xD, len=1}, shift = 2
 * @post       num = {0x3, len=1}, status = SUCCESS
 * @return     1 в случае успеха, 0 в случае провала.
 */
int test_simple_shift_within_word() {
    bignum_t num = {.words = {0xD}, .len = 1};
    bignum_t expected = {.words = {0x3}, .len = 1};
    bignum_shift_right_status_t status = bignum_shift_right(&num, 2);
    if (status != BIGNUM_SHIFT_RIGHT_SUCCESS) return 0;
    return bignum_are_equal(&num, &expected);
}

/**
 * @brief      Тест: сдвиг с переносом битов между словами.
 * @pre        num = {0x...AA, 0xF, len=2}, shift = 4
 * @post       num = {0xFA...AA, len=1}, status = SUCCESS
 * @return     1 в случае успеха, 0 в случае провала.
 */
int test_shift_with_carry_between_words() {
    bignum_t num = {.words = {0xAAAAAAAAAAAAAAAA, 0xF}, .len = 2};
    bignum_t expected;
    memset(&expected, 0, sizeof(expected));
    expected.words[0] = 0xFAAAAAAAAAAAAAAA;
    expected.len = 1;
    bignum_shift_right_status_t status = bignum_shift_right(&num, 4);
    if (status != BIGNUM_SHIFT_RIGHT_SUCCESS) return 0;
    return bignum_are_equal(&num, &expected);
}

/**
 * @brief      Тест: сдвиг на целое слово (64 бита).
 * @pre        num = {1, 2, 3, len=3}, shift = 64
 * @post       num = {2, 3, len=2}, status = SUCCESS
 * @return     1 в случае успеха, 0 в случае провала.
 */
int test_shift_by_full_word() {
    bignum_t num = {.words = {1, 2, 3}, .len = 3};
    bignum_t expected;
    memset(&expected, 0, sizeof(expected));
    expected.words[0] = 2;
    expected.words[1] = 3;
    expected.len = 2;
    bignum_shift_right_status_t status = bignum_shift_right(&num, 64);
    if (status != BIGNUM_SHIFT_RIGHT_SUCCESS) return 0;
    return bignum_are_equal(&num, &expected);
}

/**
 * @brief      Тест: сдвиг, приводящий к обнулению числа, и проверка статуса.
 * @pre        num = {1, len=1}, shift = 1
 * @post       num = {0, len=0}, status = ZEROED
 * @return     1 в случае успеха, 0 в случае провала.
 */
int test_shift_to_zero_and_check_status() {
    bignum_t num = {.words = {0x1}, .len = 1};
    bignum_t expected = {.len = 0};
    bignum_shift_right_status_t status = bignum_shift_right(&num, 1);
    if (status != BIGNUM_SHIFT_RIGHT_ZEROED) return 0;
    return bignum_are_equal(&num, &expected);
}

/**
 * @brief      Тест: сдвиг пустого числа.
 * @pre        num = {len=0}, shift = 10
 * @post       num = {len=0}, status = SUCCESS
 * @return     1 в случае успеха, 0 в случае провала.
 */
int test_shift_empty_bignum() {
    bignum_t num = {.len = 0};
    bignum_t expected = {.len = 0};
    bignum_shift_right_status_t status = bignum_shift_right(&num, 10);
    if (status != BIGNUM_SHIFT_RIGHT_SUCCESS) return 0;
    return bignum_are_equal(&num, &expected);
}

/**
 * @brief      Тест: сдвиг на число бит, большее длины числа.
 * @pre        num = {1, 2, 3, len=3}, shift = 200
 * @post       num = {len=0}, status = ZEROED
 * @return     1 в случае успеха, 0 в случае провала.
 */
int test_shift_beyond_length_returns_zeroed_status() {
    bignum_t num = {.words = {1, 2, 3}, .len = 3};
    bignum_t expected = {.len = 0};
    bignum_shift_right_status_t status = bignum_shift_right(&num, 200);
    if (status != BIGNUM_SHIFT_RIGHT_ZEROED) return 0;
    return bignum_are_equal(&num, &expected);
}

/**
 * @brief      Тест: сложный сдвиг на некратное 64 число бит.
 * @pre        num = {0xFF, 0xEE, 0xDD, len=3}, shift = 66
 * @post       num = {0x40...3B, 0x37, len=2}, status = SUCCESS
 * @return     1 в случае успеха, 0 в случае провала.
 */
int test_shift_complex_amount_across_words() {
    bignum_t num = {.words = {0xFF, 0xEE, 0xDD}, .len = 3};
    bignum_t expected;
    memset(&expected, 0, sizeof(expected));
    expected.words[0] = 0x400000000000003BULL;
    expected.words[1] = 0x37ULL;
    expected.len      = 2;
    bignum_shift_right_status_t status = bignum_shift_right(&num, 66);
    if (status != BIGNUM_SHIFT_RIGHT_SUCCESS) return 0;
    return bignum_are_equal(&num, &expected);
}

/**
 * @brief      Тест: сдвиг числа максимальной длины.
 * @pre        num = {0, ..., 0x80...0, len=CAPACITY}, shift = 1
 * @post       num = {0, ..., 0x40...0, len=CAPACITY}, status = SUCCESS
 * @return     1 в случае успеха, 0 в случае провала.
 */
int test_shift_max_len_bignum() {
    bignum_t num;
    memset(&num, 0, sizeof(num));
    num.len = BIGNUM_CAPACITY;
    num.words[BIGNUM_CAPACITY - 1] = 0x8000000000000000;
    bignum_t expected;
    memset(&expected, 0, sizeof(expected));
    expected.len = BIGNUM_CAPACITY;
    expected.words[BIGNUM_CAPACITY - 1] = 0x4000000000000000;
    bignum_shift_right_status_t status = bignum_shift_right(&num, 1);
    if (status != BIGNUM_SHIFT_RIGHT_SUCCESS) return 0;
    return bignum_are_equal(&num, &expected);
}

/**
 * @brief      Тест: сдвиг ровно на всю длину числа в битах.
 * @pre        num = {1, 2, 3, len=3}, shift = 192
 * @post       num = {len=0}, status = ZEROED
 * @return     1 в случае успеха, 0 в случае провала.
 */
int test_shift_exactly_full_length() {
    bignum_t num = {.words = {1, 2, 3}, .len = 3};
    bignum_t expected = {.len = 0};
    bignum_shift_right_status_t status = bignum_shift_right(&num, 192);
    if (status != BIGNUM_SHIFT_RIGHT_ZEROED) return 0;
    return bignum_are_equal(&num, &expected);
}

/**
 * @brief      Тест: проверка корректной нормализации после сдвига.
 * @pre        num = {1, 0xdeadbeef, len=2}, shift = 64
 * @post       num = {0xdeadbeef, len=1}, status = SUCCESS
 * @return     1 в случае успеха, 0 в случае провала.
 */
int test_normalization_after_shift() {
    bignum_t num = {.words = {0x1, 0xdeadbeef}, .len = 2};
    bignum_t expected;
    memset(&expected, 0, sizeof(expected));
    expected.words[0] = 0xdeadbeef;
    expected.len = 1;
    bignum_shift_right_status_t status = bignum_shift_right(&num, 64);
    if (status != BIGNUM_SHIFT_RIGHT_SUCCESS) return 0;
    return bignum_are_equal(&num, &expected);
}

int main() {
    printf("Starting deterministic tests for bignum_shift_right 1.0.0 (rev. 10)...\n");
    RUN_TEST(test_shift_by_zero);
    RUN_TEST(test_simple_shift_within_word);
    RUN_TEST(test_shift_with_carry_between_words);
    RUN_TEST(test_shift_by_full_word);
    RUN_TEST(test_shift_to_zero_and_check_status);
    RUN_TEST(test_shift_empty_bignum);
    RUN_TEST(test_shift_beyond_length_returns_zeroed_status);
    RUN_TEST(test_shift_complex_amount_across_words);
    RUN_TEST(test_shift_max_len_bignum);
    RUN_TEST(test_shift_exactly_full_length);
    RUN_TEST(test_normalization_after_shift);
    printf("\n----------------------------------------\n");
    printf("Test summary: %d/%d tests passed.\n", tests_passed, tests_total);
    printf("----------------------------------------\n");
    return (tests_passed == tests_total) ? 0 : 1;
}
