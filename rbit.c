#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>

#include "rbit.h"

#define RBIT_INITIAL_SIZE 16

static void *rbit_to_malloc_ptr(rbit_t *rbit)
{
    return rbit - 1;
}

static void *rbit_from_malloc_ptr(void *ptr)
{
    return (uint16_t *)ptr + 1;
}

rbit_t *rbit_new()
{
    uint16_t *ptr = calloc(2 + RBIT_INITIAL_SIZE, sizeof(uint16_t));
    *ptr = RBIT_INITIAL_SIZE;
    return rbit_from_malloc_ptr(ptr);
}

void rbit_free(uint16_t *rbit)
{
    free(rbit_to_malloc_ptr(rbit));
}

static uint16_t rbit_size(const rbit_t *rbit)
{
    return rbit[-1];
}

uint16_t rbit_cardinality(const rbit_t *rbit)
{
    return rbit[0];
}

uint16_t rbit_length(const rbit_t *rbit)
{
    return sizeof(uint16_t) * rbit_cardinality(rbit);
}

bool rbit_add(rbit_t *rbit, uint16_t item)
{
    uint16_t cardinality = *rbit;
    assert(!cardinality || rbit[cardinality] < item);
    rbit[cardinality + 1] = item;
    *rbit = cardinality + 1;
    return true;
}

rbit_t *rbit_new_items(size_t count, ...)
{
    rbit_t *rbit = rbit_new();
    if (!rbit)
        return NULL;
    va_list items;
    va_start(items, count);
    for (size_t i = 0; i < count; i++)
        if (!rbit_add(rbit, va_arg(items, unsigned)))
            goto error;
    va_end(items);
    return rbit;
error:
    rbit_free(rbit);
    return NULL;
}
