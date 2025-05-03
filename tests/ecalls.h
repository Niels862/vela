/* Will be moved later */

enum ecall {
    ECALL_PRINT_INT
};

void iprint(int i) {
    __asm__ volatile (
        "mv a0, %0\n"
        "mv a7, %1\n"
        "ecall\n"
        :
        : "r"(i), "r"(ECALL_PRINT_INT)
        : "a0", "a7"
    );
}
