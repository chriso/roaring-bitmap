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

static void test_fill_ascending()
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

static void test_fill_descending()
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

static void test_fill_optimal()
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

static void test_invert()
{
    rset_t *set = rset_new();
    assert(set);
    for (unsigned i = 4; i < 65536; i++)
        assert(rset_add(set, i));
    rset_t *inverted = rset_new();
    assert(inverted);
    assert(rset_invert(set, inverted));
    rset_t *expected = rset_new_items(4, 0, 1, 2, 3);
    assert(expected);
    assert(rset_equals(inverted, expected));

    rset_t *inverted_twice = rset_new();
    assert(inverted_twice);
    assert(rset_invert(inverted, inverted_twice));
    assert(rset_equals(set, inverted_twice));

    rset_truncate(set);
    rset_truncate(expected);
    for (unsigned i = 0; i < 65536; i++)
        assert(rset_add(expected, i));
    assert(rset_invert(set, inverted));
    assert(rset_cardinality(inverted) == 65536);

    assert(rset_equals(inverted, expected));
    assert(rset_invert(inverted, inverted_twice));
    assert(rset_cardinality(inverted_twice) == 0);
    assert(rset_equals(set, inverted_twice));

    rset_truncate(set);
    for (unsigned i = 0; i < 30000; i++)
        assert(rset_add(set, i));
    rset_truncate(expected);
    for (unsigned i = 30000; i < 65536; i++)
        assert(rset_add(expected, i));
    assert(rset_invert(set, inverted));
    assert(rset_cardinality(inverted) == 35536);
    assert(rset_equals(inverted, expected));
    assert(rset_invert(inverted, inverted_twice));
    assert(rset_cardinality(inverted_twice) == 30000);
    assert(rset_equals(set, inverted_twice));

    rset_free(expected);
    rset_free(inverted_twice);
    rset_free(inverted);
    rset_free(set);
}

static void test_intersection()
{
    rset_t *a = rset_new();
    rset_t *b = rset_new_items(10, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9);
    rset_t *result = rset_new();
    assert(a && b && result);

    assert(rset_intersection(a, b, result));
    assert(rset_cardinality(result) == 0);
    assert(rset_intersection(b, a, result));
    assert(rset_cardinality(result) == 0);

    assert(rset_fill(a));

    assert(rset_intersection(a, b, result));
    assert(rset_equals(b, result));
    assert(rset_intersection(b, a, result));
    assert(rset_equals(b, result));

    rset_truncate(a);
    for (unsigned i = 0; i < 100; i += 2)
        assert(rset_add(a, i));
    assert(rset_intersection(a, b, result));
    rset_t *expected = rset_new_items(5, 0, 2, 4, 6, 8);
    assert(rset_equals(result, expected));

    rset_truncate(b);
    for (unsigned i = 1; i < 100; i += 2)
        assert(rset_add(b, i));
    assert(rset_intersection(a, b, result));
    assert(rset_cardinality(result) == 0);

    rset_free(a);
    rset_free(b);
    rset_free(result);
    rset_free(expected);
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
    test_fill_ascending();
    test_fill_descending();
    test_fill_optimal();
    test_contains();
    test_invert();
    test_intersection();
    return 0;
}
