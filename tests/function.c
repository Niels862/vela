#include "ecalls.h"

int f() {
    return 42;
}

int _start() {
    PRINT_INT(f());

    return 0;
}
