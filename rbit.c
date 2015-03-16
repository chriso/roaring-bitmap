#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include "rbit.h"

#define RBIT_INITIAL_SIZE 16
#define RBIT_GROWTH_FACTOR 8

#define RBIT_LOW_CUTOFF (1U << 12)

static rbit_t *rbit_new_size(uint16_t size)
{
    rbit_t *set = malloc(sizeof(rbit_t));
    if (!set)
        return NULL;
    set->buffer = malloc(sizeof(uint16_t) * (1 + size));
    if (!set->buffer) {
        free(set);
        return NULL;
    }
    set->size = size;
    *set->buffer = 0;
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

uint16_t rbit_cardinality(const rbit_t *set)
{
    return *set->buffer;
}

static uint16_t rbit_length_for(uint16_t cardinality)
{
    return sizeof(uint16_t) * cardinality;
}

uint16_t rbit_length(const rbit_t *set)
{
    return sizeof(uint16_t) + rbit_length_for(*set->buffer);
}

static bool rbit_grow(rbit_t *set)
{
    size_t new_size = set->size * 2;
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
    if (!bitset)
        return false;
    for (size_t i = 1; i <= RBIT_LOW_CUTOFF; i++)
        bitset[set->buffer[i] / 16] |= 1U << (set->buffer[i] % 16);
    memcpy(set->buffer + 1, bitset, RBIT_LOW_CUTOFF * sizeof(uint16_t));
    free(bitset);
    return true;
}

static bool rbit_add_array(rbit_t *set, uint16_t item)
{
    uint16_t cardinality = *set->buffer;
    if (cardinality && set->buffer[cardinality] >= item)
        return false;
    if (cardinality == set->size && !rbit_grow(set))
        return false;
    set->buffer[cardinality + 1] = item;
    return true;
}

static bool rbit_add_bitset(rbit_t *set, uint16_t item)
{
    uint16_t offset = item / 16;
    uint16_t bit = item % 16;
    if (set->buffer[offset + 1] & (1U << bit))
        return false;
    set->buffer[offset + 1] |= 1U << bit;
    return true;
}

bool rbit_add(rbit_t *set, uint16_t item)
{
    uint16_t cardinality = *set->buffer;
    if (cardinality == RBIT_LOW_CUTOFF)
        if (!rbit_array_to_bitset(set))
            return false;
    if (cardinality < RBIT_LOW_CUTOFF) {
        if (!rbit_add_array(set, item))
            return false;
    } else {
        if (!rbit_add_bitset(set, item))
            return false;
    }
    *set->buffer = cardinality + 1;
    return true;
}

bool rbit_equals(const rbit_t *set, const rbit_t *comparison)
{
    uint16_t cardinality = *set->buffer;
    if (cardinality != *comparison->buffer)
        return false;
    uint16_t length = rbit_length_for(cardinality);
    return !memcmp(set->buffer + 1, comparison->buffer + 1, length);
}

rbit_t *rbit_new_items(uint16_t count, ...)
{
    rbit_t *set = rbit_new_size(count);
    if (!set)
        return NULL;
    va_list items;
    va_start(items, count);
    for (size_t i = 0; i < count; i++)
        if (!rbit_add(set, (uint16_t)va_arg(items, unsigned)))
            goto error;
    va_end(items);
    return set;
error:
    rbit_free(set);
    return NULL;
}
