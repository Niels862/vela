#include "registers.h"
#include <assert.h>

static char const * const vemu_register_names[VEMU_N_REGS] = {
    "zero",
    "ra",
    "sp",
    "gp",
    "tp",
    "t0",
    "t1",
    "t2",
    "s0", // fp
    "s1",
    "a0",
    "a1",
    "a2",
    "a3",
    "a4",
    "a5",
    "a6",
    "a7",
    "s2",
    "s3",
    "s4",
    "s5",
    "s6",
    "s7",
    "s8",
    "s9",
    "s10",
    "s11",
    "t3",
    "t4",
    "t5",
    "t6",
};

char const *vemu_register_name(vemu_register_t reg) {
    assert(reg < VEMU_N_REGS);
    return vemu_register_names[reg];
}
