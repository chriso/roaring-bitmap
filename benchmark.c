#include <stdio.h>
#include <assert.h>

#include "rbit.h"

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
    rbit_t *set = rbit_import(NULL, 4096);
    assert(set);

    BENCH_START
    rbit_truncate(set);
    for (unsigned i = 0; i < 65536; i++)
        assert(rbit_add(set, i));
    assert(rbit_cardinality(set) == 65536);
    BENCH_END("Add ascending")

    BENCH_START
    rbit_truncate(set);
    for (int i = 65535; i >= 0; i--)
        assert(rbit_add(set, i));
    assert(rbit_cardinality(set) == 65536);
    BENCH_END("Add descending")

    BENCH_START
    rbit_truncate(set);
    for (unsigned i = 0; i < 32768; i++)
        assert(rbit_add(set, i));
    for (unsigned i = 65535; i >= 32768; i--)
        assert(rbit_add(set, i));
    assert(rbit_cardinality(set) == 65536);
    BENCH_END("Add optimal")

    BENCH_START
    rbit_truncate(set);
    for (unsigned i = 0; i < 65536; i++) {
        assert(rbit_add(set, i));
        if (i % 100 == 0)
            assert(rbit_contains(set, i));
    }
    assert(rbit_cardinality(set) == 65536);
    BENCH_END("Add ascending with contains check")

    return 0;
}
