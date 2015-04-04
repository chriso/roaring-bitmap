#include <stdlib.h>
#include <string.h>

#include "rset.h"

#define NOINLINE __attribute__ ((noinline))
#define INLINE __attribute__ ((always_inline))
#define UNLIKELY(x) __builtin_expect((x), 0)
#define MAX(a, b) ((a > b) ? (a) : (b))

const static unsigned default_size = 8;
const static unsigned growth_factor = 2;

const static unsigned max_cardinality = 1 << 16;
const static unsigned low_cutoff = 1 << 12;
const static unsigned high_cutoff = max_cardinality - low_cutoff;
const static unsigned max_item = 0xFFFF;
const static unsigned max_size = low_cutoff;

static bool INLINE rset_is_empty(const rset_t *set)
{
    // There are 65536 possible items in the set (0-65535 inclusive) and then
    // the set can be empty, so there are 65537 (2^16+1) possible states. Since
    // the cardinality is stored in a uint16_t field with only 2^16 possible
    // states, it's necessary to use some other means of representing either an
    // empty set or a full set.
    //
    // An empty set is represented in 32-bits by storing a cardinality of two
    // and then storing the maximum item in the first slot. Since the set uses
    // a sorted array when cardinality < low_cutoff, there is no possible
    // second item that could be greater than the item in the first slot, i.e.
    // the state is invalid.

    return set->buffer[0] == 2 && set->buffer[1] == max_item;
}

static bool INLINE rset_is_full(const rset_t *set)
{
    return set->buffer[0] == 0;
}

static bool INLINE rset_is_bitset(const rset_t *set)
{
    unsigned cardinality = *set->buffer;
    return cardinality > low_cutoff && cardinality <= high_cutoff;
}

static bool INLINE rset_is_array(const rset_t *set)
{
    unsigned cardinality = *set->buffer;
    return cardinality <= low_cutoff;
}

static bool INLINE rset_is_inverted_array(const rset_t *set)
{
    unsigned cardinality = *set->buffer;
    return cardinality > high_cutoff;
}

unsigned rset_cardinality(const rset_t *set)
{
    if (rset_is_full(set))
        return max_cardinality;
    if (rset_is_empty(set))
        return 0;
    return *set->buffer;
}

bool rset_truncate(rset_t *set)
{
    set->buffer[0] = 2;
    set->buffer[1] = max_item;
    return true;
}

bool rset_fill(rset_t *set)
{
    set->buffer[0] = 0;
    return true;
}

rset_t *rset_import(const void *buffer, unsigned length)
{
    rset_t *set = malloc(sizeof(rset_t));
    if (!set)
        return NULL;
    unsigned size = length ? length : 1;
    if (size > max_size)
        size = max_size;
    set->buffer = malloc(sizeof(uint16_t) * (1 + size));
    if (!set->buffer) {
        free(set);
        return NULL;
    }
    set->size = size;
    if (buffer && length)
        memcpy(set->buffer, buffer, length);
    else
        rset_truncate(set);
    return set;
}

rset_t *rset_new()
{
    return rset_import(NULL, default_size);
}

void rset_free(rset_t *set)
{
    free(set->buffer);
    free(set);
}

const void *rset_export(const rset_t *set)
{
    return set->buffer;
}

rset_t *rset_copy(const rset_t *set)
{
    return rset_import(rset_export(set), rset_length(set));
}

static unsigned INLINE rset_length_for(unsigned cardinality)
{
    if (!cardinality)
        cardinality = 1;
    else if (cardinality >= high_cutoff)
        cardinality = max_cardinality - cardinality;
    else if (cardinality > low_cutoff)
        cardinality = low_cutoff;
    return sizeof(uint16_t) * cardinality;
}

unsigned rset_length(const rset_t *set)
{
    return sizeof(uint16_t) + rset_length_for(rset_cardinality(set));
}

static bool rset_grow_to(rset_t *set, unsigned size)
{
    if (set->size >= size)
        return true;
    uint16_t *buffer = realloc(set->buffer, sizeof(uint16_t) * (1 + size));
    if (!buffer)
        return false;
    set->buffer = buffer;
    set->size = size;
    return true;
}

static bool NOINLINE rset_grow(rset_t *set)
{
    unsigned size = set->size * growth_factor;
    if (size > max_size)
        size = max_size;
    return rset_grow_to(set, size);
}

static bool NOINLINE rset_convert_array_to_bitset(rset_t *set)
{
    uint16_t *bitset = calloc(max_size, sizeof(uint16_t));
    uint16_t *array = set->buffer + 1;
    if (!bitset)
        return false;
    for (unsigned i = 0; i < max_size; i++)
        bitset[array[i] >> 4] |= 1 << (array[i] & 0xF);
    memcpy(array, bitset, max_size * sizeof(uint16_t));
    free(bitset);
    return true;
}

static bool NOINLINE rset_convert_bitset_to_inverted_array(rset_t *set)
{
    uint16_t *array = calloc(max_size, sizeof(uint16_t));
    if (!array)
        return false;
    uint16_t *ptr = array, *bitset = set->buffer + 1;
    for (unsigned bit = 0, i = 0; i < max_size; i++)
        for (unsigned j = 0; j < 16; j++, bit++)
            if (!(bitset[i] & (1 << j)))
                *ptr++ = bit;
    memcpy(bitset, array, max_size * sizeof(uint16_t));
    free(array);
    return true;
}

static bool INLINE rset_add_array(rset_t *set, uint16_t item)
{
    unsigned i, cardinality = *set->buffer;
    if (cardinality && set->buffer[cardinality] < item) {
        i = cardinality + 1;
    } else {
        for (i = 1; i <= cardinality; i++) {
            if (set->buffer[i] < item)
                continue;
            if (set->buffer[i] == item)
                return true;
            break;
        }
    }
    if (UNLIKELY(cardinality == set->size && !rset_grow(set)))
        return false;
    if (cardinality + 1 > i) {
        memmove(set->buffer + i + 1,
                set->buffer + i,
                (cardinality + 1 - i) * sizeof(uint16_t));
    }
    set->buffer[i] = item;
    *set->buffer += 1;
    return true;
}

static bool INLINE rset_add_bitset(rset_t *set, uint16_t item)
{
    unsigned offset = (item >> 4) + 1;
    unsigned bit = 1 << (item & 0xF);
    if (!(set->buffer[offset] & bit)) {
        set->buffer[offset] |= bit;
        *set->buffer += 1;
    }
    return true;
}

static bool INLINE rset_add_inverted_array(rset_t *set, uint16_t item)
{
    unsigned cardinality = max_cardinality - *set->buffer;
    if (set->buffer[cardinality] == item) {
        *set->buffer += 1;
        return true;
    }
    uint16_t *array = set->buffer + 1;
    for (unsigned i = 0; i < cardinality; i++) {
        if (array[i] < item)
            continue;
        if (array[i] > item)
            break;
        memmove(array + i, array + i + 1,
                (cardinality - i) * sizeof(uint16_t));
        *set->buffer += 1;
        return true;
    }
    return true;
}

static bool INLINE rset_contains_array(const rset_t *set, uint16_t item)
{
    unsigned cardinality = *set->buffer;
    if (cardinality > high_cutoff)
        cardinality = max_cardinality - cardinality;
    uint16_t *array = set->buffer + 1;
    int first = 0, last = cardinality - 1, middle = (first + last) / 2;
    while (first <= last) {
        if (array[middle] == item)
            return true;
        if (array[middle] < item)
            first = middle + 1;
        else
            last = middle - 1;
        middle = (first + last) / 2;
    }
    return false;
}

static bool INLINE rset_contains_bitset(const rset_t *set, uint16_t item)
{
    return set->buffer[(item >> 4) + 1] & (1 << (item & 0xF));
}

bool rset_add(rset_t *set, uint16_t item)
{
    if (UNLIKELY(rset_is_full(set)))
        return true;

    if (UNLIKELY(rset_is_empty(set)))
        *set->buffer = 0;

    unsigned cardinality = *set->buffer;
    if (UNLIKELY(cardinality == low_cutoff)) {
        if (rset_contains_array(set, item))
            return true;
        if (!rset_convert_array_to_bitset(set))
            return false;
    } else if (UNLIKELY(cardinality == high_cutoff)) {
        if (rset_contains_bitset(set, item))
            return true;
        if (!rset_convert_bitset_to_inverted_array(set))
            return false;
    }

    if (cardinality < low_cutoff) {
        if (!rset_add_array(set, item))
            return false;
    } else if (cardinality >= high_cutoff) {
        if (!rset_add_inverted_array(set, item))
            return false;
    } else if (!rset_add_bitset(set, item))
        return false;

    return true;
}

bool rset_equals(const rset_t *set, const rset_t *comparison)
{
    unsigned cardinality = rset_cardinality(set);
    if (cardinality != rset_cardinality(comparison))
        return false;
    unsigned length = rset_length_for(cardinality);
    return !length || !memcmp(set->buffer + 1, comparison->buffer + 1, length);
}

bool rset_contains(const rset_t *set, uint16_t item)
{
    if (rset_is_full(set))
        return true;
    if (rset_is_empty(set))
        return false;
    if (rset_is_array(set))
        return rset_contains_array(set, item);
    if (rset_is_inverted_array(set))
        return !rset_contains_array(set, item);
    return rset_contains_bitset(set, item);
}

static bool rset_copy_to(const rset_t *set, rset_t *dest)
{
    if (!rset_grow_to(dest, set->size))
        return false;
    memcpy(dest->buffer, set->buffer, rset_length(set));
    return true;
}

static void INLINE rset_invert_bitset(rset_t *set)
{
    uint16_t *bitset = set->buffer + 1;
    for (unsigned i = 0; i < max_size; i++)
        bitset[i] = ~bitset[i];
}

bool rset_invert(const rset_t *set, rset_t *result)
{
    if (rset_is_empty(set)) // ~0 => U
        return rset_fill(result);
    if (rset_is_full(set)) // ~U => 0
        return rset_truncate(result);
    if (!rset_copy_to(set, result))
        return false;
    *result->buffer = max_cardinality - *result->buffer;
    if (rset_is_bitset(result))
        rset_invert_bitset(result);
    return true;
}

static bool rset_intersection_array(const rset_t *a, const rset_t *b,
                                    rset_t *result)
{
    unsigned result_size = MAX(*a->buffer, *b->buffer);
    if (!rset_grow_to(result, result_size))
        return false;
    uint16_t *array_a = a->buffer + 1;
    uint16_t *array_b = b->buffer + 1;
    uint16_t *array_result = result->buffer + 1;
    const uint16_t* a_end = array_a + *a->buffer;
    const uint16_t* b_end = array_b + *b->buffer;
    while (array_a < a_end && array_b < b_end) {
        if (*array_a < *array_b) {
            array_a++;
        } else if (*array_b < *array_a) {
            array_b++;
        } else {
            *array_result++ = *array_a++;
            array_b++;
        }
    }
    *result->buffer = array_result - result->buffer - 1;
    if (!*result->buffer)
        rset_truncate(result);
    return true;
}

static unsigned rset_intersection_bitset(const rset_t *a, const rset_t *b,
                                         rset_t *result)
{
    uint16_t *bitset_a = a->buffer + 1;
    uint16_t *bitset_b = b->buffer + 1;
    uint16_t *bitset_result = result->buffer + 1;
    unsigned cardinality = 0;
    for (unsigned i = 0; i < max_size; i++) {
        bitset_result[i] = bitset_a[i] & bitset_b[i];
        cardinality += __builtin_popcount(bitset_result[i]);
    }
    return cardinality;
}

bool rset_intersection(const rset_t *a, const rset_t *b, rset_t *result)
{
    if (rset_is_empty(a) || rset_is_empty(b)) // A & 0 => 0
        return rset_truncate(result);
    if (rset_is_full(a)) // A & U => A
        return rset_copy_to(b, result);
    else if (rset_is_full(b))
        return rset_copy_to(a, result);
    if (rset_is_array(a) && rset_is_array(b))
        return rset_intersection_array(a, b, result);

    // TODO: convert both operands to bitsets if necessary

    if (!rset_grow_to(result, max_size))
        return false;
    unsigned cardinality = rset_intersection_bitset(a, b, result);

    if (!cardinality)
        return rset_truncate(result);
    if (cardinality == max_cardinality)
        return rset_fill(result);

    *result->buffer = cardinality;
    return true;

    // TODO: convert back to an array or inverted array as necessary

    return false;
}