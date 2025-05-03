#ifndef VEMU_CPU_H
#define VEMU_CPU_H

#include <inttypes.h>

#define VEMU_N_REGS         32
#define VEMU_MAX_OPCODES    128

#define VEMU_IS_COMPRESSED(instr) (((instr) & 0x3) != 0x3)

typedef enum {
    VEMU_OPCODE_ILLEGAL     = 0x00,
    VEMU_OPCODE_ADDI        = 0x13,
    VEMU_OPCODE_LI          = 0x71,
    VEMU_OPCODE_SW          = 0x70
} vemu_opcode_t;

typedef enum {
    VEMU_OPCODE_C_ADDI4SPN,

    VEMU_OPCODE_C_ADDI,
    VEMU_OPCODE_C_LI,

    VEMU_OPCODE_C_SWSP,
} vemu_compressed_opcode_t;

typedef struct {
    uint32_t regs[VEMU_N_REGS];
    uint32_t ip;

    uint8_t **ram;
} vemu_cpu_t;

typedef struct {
    uint8_t opcode;
    uint8_t rd;
    uint8_t rs1;
    uint8_t rs2;
    uint32_t imm;
} vemu_decoded_t;

void vemu_cpu_init(vemu_cpu_t *cpu, uint8_t **ram);

void vemu_cpu_run(vemu_cpu_t *cpu, uint32_t entry);

#endif
