#include <stdarg.h>
#include <assert.h>

#include "rset.h"

rset_t *rset_new_items(unsigned count, ...)
{
    rset_t *set = rset_import(NULL, count);
    if (!set)
        return NULL;
    va_list items;
    va_start(items, count);
    for (unsigned i = 0; i < count; i++)
        if (!rset_add(set, (uint16_t)va_arg(items, unsigned)))
            goto error;
    va_end(items);
    return set;
error:
    rset_free(set);
    return NULL;
}

static void test_new()
{
    rset_t *set = rset_new();
    assert(set);
    assert(rset_cardinality(set) == 0);
    assert(rset_length(set) == sizeof(uint16_t) * 2);
    rset_free(set);
}

static void test_new_items()
{
    rset_t *set = rset_new_items(0);
    assert(set);
    assert(rset_cardinality(set) == 0);
    assert(rset_length(set) == sizeof(uint16_t) * 2);
    rset_free(set);

    set = rset_new_items(3, 1000, 2000, 3000);
    assert(set);
    assert(rset_cardinality(set) == 3);
    assert(rset_length(set) == sizeof(uint16_t) * (1 + 3));
    assert(set->buffer[0] == 3 &&
           set->buffer[1] == 1000 &&
           set->buffer[2] == 2000 &&
           set->buffer[3] == 3000);
    rset_free(set);
}

static void test_equals()
{
    rset_t *set = rset_new_items(3, 1000, 2000, 3000);
    rset_t *comparison = rset_new();
    assert(set && comparison);

    assert(!rset_equals(set, comparison));
    assert(rset_add(comparison, 1000));
    assert(!rset_equals(set, comparison));
    assert(rset_add(comparison, 2000));
    assert(!rset_equals(set, comparison));
    assert(rset_add(comparison, 3000));
    assert(rset_equals(set, comparison));
    rset_free(comparison);

    comparison = rset_new_items(3, 1000, 2000, 3001);
    assert(comparison);
    assert(!rset_equals(set, comparison));
    rset_free(comparison);

    comparison = rset_new_items(3, 1000, 2000, 3000);
    assert(comparison);
    assert(rset_equals(set, comparison));
    rset_free(comparison);

    rset_free(set);
}

static void test_import_export()
{
    rset_t *set = rset_new_items(3, 1, 2, 3);
    assert(set);
    assert(rset_export(set) == set->buffer);
    assert(rset_length(set) == 4 * sizeof(uint16_t));

    rset_t *copy = rset_import(rset_export(set), rset_length(set));
    assert(copy);

    assert(rset_equals(set, copy));

    rset_free(set);
    rset_free(copy);
}

static void test_copy()
{
    rset_t *set = rset_new();
    assert(set);
    rset_t *copy = rset_copy(set);
    assert(copy);

    assert(rset_equals(set, copy));
    assert(rset_cardinality(set) == rset_cardinality(copy));
    assert(rset_length(set) == rset_length(copy));

    rset_free(set);
    rset_free(copy);

    set = rset_new_items(5, 1, 2, 3, 4, 5);
    assert(set);
    copy = rset_copy(set);
    assert(copy);

    assert(rset_equals(set, copy));
    assert(rset_cardinality(set) == rset_cardinality(copy));
    assert(rset_length(set) == rset_length(copy));

    rset_free(set);
    rset_free(copy);
}

static void test_truncate()
{
    rset_t *set = rset_new_items(5, 1, 2, 3, 4, 5);
    assert(set);

    assert(rset_cardinality(set) == 5);

    rset_truncate(set);

    assert(rset_cardinality(set) == 0);

    rset_free(set);
}

static void test_buffer_resizing()
{
    rset_t *set = rset_new();
    assert(set);
    for (uint16_t i = 0; i < 1000; i++)
        assert(rset_add(set, i));
    assert(rset_cardinality(set) == 1000);
    rset_free(set);
}

static void test_array_to_bitset()
{
    rset_t *set = rset_new();
    assert(set);
    for (uint16_t i = 0; i < 32768; i++)
        assert(rset_add(set, i * 2));
    assert(rset_cardinality(set) == 32768);
    for (uint16_t i = 0; i < 4096; i++)
        assert(set->buffer[i + 1] == 0x5555); // 0101010101010101
    rset_free(set);
}

static void test_bitset_to_inverted_array()
{
    rset_t *set = rset_new();
    assert(set);
    for (uint16_t i = 0; i <= 61440; i++)
        assert(rset_add(set, i));
    assert(rset_cardinality(set) == 61441);
    for (uint16_t i = 0; i < 4095; i++)
        assert(set->buffer[i + 1] == 61441 + i);
    rset_free(set);
}

static void test_add_ascending()
{
    rset_t *set = rset_new();
    rset_t *comparison = rset_new();
    assert(set && comparison);
    for (unsigned i = 0; i < 65536; i++) {
        assert(rset_add(set, i));
        assert(rset_add(set, i)); // idempotent
        assert(rset_add(comparison, i));
        assert(rset_equals(set, comparison));
    }
    assert(rset_cardinality(set) == 65536);
    assert(rset_length(set) == sizeof(uint16_t));
    assert(set->buffer[0] == 0);
    rset_free(set);
    rset_free(comparison);
}

static void test_add_descending()
{
    rset_t *set = rset_new();
    rset_t *comparison = rset_new();
    assert(set && comparison);
    for (int i = 65535; i >= 0; i--) {
        assert(rset_add(set, i));
        assert(rset_add(set, i)); // idempotent
        assert(rset_add(comparison, i));
        assert(rset_equals(set, comparison));
    }
    assert(rset_cardinality(set) == 65536);
    assert(rset_length(set) == sizeof(uint16_t));
    assert(set->buffer[0] == 0);
    rset_free(set);
    rset_free(comparison);
}

static void test_add_optimal()
{
    rset_t *set = rset_new();
    rset_t *comparison = rset_new();
    assert(set && comparison);
    for (unsigned i = 0; i < 32768; i++) {
        assert(rset_add(set, i));
        assert(rset_add(set, i)); // idempotent
        assert(rset_add(comparison, i));
        assert(rset_equals(set, comparison));
    }
    for (unsigned i = 65535; i >= 32768; i--) {
        assert(rset_add(set, i));
        assert(rset_add(set, i)); // idempotent
        assert(rset_add(comparison, i));
        assert(rset_equals(set, comparison));
    }
    assert(rset_cardinality(set) == 65536);
    assert(rset_length(set) == sizeof(uint16_t));
    assert(set->buffer[0] == 0);
    rset_free(set);
    rset_free(comparison);
}

static void test_contains()
{
    rset_t *set = rset_new();
    assert(set);
    for (unsigned i = 0; i < 65536; i++) {
        assert(!rset_contains(set, i));
        assert(rset_add(set, i));
        assert(rset_contains(set, i));
    }
    rset_free(set);
}

int main()
{
    test_new();
    test_new_items();
    test_equals();
    test_import_export();
    test_copy();
    test_truncate();
    test_buffer_resizing();
    test_array_to_bitset();
    test_bitset_to_inverted_array();
    test_add_ascending();
    test_add_descending();
    test_add_optimal();
    test_contains();
    return 0;
}
