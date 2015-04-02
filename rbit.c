#include <stdlib.h>
#include <string.h>

#include "rbit.h"

#define NOINLINE __attribute__ ((noinline))
#define INLINE __attribute__ ((always_inline))
#define UNLIKELY(x) __builtin_expect((x), 0)

const static unsigned default_size = 8;
const static unsigned growth_factor = 2;

const static unsigned max_cardinality = 1 << 16;
const static unsigned low_cutoff = 1 << 12;
const static unsigned high_cutoff = max_cardinality - low_cutoff;
const static unsigned max_item = 0xFFFF;

static bool INLINE rbit_is_empty(const rbit_t *set)
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

unsigned rbit_cardinality(const rbit_t *set)
{
    if (!*set->buffer)
        return max_cardinality;
    if (rbit_is_empty(set))
        return 0;
    return *set->buffer;
}

void rbit_truncate(rbit_t *set)
{
    set->buffer[0] = 2;
    set->buffer[1] = max_item;
}

rbit_t *rbit_import(const void *buffer, unsigned length)
{
    rbit_t *set = malloc(sizeof(rbit_t));
    if (!set)
        return NULL;
    unsigned size = length ? length : 1;
    if (size > low_cutoff)
        size = low_cutoff;
    set->buffer = malloc(sizeof(uint16_t) * (1 + size));
    if (!set->buffer) {
        free(set);
        return NULL;
    }
    set->size = size;
    if (buffer && length)
        memcpy(set->buffer, buffer, length);
    else
        rbit_truncate(set);
    return set;
}

rbit_t *rbit_new()
{
    return rbit_import(NULL, default_size);
}

void rbit_free(rbit_t *set)
{
    free(set->buffer);
    free(set);
}

const void *rbit_export(const rbit_t *set)
{
    return set->buffer;
}

rbit_t *rbit_copy(const rbit_t *set)
{
    return rbit_import(rbit_export(set), rbit_length(set));
}

static unsigned INLINE rbit_length_for(unsigned cardinality)
{
    if (!cardinality)
        cardinality = 1;
    else if (cardinality >= high_cutoff)
        cardinality = max_cardinality - cardinality;
    else if (cardinality > low_cutoff)
        cardinality = low_cutoff;
    return sizeof(uint16_t) * cardinality;
}

unsigned rbit_length(const rbit_t *set)
{
    return sizeof(uint16_t) + rbit_length_for(rbit_cardinality(set));
}

static bool NOINLINE rbit_grow(rbit_t *set)
{
    unsigned new_size = set->size * growth_factor;
    if (new_size > low_cutoff)
        new_size = low_cutoff;
    uint16_t *buffer = realloc(set->buffer, sizeof(uint16_t) * (1 + new_size));
    if (!buffer)
        return false;
    set->buffer = buffer;
    set->size = new_size;
    return true;
}

static bool NOINLINE rbit_convert_array_to_bitset(rbit_t *set)
{
    uint16_t *bitset = calloc(low_cutoff, sizeof(uint16_t));
    uint16_t *array = set->buffer + 1;
    if (!bitset)
        return false;
    for (unsigned i = 0; i < low_cutoff; i++)
        bitset[array[i] >> 4] |= 1 << (array[i] & 0xF);
    memcpy(array, bitset, low_cutoff * sizeof(uint16_t));
    free(bitset);
    return true;
}

static bool NOINLINE rbit_convert_bitset_to_inverted_array(rbit_t *set)
{
    uint16_t *array = calloc(low_cutoff, sizeof(uint16_t));
    if (!array)
        return false;
    uint16_t *ptr = array, *bitset = set->buffer + 1;
    for (unsigned bit = 0, i = 0; i < low_cutoff; i++)
        for (unsigned j = 0; j < 16; j++, bit++)
            if (!(bitset[i] & (1 << j)))
                *ptr++ = bit;
    memcpy(bitset, array, low_cutoff * sizeof(uint16_t));
    free(array);
    return true;
}

static bool INLINE rbit_add_array(rbit_t *set, uint16_t item)
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
    if (UNLIKELY(cardinality == set->size && !rbit_grow(set)))
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

static bool INLINE rbit_add_bitset(rbit_t *set, uint16_t item)
{
    unsigned offset = (item >> 4) + 1;
    unsigned bit = 1 << (item & 0xF);
    if (!(set->buffer[offset] & bit)) {
        set->buffer[offset] |= bit;
        *set->buffer += 1;
    }
    return true;
}

static bool INLINE rbit_add_inverted_array(rbit_t *set, uint16_t item)
{
    unsigned cardinality = max_cardinality - *set->buffer;
    if (set->buffer[cardinality] == item) {
        *set->buffer += 1;
        return true;
    }
    for (unsigned i = 1; i <= cardinality; i++) {
        if (set->buffer[i] < item)
            continue;
        if (set->buffer[i] > item)
            break;
        memmove(set->buffer + i,
                set->buffer + i + 1,
                (cardinality - i) * sizeof(uint16_t));
        *set->buffer += 1;
        return true;
    }
    return true;
}

static bool INLINE rbit_contains_array(const rbit_t *set, uint16_t item)
{
    unsigned cardinality = *set->buffer;
    for (unsigned i = 1; i <= cardinality; i++)
        if (set->buffer[i] == item)
            return true;
    return false;
}

static bool INLINE rbit_contains_bitset(const rbit_t *set, uint16_t item)
{
    return set->buffer[(item >> 4) + 1] & (1 << (item & 0xF));
}

static bool INLINE rbit_contains_inverted_array(const rbit_t *set,
                                                uint16_t item)
{
    unsigned cardinality = max_cardinality - *set->buffer;
    for (unsigned i = 1; i <= cardinality; i++)
        if (set->buffer[i] == item)
            return false;
    return true;
}

bool rbit_add(rbit_t *set, uint16_t item)
{
    unsigned cardinality = *set->buffer;
    if (UNLIKELY(!cardinality))
        return true;

    if (UNLIKELY(rbit_is_empty(set))) {
        cardinality = *set->buffer = 0;
    } else if (UNLIKELY(cardinality == low_cutoff)) {
        if (rbit_contains_array(set, item))
            return true;
        if (!rbit_convert_array_to_bitset(set))
            return false;
    } else if (UNLIKELY(cardinality == high_cutoff)) {
        if (rbit_contains_bitset(set, item))
            return true;
        if (!rbit_convert_bitset_to_inverted_array(set))
            return false;
    }

    if (cardinality < low_cutoff) {
        if (!rbit_add_array(set, item))
            return false;
    } else if (cardinality >= high_cutoff) {
        if (!rbit_add_inverted_array(set, item))
            return false;
    } else if (!rbit_add_bitset(set, item))
        return false;

    return true;
}

bool rbit_equals(const rbit_t *set, const rbit_t *comparison)
{
    unsigned cardinality = rbit_cardinality(set);
    if (cardinality != rbit_cardinality(comparison))
        return false;
    unsigned length = rbit_length_for(cardinality);
    return !length || !memcmp(set->buffer + 1, comparison->buffer + 1, length);
}

bool rbit_contains(const rbit_t *set, uint16_t item)
{
    unsigned cardinality = *set->buffer;
    if (UNLIKELY(!cardinality))
        return true;
    if (UNLIKELY(rbit_is_empty(set)))
        return false;
    if (cardinality <= low_cutoff)
        return rbit_contains_array(set, item);
    if (cardinality > high_cutoff)
        return rbit_contains_inverted_array(set, item);
    return rbit_contains_bitset(set, item);
}
