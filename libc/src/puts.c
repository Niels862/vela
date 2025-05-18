#include "stdio.h"
#include "ecalls.h"

int puts(char const *s) {
    while (*s) {
        PRINT_CHAR(*s);
        s++;
    }
    PRINT_CHAR('\n');

    return 0;
}
