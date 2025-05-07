/* Will be moved later */

enum ecall {
    ECALL_PRINT_INT
};

#define ECALL0(ecall)                           \
    do {                                        \
        register int a7 asm("a7") = (ecall);    \
        asm volatile("ecall"                    \
                     :                          \
                     : "r"(a7)                  \
                     : "memory");               \
    } while (0)

#define ECALL1(ecall, arg0)                     \
    do {                                        \
        register int a0 asm("a0") = (arg0);     \
        register int a7 asm("a7") = (ecall);    \
        asm volatile("ecall"                    \
                     :                          \
                     : "r"(a0), "r"(a7)         \
                     : "memory");               \
    } while (0)

#define SYSCALL2(NUM, ARG0, ARG1)               \
    do {                                        \
        register int a0 asm("a0") = (ARG0);     \
        register int a1 asm("a1") = (ARG1);     \
        register int a7 asm("a7") = (NUM);      \
        asm volatile("ecall"                    \
                     :                          \
                     : "r"(a0), "r"(a1), "r"(a7) \
                     : "memory");               \
    } while (0)

#define PRINT_INT(i) ECALL1(ECALL_PRINT_INT, (i))
