#ifndef RBIT_H_
#define RBIT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * A C implementation of Daniel Lemire (et. al.)'s Roaring Bitmaps.
 *
 * The items are layed in memory as follows:
 *
 *     buffer ptr
 *     v
 *     |  cardinality  |   items...  |
 *
 * The set items are represented by either a sorted array of 16-bit unsigned
 * ints, or a bitset, whichever uses the least amount of space. The cut-off
 * point is 4096 (2^12) items (8KB).
 *
 * When the bitset becomes very dense it is more efficient to store an
 * inverted array, where each item represents an unsigned int that is *not* in
 * the set. The cut-off point is 61440 (2^16-2^12) items (8KB).
 *
 * See the original paper for more information:
 *
 *     http://arxiv.org/pdf/1402.6407v4.pdf
 */

typedef struct {
    uint16_t *buffer;
    unsigned size;
} rbit_t;

/**
 * Create a new set.
 */

rbit_t *rbit_new(void);

/**
 * Free the specified set.
 */

void rbit_free(rbit_t *set);

/**
 * Get the cardinality of the set.
 */

unsigned rbit_cardinality(const rbit_t *set);

/**
 * Add an item to the set.
 *
 * Returns true if the operation was successful and false otherwise.
 */

bool rbit_add(rbit_t *set, uint16_t item);

/**
 * Check if the set contains an item.
 */

bool rbit_contains(const rbit_t *set, uint16_t item);

/**
 * Check if two sets are equal.
 */

bool rbit_equals(const rbit_t *set, const rbit_t *comparison);

/**
 * Truncate the set.
 */

void rbit_truncate(rbit_t *set);

/**
 * Export the set.
 *
 * The length of the buffer can be obtained with `rbit_length`.
 */

const void *rbit_export(const rbit_t *set);

/**
 * Get the length of the set in bytes.
 */

unsigned rbit_length(const rbit_t *set);

/**
 * Import a set given a buffer.
 */

rbit_t *rbit_import(const void *buffer, unsigned length);

/**
 * Make a copy of the set.
 */

rbit_t *rbit_copy(const rbit_t *set);

#ifdef __cplusplus
}
#endif

#endif
