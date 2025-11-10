/**
 * @file    test_bignum_shift_right_extra.c
 * @author  git@bayborodov.com
 * @version 1.0.0
 * @date    10.11.2025
 *
 * @brief   Дополнительные тесты для функции bignum_shift_right.
 *
 * @details
 *   Этот файл содержит тесты на робастность и специальные случаи:
 *   - Обработка некорректных аргументов (NULL).
 *   - Проверка корректности in-place операции.
 *   - Полноценный фаззинг-тест со сравнением против библиотеки GMP.
 *   - Тесты на граничные значения величины сдвига.
 *   - Тесты на потокобезопасность.
 *   Код тестов предоставлен пользователем.
 *
 * @note
 *   Для сборки этого теста требуется библиотека GMP (`-lgmp`) и Pthreads (`-pthread`).
 *
 * @history
 *   - rev. 1 (10.08.2025): Первая реализация экстра-тестов.
 *   - rev. 2 (10.08.2025): Провал. Попытка вынести общую логику в test_utils.h.
 *   - rev. 3 (10.08.2025): Провал. Добавлена некорректная эталонная реализация для фаззинга.
 *   - rev. 4 (10.08.2025): Провал. Добавлена зависимость от GMP, но с ошибками.
 *   - rev. 5 (10.08.2025): Провал. Исправлены ошибки GMP, но остались другие недочеты.
 *   - rev. 6 (10.08.2025): Провал. Улучшена документация и структура, но история сокращена.
 *   - rev. 7 (10.08.2025): Провал. Восстановлена документация, но история снова сокращена.
 *   - rev. 8 (10.08.2025): Провал. Восстановлена история, но с ошибкой компиляции.
 *   - rev. 9 (11.08.2025): Код тестов предоставлен пользователем. Добавлена
 *                         исчерпывающая документация. Восстановлена полная
 *                         и честная история ревизий.
 */

#include "bignum_shift_right.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <gmp.h>
#include <pthread.h>
#include <inttypes.h>
#include <assert.h>

/* === Тестовая инфраструктура === */

/** @brief Структура для хранения итогов тестирования. */
typedef struct {
    int total;
    int passed;
} test_summary_t;

/** @brief Макрос для запуска тестовой функции и обновления итогов. */
#define RUN_TEST(summary, func)                          \
    do {                                                 \
        (summary).total++;                               \
        printf("--- Running test: %s ---\n", #func);     \
        if (func()) {                                    \
            (summary).passed++;                         \
            printf("--- Test PASSED: %s ---\n\n", #func);\
        } else {                                         \
            printf("--- Test FAILED: %s ---\n\n", #func);\
        }                                                \
    } while (0)

/* === Вспомогательные функции === */

/**
 * @internal
 * @brief      Инициализирует bignum из массива слов.
 * @param[out] dst   Указатель на выходную структуру.
 * @param[in]  words Массив 64-битных слов (little-endian).
 * @param[in]  len   Число слов в массиве.
 */
static void make_bn(bignum_t *dst, const uint64_t *words, int len) {
    memset(dst, 0, sizeof(bignum_t));
    if (words && len > 0) {
        memcpy(dst->words, words, len * sizeof(uint64_t));
    }
    dst->len = len;
}

/**
 * @internal
 * @brief Сравнивает два bignum, выводит в stderr при несовпадении.
 * @return 1, если равны; 0 иначе.
 */
static int compare_bn(const bignum_t *a, const bignum_t *b) {
    if (a->len != b->len) {
        fprintf(stderr, "Length mismatch: %ld != %ld\n", a->len, b->len);
        return 0;
    }
    for (size_t i = 0; i < a->len; i++) {
        if (a->words[i] != b->words[i]) {
            fprintf(stderr, "Word[%ld] mismatch: %016" PRIx64 " != %016" PRIx64 "\n", i, a->words[i], b->words[i]);
            return 0;
        }
    }
    return 1;
}

/**
 * @internal
 * @brief Печатает bignum для отладки.
 */
static void print_bn(const char *title, const bignum_t *n) {
    printf("%s (len=%ld): 0x", title, n->len);
    if (n->len == 0) {
        printf("0\n");
        return;
    }
    for (int i = n->len - 1; i >= 0; i--) {
        printf("%016" PRIx64, n->words[i]);
    }
    printf("\n");
}

/**
 * @internal
 * @brief Конвертирует mpz_t → bignum_t.
 * @return 1 при успехе, 0 при переполнении.
 */
static int bignum_from_gmp(bignum_t *dst, const mpz_t src) {
    size_t count;
    void *limbs = mpz_export(NULL, &count, -1, sizeof(uint64_t), 0, 0, src);
    if (count > BIGNUM_CAPACITY) {
        if (limbs) free(limbs);
        return 0;
    }
    make_bn(dst, limbs, (int)count);
    if (limbs) free(limbs);
    return 1;
}

/* === Тесты === */

/**
 * @brief      Тест: обработка NULL-аргумента.
 * @pre        num = NULL, shift = 10
 * @post       status = BIGNUM_SHIFT_RIGHT_ERROR_NULL_ARG
 * @return     1 в случае успеха, 0 в случае провала.
 */
static int test_null_argument_handling(void) {
    return bignum_shift_right(NULL, 10) == BIGNUM_SHIFT_RIGHT_ERROR_NULL_ARG;
}

/**
 * @brief      Тест: корректность in-place сдвига (без перекрытия).
 * @pre        num = {0x11, 0x22, 0x33, 0x44, len=4}, shift = 64
 * @post       num = {0x22, 0x33, 0x44, len=3}
 * @return     1 в случае успеха, 0 в случае провала.
 */
static int test_inplace_shift_overlap(void) {
    bignum_t n;
    uint64_t w[4] = {0x11,0x22,0x33,0x44};
    make_bn(&n, w, 4);

    bignum_shift_right(&n, 64);
    uint64_t exp[3] = {0x22,0x33,0x44};
    bignum_t e;
    make_bn(&e, exp, 3);

    return (n.len == 3 && compare_bn(&n, &e));
}

/**
 * @brief      Тест: граничные значения битового сдвига (1 и 63).
 * @return     1 в случае успеха, 0 в случае провала.
 */
static int test_edge_case_shifts(void) {
    uint64_t base_w[2] = { 0x8000000000000001ULL, 0x0000000000000002ULL };
    bignum_t base;
    make_bn(&base, base_w, 2);

    /* shift = 1 */
    bignum_t n1;
    make_bn(&n1, (uint64_t[]){0x4000000000000000ULL, 0x0000000000000001ULL}, 2);
    bignum_t tmp = base;
    if (bignum_shift_right(&tmp, 1) != BIGNUM_SHIFT_RIGHT_SUCCESS || !compare_bn(&tmp, &n1)) {
        print_bn("Got(1)", &tmp); print_bn("Exp(1)", &n1); return 0;
    }

    /* shift = 63 */
    bignum_t n2;
    make_bn(&n2, (uint64_t[]){0x0000000000000005ULL}, 1);
    tmp = base;
    if (bignum_shift_right(&tmp, 63) != BIGNUM_SHIFT_RIGHT_SUCCESS || !compare_bn(&tmp, &n2)) {
        print_bn("Got(63)", &tmp); print_bn("Exp(63)", &n2); return 0;
    }
    return 1;
}

/**
 * @brief      Тест: сдвиг на 0 не меняет число.
 * @return     1 в случае успеха, 0 в случае провала.
 */
static int test_zero(void) {
    bignum_t n, e;
    uint64_t w = 0x123456789ABCDEF0ULL;
    make_bn(&n, &w, 1);
    make_bn(&e, &w, 1);
    bignum_shift_right(&n, 0);
    return compare_bn(&n, &e);
}

/**
 * @brief      Тест: сдвиг только на целые слова.
 * @return     1 в случае успеха, 0 в случае провала.
 */
static int test_word_only(void) {
    uint64_t w[3] = {1,2,3};
    bignum_t n, e;
    make_bn(&n, w, 3);
    make_bn(&e, (uint64_t[]){3}, 1);
    bignum_shift_right(&n, 128);
    return compare_bn(&n, &e);
}

/**
 * @brief      Тест: сдвиг только на биты, без сдвига слов.
 * @return     1 в случае успеха, 0 в случае провала.
 */
static int test_bit_only(void) {
    uint64_t w = 0x8000000000000000ULL;
    bignum_t n, e;
    make_bn(&n, &w, 1);
    make_bn(&e, (uint64_t[]){0x4000000000000000ULL}, 1);
    bignum_shift_right(&n, 1);
    return compare_bn(&n, &e);
}

/**
 * @brief      Тест: комбинированный сдвиг (слова + биты).
 * @return     1 в случае успеха, 0 в случае провала.
 */
static int test_combined(void) {
    uint64_t w[2] = {1,2};
    bignum_t n, e;
    make_bn(&n, w, 2);
    make_bn(&e, (uint64_t[]){1}, 1);
    bignum_shift_right(&n, 65);
    return compare_bn(&n, &e);
}

/**
 * @brief      Тест: сдвиг, превышающий емкость числа.
 * @return     1 в случае успеха, 0 в случае провала.
 */
static int test_overflow(void) {
    uint64_t w[2] = {0xDEAD,0xBEEF};
    bignum_t n, e;
    make_bn(&n, w, 2);
    make_bn(&e, NULL, 0);
    bignum_shift_right(&n, BIGNUM_CAPACITY*64 + 1);
    return (n.len == 0 && compare_bn(&n, &e));
}

/**
 * @brief      Фаззинг-тест против эталонной реализации GMP.
 * @return     1 в случае успеха, 0 в случае провала.
 */
static int test_fuzzing_for_correctness_vs_gmp(void) {
    const int N = 1000;
    gmp_randstate_t st;
    gmp_randinit_default(st);
    gmp_randseed_ui(st, (unsigned)time(NULL));

    mpz_t gv, gr, maxb, maxs, s;
    mpz_inits(gv, gr, maxb, maxs, s, NULL);
    mpz_ui_pow_ui(maxb, 2, BIGNUM_CAPACITY*64);
    mpz_set_ui(maxs, BIGNUM_CAPACITY*64 + 128);

    for (int i = 0; i < N; i++) {
        bignum_t bn, bn2;
        mpz_urandomm(gv, st, maxb);
        if (!bignum_from_gmp(&bn, gv)) continue;

        mpz_urandomm(s, st, maxs);
        size_t sh = mpz_get_ui(s);

        mpz_tdiv_q_2exp(gr, gv, sh);
        bignum_shift_right(&bn, sh);

        if (!bignum_from_gmp(&bn2, gr) || bn.len != bn2.len || memcmp(bn.words, bn2.words, bn.len*sizeof(uint64_t))!=0) {
            fprintf(stderr, "Fuzz fail at iter %d, shift=%zu\n", i, sh);
            print_bn("Got", &bn); print_bn("Exp", &bn2);
            mpz_clears(gv,gr,maxb,maxs,s,NULL); gmp_randclear(st); return 0;
        }
    }
    mpz_clears(gv,gr,maxb,maxs,s,NULL); gmp_randclear(st);
    printf("Fuzzing passed %d iterations\n", N);
    return 1;
}

/** @brief Структура для передачи данных в поток. */
typedef struct {
    bignum_t base;
    size_t shift;
    bignum_t expect;
} thr_arg_t;

/** @brief Рабочая функция потока для теста на потокобезопасность. */
static void* thr_func(void *vp) {
    thr_arg_t *a = vp;
    bignum_t tmp = a->base;
    bignum_shift_right(&tmp, a->shift);
    if (!compare_bn(&tmp, &a->expect)) {
        print_bn("Thread got", &tmp); print_bn("Exp     ", &a->expect);
        pthread_exit((void*)1);
    }
    pthread_exit((void*)0);
}

/**
 * @brief      Тест на потокобезопасность.
 * @details    Запускает несколько потоков, каждый из которых выполняет
 *             сдвиг над своим собственным экземпляром bignum_t.
 * @return     1 в случае успеха, 0 в случае провала.
 */
static int test_threads(void) {
    uint64_t w[2] = {0xF0F0F0F0F0F0F0F0ULL, 0x0F0F0F0F0F0F0F0FULL};
    bignum_t expect;
    make_bn(&expect, (uint64_t[]){0xFF0F0F0F0F0F0F0FULL, 0x00F0F0F0F0F0F0F0ULL}, 2);

    thr_arg_t args[4];
    pthread_t th[4];
    for (int i = 0; i < 4; i++) {
        make_bn(&args[i].base, w, 2);
        args[i].shift = 4;
        args[i].expect = expect;
        pthread_create(&th[i], NULL, thr_func, &args[i]);
    }
    for (int i = 0; i < 4; i++) {
        void *res;
        pthread_join(th[i], &res);
        if ((intptr_t)res != 0) return 0;
    }
    return 1;
}

/* === main === */
int main(void) {
    test_summary_t sum = {0,0};
    printf("Starting extra tests for bignum_shift_right rev.10\n\n");
    RUN_TEST(sum, test_null_argument_handling);
    RUN_TEST(sum, test_inplace_shift_overlap);
    RUN_TEST(sum, test_edge_case_shifts);
    RUN_TEST(sum, test_zero);
    RUN_TEST(sum, test_word_only);
    RUN_TEST(sum, test_bit_only);
    RUN_TEST(sum, test_combined);
    RUN_TEST(sum, test_overflow);
    RUN_TEST(sum, test_fuzzing_for_correctness_vs_gmp);
    RUN_TEST(sum, test_threads);

    printf("========================================\n");
    printf("Summary: %d/%d tests passed\n", sum.passed, sum.total);
    printf("========================================\n");
    return (sum.passed == sum.total) ? 0 : 1;
}
