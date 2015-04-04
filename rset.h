#ifndef rset_H_
#define rset_H_

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
 * ints or a bitset, whichever uses the least amount of space. The cut-off
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
} rset_t;

/**
 * Create a new set.
 */

rset_t *rset_new(void);

/**
 * Free the specified set.
 */

void rset_free(rset_t *set);

/**
 * Get the cardinality of the set.
 */

unsigned rset_cardinality(const rset_t *set);

/**
 * Add an item to the set.
 *
 * Returns true if the operation was successful and false otherwise.
 */

bool rset_add(rset_t *set, uint16_t item);

/**
 * Check if the set contains an item.
 */

bool rset_contains(const rset_t *set, uint16_t item);

/**
 * Check if two sets are equal.
 */

bool rset_equals(const rset_t *set, const rset_t *comparison);

/**
 * Invert the set and place the result in the `result` set.
 *
 * Returns true if the operation was successful and false otherwise.
 */

bool rset_invert(const rset_t *set, rset_t *result);

/**
 * Calculate the intersection of two sets and place the result in
 * the `result` set.
 *
 * Returns true if the operation was successful and false otherwise.
 */
bool rset_intersection(const rset_t *a, const rset_t *b, rset_t *result);

/**
 * Truncate the set.
 */

bool rset_truncate(rset_t *set);

/**
 * Fill the set with all possible items.
 */

bool rset_fill(rset_t *set);

/**
 * Export the set.
 *
 * The length of the buffer can be obtained with `rset_length`.
 */

const void *rset_export(const rset_t *set);

/**
 * Get the length of the set in bytes.
 */

unsigned rset_length(const rset_t *set);

/**
 * Import a set given a buffer.
 */

rset_t *rset_import(const void *buffer, unsigned length);

/**
 * Make a copy of the set.
 */

rset_t *rset_copy(const rset_t *set);

#ifdef __cplusplus
}
#endif

#endif
