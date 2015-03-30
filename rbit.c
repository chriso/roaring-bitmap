#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "rbit.h"

#define NOINLINE __attribute__ ((noinline))
#define INLINE __attribute__ ((always_inline))
#define UNLIKELY(x) __builtin_expect((x), 0)

const static unsigned default_size = 8;
const static unsigned growth_factor = 2;

const static unsigned max_items = 1 << 16;
const static unsigned low_cutoff = 1 << 12;
const static unsigned high_cutoff = max_items - low_cutoff;

const static unsigned empty_marker_a = high_cutoff + 1;
const static unsigned empty_marker_b = max_items - 1;

unsigned rbit_cardinality(const rbit_t *set)
{
    unsigned cardinality = set->buffer[0];
    if (!cardinality)
        return max_items;
    if (cardinality == empty_marker_a && set->buffer[1] == empty_marker_b)
        return 0;
    return cardinality;
}

void rbit_truncate(rbit_t *set)
{
    set->buffer[0] = empty_marker_a;
    set->buffer[1] = empty_marker_b;
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
        cardinality = max_items - cardinality;
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

static bool NOINLINE rbit_convert_array_to_bitset(rbit_t *set, uint16_t item)
{
    if (set->buffer[low_cutoff] >= item)
        return false;
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

static bool NOINLINE rbit_convert_bitset_to_inverted_array(rbit_t *set,
                                                           uint16_t item)
{
    if (set->buffer[(item >> 4) + 1] & (1 << (item & 0xF)))
        return false;
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
    unsigned cardinality = rbit_cardinality(set);
    if (!cardinality)
        set->buffer[0] = 0;
    else if (set->buffer[cardinality] >= item)
        return false;
    else if (cardinality == set->size && !rbit_grow(set))
        return false;
    set->buffer[cardinality + 1] = item;
    return true;
}

static bool INLINE rbit_add_bitset(rbit_t *set, uint16_t item)
{
    unsigned offset = (item >> 4) + 1;
    unsigned bit = 1 << (item & 0xF);
    if (set->buffer[offset] & bit)
        return false;
    set->buffer[offset] |= bit;
    return true;
}

static bool INLINE rbit_add_inverted_array(rbit_t *set, uint16_t item)
{
    unsigned cardinality = *set->buffer;
    for (unsigned i = 1; i <= cardinality; i++) {
        if (set->buffer[i] < item)
            continue;
        if (set->buffer[i] > item)
            break;
        memmove(set->buffer + i,
                set->buffer + i + 1,
                (max_items - cardinality - i) * sizeof(uint16_t));
        return true;
    }
    return false;
}

bool rbit_add(rbit_t *set, uint16_t item)
{
    unsigned cardinality = rbit_cardinality(set);
    if (UNLIKELY(cardinality == max_items))
        return false;

    // convert the structure if necessary
    if (UNLIKELY(cardinality == low_cutoff)) {
        if (!rbit_convert_array_to_bitset(set, item))
            return false;
    } else if (UNLIKELY(cardinality == high_cutoff)) {
        if (!rbit_convert_bitset_to_inverted_array(set, item))
            return false;
    }

    // add the item
    if (cardinality < low_cutoff) {
        if (!rbit_add_array(set, item))
            return false;
    } else if (cardinality >= high_cutoff) {
        if (!rbit_add_inverted_array(set, item))
            return false;
    } else if (!rbit_add_bitset(set, item))
        return false;

    // increment cardinality
    *set->buffer += 1;
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
