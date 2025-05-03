#ifndef VEMU_CPU_H
#define VEMU_CPU_H

#include <inttypes.h>

#define VEMU_N_REGS         32
#define VEMU_MAX_OPCODES    128
#define VEMU_MAX_FUNCT3     8

#define VEMU_IS_COMPRESSED(instr) (((instr) & 0x3) != 0x3)

typedef enum {
    VEMU_OPCODE_ILLEGAL,
    VEMU_OPCODE_ADD,
    VEMU_OPCODE_ADDI,
    VEMU_OPCODE_LI,
    VEMU_OPCODE_LB,
    VEMU_OPCODE_LH,
    VEMU_OPCODE_LW,
    VEMU_OPCODE_LBU,
    VEMU_OPCODE_LHU,
    VEMU_OPCODE_SB,
    VEMU_OPCODE_SH,
    VEMU_OPCODE_SW
} vemu_opcode_t;

typedef enum {
    VEMU_FUNCT_LB           = 0x0,
    VEMU_FUNCT_LH           = 0x1,
    VEMU_FUNCT_LW           = 0x2,
    VEMU_FUNCT_LBU          = 0x4,
    VEMU_FUNCT_LHU          = 0x5,

    VEMU_FUNCT_SB           = 0x0,
    VEMU_FUNCT_SH           = 0x1,
    VEMU_FUNCT_SW           = 0x2
} vemu_funct_t;

typedef enum {
    VEMU_OPCODE_C_ADDI4SPN,
    VEMU_OPCODE_C_ADDI,
    VEMU_OPCODE_C_LI,
    VEMU_OPCODE_C_SWSP,
    VEMU_OPCODE_C_ADD,
    VEMU_OPCODE_C_MV,
} vemu_compressed_opcode_t;

typedef enum {
    VEMU_OPCODE_R_ADDI      = 0x13,
    VEMU_OPCODE_R_LI        = 0x71,
    VEMU_OPCODE_R_L         = 0x03,
    VEMU_OPCODE_R_S         = 0x23,
} vemu_regular_opcode_t;

typedef enum {
    VEMU_INSTRFORMAT_R,
    VEMU_INSTRFORMAT_I,
    VEMU_INSTRFORMAT_S,
    VEMU_INSTRFORMAT_B,
    VEMU_INSTRFORMAT_U,
    VEMU_INSTRFORMAT_J,
} vemu_instruction_format_t;

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

void vemu_disassemble(vemu_decoded_t *dec);

void vemu_cpu_init(vemu_cpu_t *cpu, uint8_t **ram);

void vemu_cpu_run(vemu_cpu_t *cpu, uint32_t entry);

#endif
