#include "ecalls.h"

int fib(int n) {
    if (n <= 1) {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

int fastfib(int n) {
    static int cache[256];

    if (n <= 1) {
        return 0;
    } else if (cache[n]) {
        return cache[n];
    }

    cache[n] = fib(n - 1) + fib(n - 2);
    return cache[n];
}

int _start() {
    PRINT_INT(fib(12));
    PRINT_INT(fib(16));
    PRINT_INT(fastfib(16));

    return 0;
}
