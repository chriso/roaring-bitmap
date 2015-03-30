#include <assert.h>

#include "rbit.h"

static void test_new()
{
    rbit_t *set = rbit_new();
    assert(set);
    assert(rbit_cardinality(set) == 0);
    assert(rbit_length(set) == sizeof(uint16_t) * 2);
    rbit_free(set);
}

static void test_new_items()
{
    rbit_t *set = rbit_new_items(0);
    assert(set);
    assert(rbit_cardinality(set) == 0);
    assert(rbit_length(set) == sizeof(uint16_t) * 2);
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

static void test_import_export()
{
    rbit_t *set = rbit_new_items(3, 1, 2, 3);
    assert(set);
    assert(rbit_export(set) == set->buffer);
    assert(rbit_length(set) == 4 * sizeof(uint16_t));

    rbit_t *copy = rbit_import(rbit_export(set), rbit_length(set));
    assert(copy);

    assert(rbit_equals(set, copy));

    rbit_free(set);
    rbit_free(copy);
}

static void test_copy()
{
    rbit_t *set = rbit_new();
    assert(set);
    rbit_t *copy = rbit_copy(set);
    assert(copy);

    assert(rbit_equals(set, copy));
    assert(rbit_cardinality(set) == rbit_cardinality(copy));
    assert(rbit_length(set) == rbit_length(copy));

    rbit_free(set);
    rbit_free(copy);

    set = rbit_new_items(5, 1, 2, 3, 4, 5);
    assert(set);
    copy = rbit_copy(set);
    assert(copy);

    assert(rbit_equals(set, copy));
    assert(rbit_cardinality(set) == rbit_cardinality(copy));
    assert(rbit_length(set) == rbit_length(copy));

    rbit_free(set);
    rbit_free(copy);
}

static void test_buffer_resizing()
{
    rbit_t *set = rbit_new();
    assert(set);
    for (uint16_t i = 0; i < 1000; i++)
        assert(rbit_add(set, i));
    assert(rbit_cardinality(set) == 1000);
    for (uint16_t i = 0; i < 1000; i++)
        assert(!rbit_add(set, i));
    rbit_free(set);
}

static void test_array_to_bitset()
{
    rbit_t *set = rbit_new();
    assert(set);
    for (uint16_t i = 0; i < 32768; i++)
        assert(rbit_add(set, i * 2));
    assert(rbit_cardinality(set) == 32768);
    for (uint16_t i = 0; i < 4096; i++)
        assert(set->buffer[i + 1] == 0x5555); // 0101010101010101
    for (uint16_t i = 0; i < 32768; i++)
        assert(!rbit_add(set, i * 2));
    rbit_free(set);
}

static void test_bitset_to_inverted_array()
{
    rbit_t *set = rbit_new();
    assert(set);
    for (uint16_t i = 0; i <= 61440; i++)
        assert(rbit_add(set, i));
    assert(rbit_cardinality(set) == 61441);
    for (uint16_t i = 0; i < 4095; i++)
        assert(set->buffer[i + 1] == 61441 + i);
    for (uint16_t i = 0; i <= 61440; i++)
        assert(!rbit_add(set, i));
    rbit_free(set);
}

static void test_add_full_set()
{
    rbit_t *set = rbit_new();
    rbit_t *comparison = rbit_new();
    assert(set && comparison);
    for (unsigned i = 0; i < 65536; i++) {
        assert(rbit_add(set, i));
        assert(!rbit_add(set, i));
        assert(rbit_add(comparison, i));
        assert(rbit_equals(set, comparison));
    }
    assert(rbit_cardinality(set) == 65536);
    assert(rbit_length(set) == sizeof(uint16_t));
    assert(set->buffer[0] == 0);
    rbit_free(set);
    rbit_free(comparison);
}

int main()
{
    test_new();
    test_new_items();
    test_equals();
    test_import_export();
    test_copy();
    test_buffer_resizing();
    test_array_to_bitset();
    test_bitset_to_inverted_array();
    test_add_full_set();
    return 0;
}
