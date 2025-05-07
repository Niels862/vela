#ifndef VEMU_REGISTERS_H
#define VEMU_REGISTERS_H

typedef enum {
    VEMU_ZERO,
    VEMU_RA,
    VEMU_SP,
    VEMU_GP,
    VEMU_TP,
    VEMU_T0,
    VEMU_T1,
    VEMU_T2,
    VEMU_S0,
    VEMU_S1,
    VEMU_A0,
    VEMU_A1,
    VEMU_A2,
    VEMU_A3,
    VEMU_A4,
    VEMU_A5,
    VEMU_A6,
    VEMU_A7,
    VEMU_S2,
    VEMU_S3,
    VEMU_S4,
    VEMU_S5,
    VEMU_S6,
    VEMU_S7,
    VEMU_S8,
    VEMU_S9,
    VEMU_S10,
    VEMU_S11,
    VEMU_T3,
    VEMU_T4,
    VEMU_T5,
    VEMU_T6,
} vemu_register_t;

#define VEMU_N_REGS 32

char const *vemu_register_name(vemu_register_t reg);

#endif
