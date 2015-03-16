#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include "rbit.h"

#define RBIT_INITIAL_SIZE 16

static void *rbit_to_malloc_ptr(rbit_t *set)
{
    return set - 1;
}

static void *rbit_from_malloc_ptr(void *ptr)
{
    return (uint16_t *)ptr + 1;
}

static rbit_t *rbit_new_size(uint16_t size)
{
    uint16_t *ptr = calloc(2 + size, sizeof(uint16_t));
    *ptr = RBIT_INITIAL_SIZE;
    return rbit_from_malloc_ptr(ptr);
}

rbit_t *rbit_new()
{
    return rbit_new_size(RBIT_INITIAL_SIZE);
}

void rbit_free(uint16_t *set)
{
    free(rbit_to_malloc_ptr(set));
}

static uint16_t rbit_size(const rbit_t *set)
{
    return set[-1];
}

uint16_t rbit_cardinality(const rbit_t *set)
{
    return set[0];
}

static uint16_t rbit_length_for(uint16_t cardinality)
{
    return sizeof(uint16_t) * cardinality;
}

uint16_t rbit_length(const rbit_t *rbit)
{
    return sizeof(uint16_t) + rbit_length_for(*rbit);
}

bool rbit_add(rbit_t *rbit, uint16_t item)
{
    uint16_t cardinality = *rbit;
    assert(!cardinality || rbit[cardinality] < item);
    rbit[cardinality + 1] = item;
    *rbit = cardinality + 1;
    return true;
}

bool rbit_equals(const rbit_t *rbit, const rbit_t *comparison)
{
    if (*rbit != *comparison)
        return false;
    uint16_t length = rbit_length_for(*rbit);
    return !memcmp(rbit + 1, comparison + 1, length);
}

rbit_t *rbit_new_items(uint16_t count, ...)
{
    rbit_t *rbit = rbit_new_size(count);
    if (!rbit)
        return NULL;
    va_list items;
    va_start(items, count);
    for (size_t i = 0; i < count; i++)
        if (!rbit_add(rbit, (uint16_t)va_arg(items, unsigned)))
            goto error;
    va_end(items);
    return rbit;
error:
    rbit_free(rbit);
    return NULL;
}
