#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include "rbit.h"

#define RBIT_INITIAL_SIZE 8
#define RBIT_GROWTH_FACTOR 8

#define RBIT_MAX (1 << 16)
#define RBIT_LOW_CUTOFF (RBIT_MAX / 8 / sizeof(uint16_t))
#define RBIT_HIGH_CUTOFF (uint16_t)(RBIT_MAX - RBIT_LOW_CUTOFF)

#define RBIT_MAGIC (RBIT_HIGH_CUTOFF + 1)

unsigned rbit_cardinality(const rbit_t *set)
{
    unsigned cardinality = set->buffer[0];
    if (!cardinality)
        return RBIT_MAX;
    if (cardinality == RBIT_MAGIC && set->buffer[1] == 0xFFFF)
        return 0;
    return cardinality;
}

static rbit_t *rbit_new_size(unsigned size)
{
    rbit_t *set = malloc(sizeof(rbit_t));
    if (!set)
        return NULL;
    if (!size)
        size += 1;
    set->buffer = malloc(sizeof(uint16_t) * (1 + size));
    if (!set->buffer) {
        free(set);
        return NULL;
    }
    set->size = size;
    set->buffer[0] = RBIT_MAGIC;
    set->buffer[1] = 0xFFFF;
    return set;
}

rbit_t *rbit_new()
{
    return rbit_new_size(RBIT_INITIAL_SIZE);
}

void rbit_free(rbit_t *set)
{
    free(set->buffer);
    free(set);
}

static unsigned rbit_length_for(unsigned cardinality)
{
    if (!cardinality)
        cardinality = 1;
    else if (cardinality >= RBIT_HIGH_CUTOFF)
        cardinality = RBIT_MAX - cardinality;
    else if (cardinality > RBIT_LOW_CUTOFF)
        cardinality = RBIT_LOW_CUTOFF;
    return sizeof(uint16_t) * cardinality;
}

unsigned rbit_length(const rbit_t *set)
{
    return sizeof(uint16_t) + rbit_length_for(rbit_cardinality(set));
}

static bool rbit_grow(rbit_t *set)
{
    unsigned new_size = set->size * 2;
    if (new_size > RBIT_LOW_CUTOFF)
        new_size = RBIT_LOW_CUTOFF;
    uint16_t *buffer = realloc(set->buffer, sizeof(uint16_t) * (1 + new_size));
    if (!buffer)
        return false;
    set->buffer = buffer;
    set->size = new_size;
    return true;
}

static bool rbit_array_to_bitset(rbit_t *set)
{
    uint16_t *bitset = calloc(RBIT_LOW_CUTOFF, sizeof(uint16_t));
    uint16_t *array = set->buffer + 1;
    if (!bitset)
        return false;
    for (unsigned i = 0; i < RBIT_LOW_CUTOFF; i++)
        bitset[array[i] / 16] |= 1U << (array[i] % 16);
    memcpy(array, bitset, RBIT_LOW_CUTOFF * sizeof(uint16_t));
    free(bitset);
    return true;
}

static bool rbit_bitset_to_inverted_array(rbit_t *set)
{
    uint16_t *array = calloc(RBIT_LOW_CUTOFF, sizeof(uint16_t));
    if (!array)
        return false;
    uint16_t *ptr = array, *bitset = set->buffer + 1;
    for (unsigned bit = 0, i = 0; i < RBIT_LOW_CUTOFF; i++)
        for (unsigned j = 0; j < 16; j++, bit++)
            if (!(bitset[i] & (1U << j)))
                *ptr++ = bit;
    memcpy(bitset, array, RBIT_LOW_CUTOFF * sizeof(uint16_t));
    free(array);
    return true;
}

static bool rbit_add_array(rbit_t *set, uint16_t item)
{
    unsigned cardinality = rbit_cardinality(set);
    if (!cardinality)
        set->buffer[0] = 0;
    else if (set->buffer[cardinality] >= item)
        return false;
    if (cardinality == set->size && !rbit_grow(set))
        return false;
    set->buffer[cardinality + 1] = item;
    return true;
}

static bool rbit_add_bitset(rbit_t *set, uint16_t item)
{
    unsigned offset = item / 16 + 1;
    uint16_t bit = 1U << (item % 16);
    if (set->buffer[offset] & bit)
        return false;
    set->buffer[offset] |= bit;
    return true;
}

static bool rbit_add_inverted_array(rbit_t *set, uint16_t item)
{
    unsigned cardinality = rbit_cardinality(set);
    for (unsigned i = 0; i < cardinality; i++) {
        if (set->buffer[i + 1] < item)
            continue;
        if (set->buffer[i + 1] > item)
            break;
        memmove(set->buffer + 1 + i, set->buffer + 1 + i + 1,
                ((1U << 16) - cardinality - i - 1) * sizeof(uint16_t));
        return true;
    }
    return false;
}

bool rbit_add(rbit_t *set, uint16_t item)
{
    unsigned cardinality = rbit_cardinality(set);
    if (cardinality == RBIT_MAX)
        return false;
    if (cardinality == RBIT_LOW_CUTOFF) {
        for (unsigned i = 1; i <= RBIT_LOW_CUTOFF; i++)
            if (set->buffer[i] == item)
                return false;
        if (!rbit_array_to_bitset(set))
            return false;
    } else if (cardinality == RBIT_HIGH_CUTOFF) {
        if (set->buffer[item / 16 + 1] & (1U << (item % 16)))
            return false;
        if (!rbit_bitset_to_inverted_array(set))
            return false;
    }
    if (cardinality < RBIT_LOW_CUTOFF) {
        if (!rbit_add_array(set, item))
            return false;
    } else if (cardinality >= RBIT_HIGH_CUTOFF) {
        if (!rbit_add_inverted_array(set, item))
            return false;
    } else if (!rbit_add_bitset(set, item))
        return false;
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

rbit_t *rbit_new_items(unsigned count, ...)
{
    rbit_t *set = rbit_new_size(count);
    if (!set)
        return NULL;
    va_list items;
    va_start(items, count);
    for (unsigned i = 0; i < count; i++)
        if (!rbit_add(set, (uint16_t)va_arg(items, unsigned)))
            goto error;
    va_end(items);
    return set;
error:
    rbit_free(set);
    return NULL;
}
