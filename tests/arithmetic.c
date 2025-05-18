#include "ecalls.h"
#include <stdint.h>

#define MIN (-2147483648)
#define MAX (2147483647)

int _start() {
    int min = MIN;
    int max = MAX;

    TEST_ASSERT(min - 1, max);
    TEST_ASSERT(max + 1, min);

    TEST_ASSERT(min ^ min, 0);
    TEST_ASSERT(min & 0, 0);

    for (int i = -10; i <= 10; i++) {
        TEST_ASSERT(0 * i, 0);
        TEST_ASSERT(1 * i, i);
        TEST_ASSERT(i << 1, 2 * i);
        TEST_ASSERT(i << 2, 4 * i);
    }

    return 0;
}
