#include <assert.h>

#include "rbit.h"

static void test_new()
{
    rbit_t *set = rbit_new();
    assert(set);
    assert(rbit_cardinality(set) == 0);
    assert(rbit_length(set) == sizeof(uint16_t));
    rbit_free(set);
}

static void test_new_items()
{
    rbit_t *set = rbit_new_items(0);
    assert(set);
    assert(rbit_cardinality(set) == 0);
    assert(rbit_length(set) == sizeof(uint16_t));
    rbit_free(set);

    set = rbit_new_items(3, 1000, 2000, 3000);
    assert(set);
    assert(rbit_cardinality(set) == 3);
    assert(rbit_length(set) == sizeof(uint16_t) * (1 + 3));
    assert(set->buffer[0] == 3 &&
           set->buffer[1] == 1000 &&
           set->buffer[2] == 2000 &&
           set->buffer[3] == 3000);
    rbit_free(set);
}

static void test_equals()
{
    rbit_t *set = rbit_new_items(3, 1000, 2000, 3000);
    rbit_t *comparison = rbit_new();
    assert(set && comparison);

    assert(!rbit_equals(set, comparison));
    assert(rbit_add(comparison, 1000));
    assert(!rbit_equals(set, comparison));
    assert(rbit_add(comparison, 2000));
    assert(!rbit_equals(set, comparison));
    assert(rbit_add(comparison, 3000));
    assert(rbit_equals(set, comparison));
    rbit_free(comparison);

    comparison = rbit_new_items(3, 1000, 2000, 3001);
    assert(comparison);
    assert(!rbit_equals(set, comparison));
    rbit_free(comparison);

    comparison = rbit_new_items(3, 1000, 2000, 3000);
    assert(comparison);
    assert(rbit_equals(set, comparison));
    rbit_free(comparison);

    rbit_free(set);
}

static void test_buffer_resizing()
{
    rbit_t *set = rbit_new();
    assert(set);
    assert(rbit_cardinality(set) == 0);
    assert(rbit_length(set) == sizeof(uint16_t));
    for (uint16_t i = 0; i < 1000; i++)
        assert(rbit_add(set, i));
    assert(rbit_cardinality(set) == 1000);
    rbit_free(set);
}

int main()
{
    test_new();
    test_new_items();
    test_equals();
    test_buffer_resizing();
    return 0;
}
