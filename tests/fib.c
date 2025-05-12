#include "ecalls.h"
#include <stdint.h>

int fib(int n) {
    if (n <= 1) {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

int fastfib(int n) {
    static int cache[256] = { 0 };

    if (n <= 1) {
        return n;
    }

    if (cache[n]) {
        return cache[n];
    }

    cache[n] = fastfib(n - 1) + fastfib(n - 2);
    return cache[n];
}

int fasterfib(int n) {
    static int cache[256];

    cache[0] = 0;
    cache[1] = 1;

    for (int i = 2; i <= n; i++) {
        cache[i] = cache[i - 1] + cache[i - 2];
    }

    return cache[n];
}

int _start() {
    int n = 36;

    uint32_t t[3];

    START_TRACE();
    int f1 = fib(n);
    TRACE_RESULT(t[0]);

    START_TRACE();
    int f2 = fastfib(n);
    TRACE_RESULT(t[1]);

    START_TRACE();
    int f3 = fasterfib(n);
    TRACE_RESULT(t[2]);

    PRINT_INT(t[0]);
    PRINT_INT(f1);

    PRINT_INT(t[1]);
    PRINT_INT(f2);

    PRINT_INT(t[2]);
    PRINT_INT(f3);

    return 0;
}
