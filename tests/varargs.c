#include <stdarg.h>
#include <stdio.h>
#include <ecalls.h>

int sum_ints(int count, ...) {
    va_list args;
    va_start(args, count);

    int total = 0;
    for (int i = 0; i < count; i++) {
        total += va_arg(args, int);
    }
    
    va_end(args);
    
    return total;
}

void print_strings(int count, ...) {
    va_list args;
    va_start(args, count);

    for (int i = 0; i < count; i++) {
        char const *str = va_arg(args, char const *);
        puts(str);
    }

    va_end(args);
}

int _start() {
    int result = sum_ints(4, 10, 20, 30, 40);
    TEST_ASSERT(result, 100);

    result = sum_ints(3, -5, 0, 5);
    TEST_ASSERT(result, 0);

    print_strings(3, "hello", "world", ":)");

    return 0;
}
