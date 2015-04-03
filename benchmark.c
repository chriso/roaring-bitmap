#include <stdio.h>
#include <assert.h>

#include "rset.h"

#ifdef __MACH__
# include <mach/mach_time.h>
#else
# error Unsupported system
#endif

const int times = 2048;

uint64_t nanoseconds()
{
#ifdef __MACH__
    return mach_absolute_time();
#endif
}

#define BENCH_START \
    { \
        uint64_t start, elapsed, best_time = -1; \
        for (int i = 0; i < times; i++) { \
            start = nanoseconds();

#define BENCH_END(info) \
            elapsed = nanoseconds() - start; \
            if (elapsed < best_time) \
                best_time = elapsed; \
        } \
        printf(info ": %llu ns\n", best_time); \
    }

int main()
{
    rset_t *set = rset_import(NULL, 4096);
    rset_t *result = rset_import(NULL, 4096);
    assert(set && result);

    rset_truncate(set);
    for (unsigned i = 0; i < 32768; i++)
        assert(rset_add(set, i * 2));
    assert(rset_cardinality(set) == 32768);
    BENCH_START
    assert(rset_contains(set, 10000));
    BENCH_END("Contains bitset")

    rset_truncate(set);
    for (unsigned i = 0; i < 4095; i++)
        assert(rset_add(set, i));
    assert(rset_cardinality(set) == 4095);
    BENCH_START
    assert(rset_contains(set, 4000));
    BENCH_END("Contains array")

    rset_truncate(set);
    for (unsigned i = 0; i < 32768; i++)
        assert(rset_add(set, i * 2));
    assert(rset_cardinality(set) == 32768);
    BENCH_START
    rset_invert(set, result);
    assert(rset_cardinality(result) == 32768);
    BENCH_END("Invert bitset")

    rset_truncate(set);
    for (unsigned i = 0; i < 4095; i++)
        assert(rset_add(set, i * 2));
    assert(rset_cardinality(set) == 4095);
    BENCH_START
    rset_invert(set, result);
    assert(rset_cardinality(result) == 61441);
    BENCH_END("Invert array")

    BENCH_START
    rset_truncate(set);
    for (unsigned i = 0; i < 65536; i++)
        assert(rset_add(set, i));
    assert(rset_cardinality(set) == 65536);
    BENCH_END("Fill ascending")

    BENCH_START
    rset_truncate(set);
    for (int i = 65535; i >= 0; i--)
        assert(rset_add(set, i));
    assert(rset_cardinality(set) == 65536);
    BENCH_END("Fill descending")

    BENCH_START
    rset_truncate(set);
    for (unsigned i = 0; i < 32768; i++)
        assert(rset_add(set, i));
    for (unsigned i = 65535; i >= 32768; i--)
        assert(rset_add(set, i));
    assert(rset_cardinality(set) == 65536);
    BENCH_END("Fill optimal")

    return 0;
}
