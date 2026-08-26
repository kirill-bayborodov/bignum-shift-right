/**
 * @file bignum_shift_right.c
 * @brief C11 reference implementation for bignum_shift_right.
 * @details Validates inputs, computes into a stack-local temporary, normalizes
 * the result, and publishes it only after successful completion. The function
 * is deterministic, allocation-free, and safe for independent concurrent calls.
 */
#include "bignum_shift_right.h"
#include <string.h>

bignum_shift_right_status_t bignum_shift_right(bignum_t *restrict num, size_t shift_amount)
{
    bignum_t tmp = {0};
    size_t word_shift, bit_shift;
    if (num == NULL) return BIGNUM_SHIFT_RIGHT_ERROR_NULL_ARG;
    if (shift_amount == 0U) return BIGNUM_SHIFT_RIGHT_SUCCESS;
    if (num->len == 0U) return BIGNUM_SHIFT_RIGHT_SUCCESS;
    if (num->len > BIGNUM_CAPACITY) return BIGNUM_SHIFT_RIGHT_ERROR_NULL_ARG;
    word_shift = shift_amount / 64U; bit_shift = shift_amount % 64U;
    if (word_shift >= num->len) { *num = tmp; return BIGNUM_SHIFT_RIGHT_ZEROED; }
    for (size_t i = word_shift; i < num->len; ++i) {
        size_t dst = i - word_shift;
        tmp.words[dst] = num->words[i] >> bit_shift;
        if (bit_shift != 0U && i + 1U < num->len)
            tmp.words[dst] |= num->words[i + 1U] << (64U - bit_shift);
    }
    tmp.len = num->len - word_shift;
    while (tmp.len > 0U && tmp.words[tmp.len - 1U] == 0U) --tmp.len;
    *num = tmp;
    return tmp.len == 0U ? BIGNUM_SHIFT_RIGHT_ZEROED : BIGNUM_SHIFT_RIGHT_SUCCESS;
}
