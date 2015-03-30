#include <stdio.h>
#include <assert.h>

#include "rbit.h"

#ifdef __MACH__
# include <mach/mach_time.h>
#else
# error Unsupported system
#endif

const int times = 2048;

int main()
{
    rbit_t *set = rbit_import(NULL, 4096);
    assert(set);

    uint64_t best_time = -1;

    for (int i = 0; i < times; i++) {

#ifdef __MACH__
        uint64_t start = mach_absolute_time();
#endif

        rbit_truncate(set);

        for (unsigned i = 0; i < 65536; i++)
            rbit_add(set, i);

#ifdef __MACH__
        uint64_t elapsed = mach_absolute_time() - start;
#endif

        if (elapsed < best_time)
            best_time = elapsed;
    }

    printf("%llu\n", best_time);

    return 0;
}
