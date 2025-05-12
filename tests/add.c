#include "ecalls.h"

int _start() {
    int a = 5;
    int b = 7;

    TEST_ASSERT(a + b, 13);
    
    return 0;
}
