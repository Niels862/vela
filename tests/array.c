#include "ecalls.h"

int _start() {
    int arr[64];

    for (int i = 0; i < 64; i++) {
        arr[i] = i;
    }

    int sum = 0;
    for (int i = 0; i < 64; i++) {
        sum += arr[i];
    }

    PRINT_INT(sum);

    return 0;
}
