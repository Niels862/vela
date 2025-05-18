#include "ecall-codes.h"

#define ECALL0(ecall)                               \
    do {                                            \
        register int a7 asm("a7") = (ecall);        \
        asm volatile("ecall"                        \
                     :                              \
                     : "r"(a7)                      \
                     : "memory");                   \
    } while (0)

#define ECALL1(ecall, arg0)                         \
    do {                                            \
        register int a0 asm("a0") = (arg0);         \
        register int a7 asm("a7") = (ecall);        \
        asm volatile("ecall"                        \
                     :                              \
                     : "r"(a0), "r"(a7)             \
                     : "memory");                   \
    } while (0)

#define ECALL2(NUM, ARG0, ARG1)                     \
    do {                                            \
        register int a0 asm("a0") = (ARG0);         \
        register int a1 asm("a1") = (ARG1);         \
        register int a7 asm("a7") = (NUM);          \
        asm volatile("ecall"                        \
                     :                              \
                     : "r"(a0), "r"(a1), "r"(a7)    \
                     : "memory");                   \
    } while (0)

#define ECALL3(NUM, ARG0, ARG1, ARG2)               \
do {                                                \
    register int a0 asm("a0") = (ARG0);             \
    register int a1 asm("a1") = (ARG1);             \
    register int a2 asm("a2") = (ARG2);             \
    register int a7 asm("a7") = (NUM);              \
    asm volatile("ecall"                            \
                    :                               \
                    : "r"(a0), "r"(a1), "r"(a2),    \
                      "r"(a7)                       \
                    : "memory");                    \
} while (0)

#define ECALL0_RET(ecall, result)                   \
    do {                                            \
        register int a7 asm("a7") = (ecall);        \
        register int a0 asm("a0");                  \
        asm volatile("ecall"                        \
                        : "=r"(a0)                  \
                        : "r"(a7)                   \
                        : "memory");                \
        (result) = a0;                              \
    } while (0)

#define PRINT_INT(i)            ECALL1(VEMU_ECALL_PRINT_INT, (i))
#define PRINT_CHAR(c)           ECALL1(VEMU_ECALL_PRINT_CHAR, (c))
#define START_TRACE()           ECALL0(VEMU_ECALL_START_TRACE)
#define TRACE_RESULT(res)       ECALL0_RET(VEMU_ECALL_TRACE_RESULT, res)
#define TEST_ASSERT(x, y)       ECALL3(VEMU_ECALL_TEST_ASSERT, __LINE__, x, y)
