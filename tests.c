#include <assert.h>

#include "rbit.h"

int main()
{
    rbit_t *rbit = rbit_new();
    assert(rbit);

    assert(rbit_cardinality(rbit) == 0);
    assert(rbit_add(rbit, 0));
    assert(rbit_cardinality(rbit) == 1);
    assert(rbit_add(rbit, 1));
    assert(rbit_cardinality(rbit) == 2);
    assert(rbit_add(rbit, 2));
    assert(rbit_cardinality(rbit) == 3);

    rbit_free(rbit);

    return 0;
}
