#ifndef RBIT_H_
#define RBIT_H_

/**
 * A C implementation of Daniel Lemire (et. al.)'s Roaring Bitmaps.
 *
 * The set is layed in memory as follows:
 *
 *     malloc ptr     rbit ptr
 *     v              v
 *     |     size     |  cardinality  |   items...  |
 *
 * The set items are represented by either a sorted array of 16-bit unsigned
 * ints, or a bitset, whichever uses the least amount of space. The cut-off
 * point is 4096 (2^12) items (8KB).
 *
 * When the bitset becomes very dense it is more efficient to store an
 * inverted array, where each item represents an unsigned int that is *not* in
 * the set. The cut-off point is 61440 (2^16-2^12) items (8KB).
 *
 * This implementation makes an additional optimization for small sets
 * by collapsing the cardinality field into the first item when the set size
 * is less than 32768 (2^15). The implementation also makes use of SSE
 * instructions where possible, e.g. the PCMPESTRM instruction for fast
 * intersections.
 *
 * See the original paper for more information:
 *
 *     http://arxiv.org/pdf/1402.6407v4.pdf
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef uint16_t rbit_t;

/**
 * Create a new set.
 */

rbit_t *rbit_new();

/**
 * Free the specified set.
 */

void rbit_free(rbit_t *rbit);

/**
 * Get the cardinality of the set.
 */

uint16_t rbit_cardinality(const rbit_t *rbit);

/**
 * Add an item to the set.
 *
 * The item *must* be greater than the last item added to the set. Behavior
 * when adding an item that is less than or equal to a previously added item
 * is undefined.
 *
 * Return true if the operation was successful and false otherwise.
 */

bool rbit_add(rbit_t *rbit, uint16_t item);

#ifdef __cplusplus
}
#endif

#endif
