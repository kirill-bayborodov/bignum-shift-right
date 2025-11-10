/**
 * @file    test_bignum_shift_right_mt.c
 * @author  git@bayborodov.com
 * @version 1.0.0
 * @date    10.11.2025
 *
 * @brief   Тест на потокобезопасность для функции bignum_shift_right.
 *
 * @details
 *   Этот тест доказывает, что функция `bignum_shift_right` является
 *   потокобезопасной. Доказательство строится на том, что функция
 *   не использует глобальное или статическое состояние.
 *
 *   Алгоритм теста:
 *   1. Создается несколько потоков.
 *   2. Каждому потоку выделяется свой, уникальный и непересекающийся
 *      экземпляр `bignum_t` и уникальные параметры для сдвига.
 *   3. Ожидаемый результат для каждого потока рассчитывается с помощью
 *      независимой эталонной библиотеки GMP.
 *   4. Каждый поток выполняет операцию сдвига и сравнивает свой
 *      результат с эталонным.
 *   5. Главный поток ожидает завершения всех дочерних потоков и
 *      агрегирует их результаты.
 *
 * @note
 *   Для сборки этого теста требуется флаг `-pthread` и библиотека GMP (`-lgmp`).
 *
 * @history
 *   - rev. 1 (10.08.2025): Первоначальное создание с самопроверкой.
 *   - rev. 2 (10.08.2025): Тест усилен. Добавлена верификация через GMP,
 *                         но допущена ошибка - пропущен #include <stdlib.h>.
 *   - rev. 3 (10.08.2025): Исправлена ошибка компиляции. Добавлен #include <stdlib.h>.
 */

#include "bignum_shift_right.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <stdlib.h> // Для free()
#include <gmp.h>

#define NUM_THREADS 8
#define NUM_ITERATIONS_PER_THREAD 100
#define TEST_PASSED ((void*)1)
#define TEST_FAILED ((void*)0)

/**
 * @brief Структура для передачи данных в поток.
 */
typedef struct {
    bignum_t num;       /**< Исходное число для сдвига. */
    size_t shift_amount;/**< Величина сдвига. */
    bignum_t expected;  /**< Ожидаемый результат (рассчитан через GMP). */
    int thread_id;      /**< Идентификатор потока для логирования. */
} thread_data_t;

// Вспомогательная функция из другого теста для конвертации
int bignum_from_gmp(bignum_t* num, const mpz_t gmp_val) {
    memset(num, 0, sizeof(bignum_t));
    size_t num_limbs;
    uint64_t* limbs = (uint64_t*)mpz_export(NULL, &num_limbs, -1, sizeof(uint64_t), 0, 0, gmp_val);
    if (num_limbs > BIGNUM_CAPACITY) {
        if (limbs) free(limbs);
        return 0;
    }
    if (limbs) {
        num->len = num_limbs;
        memcpy(num->words, limbs, num_limbs * sizeof(uint64_t));
        free(limbs);
    } else {
        num->len = 0;
    }
    return 1;
}

/**
 * @brief      Функция, выполняемая в каждом потоке.
 * @details    Выполняет сдвиг и сравнивает результат с ожидаемым.
 * @param[in]  arg Указатель на структуру thread_data_t.
 * @return     TEST_PASSED в случае успеха, TEST_FAILED в случае провала.
 */
void* shift_worker(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;

    for (int i = 0; i < NUM_ITERATIONS_PER_THREAD; ++i) {
        bignum_t current_num = data->num; // Работаем с копией
        bignum_shift_right_status_t status = bignum_shift_right(&current_num, data->shift_amount);

        if (data->expected.len == 0) {
            if (status != BIGNUM_SHIFT_RIGHT_ZEROED && status != BIGNUM_SHIFT_RIGHT_SUCCESS) return TEST_FAILED;
        } else {
            if (status != BIGNUM_SHIFT_RIGHT_SUCCESS) return TEST_FAILED;
        }

        if (current_num.len != data->expected.len) return TEST_FAILED;
        if (memcmp(current_num.words, data->expected.words, data->expected.len * sizeof(uint64_t)) != 0) return TEST_FAILED;
        for (int j = current_num.len; j < BIGNUM_CAPACITY; ++j) {
            if (current_num.words[j] != 0) return TEST_FAILED;
        }
    }
    return TEST_PASSED;
}

int main() {
    pthread_t threads[NUM_THREADS];
    thread_data_t data[NUM_THREADS] = {0};
    int tests_passed = 0;

    printf("Starting multithreading test for bignum_shift_right (rev. 10)...\n");
    printf("Creating %d threads, each running %d iterations...\n\n", NUM_THREADS, NUM_ITERATIONS_PER_THREAD);

    mpz_t gmp_num, gmp_res;
    mpz_inits(gmp_num, gmp_res, NULL);

    // Инициализация данных для каждого потока
    for (int i = 0; i < NUM_THREADS; ++i) {
        data[i].thread_id = i;
        data[i].num.len = 2;
        data[i].num.words[0] = 0x1111111111111111ULL * (i + 1);
        data[i].num.words[1] = 0x2222222222222222ULL * (i + 1);
        data[i].shift_amount = 4 * (i + 1);

        // Рассчитываем ожидаемый результат через GMP
        mpz_import(gmp_num, data[i].num.len, -1, sizeof(uint64_t), 0, 0, data[i].num.words);
        mpz_tdiv_q_2exp(gmp_res, gmp_num, data[i].shift_amount);
        assert(bignum_from_gmp(&data[i].expected, gmp_res) && "GMP conversion failed");
    }
    mpz_clears(gmp_num, gmp_res, NULL);

    // Создание потоков
    for (int i = 0; i < NUM_THREADS; ++i) {
        if (pthread_create(&threads[i], NULL, shift_worker, &data[i]) != 0) {
            perror("pthread_create");
            return 1;
        }
    }

    // Ожидание завершения потоков
    for (int i = 0; i < NUM_THREADS; ++i) {
        void* result;
        if (pthread_join(threads[i], &result) != 0) {
            perror("pthread_join");
            return 1;
        }
        if (result == TEST_PASSED) {
            tests_passed++;
            printf("Thread %d: PASSED\n", i);
        } else {
            printf("Thread %d: FAILED\n", i);
        }
    }

    printf("\n----------------------------------------\n");
    printf("Multithreading test summary: %d/%d threads passed.\n", tests_passed, NUM_THREADS);
    printf("----------------------------------------\n");

    return (tests_passed == NUM_THREADS) ? 0 : 1;
}
