#ifndef VEMU_CPU_H
#define VEMU_CPU_H

#include "registers.h"
#include <stdbool.h>
#include <stdint.h>

#define VEMU_MAX_OPCODES    128
#define VEMU_MAX_FUNCT3     8

#define VEMU_IS_COMPRESSED(instr) (((instr) & 0x3) != 0x3)

typedef enum {
    VEMU_OPCODE_ILLEGAL,
    VEMU_OPCODE_NOP,

    VEMU_OPCODE_LUI,
    VEMU_OPCODE_AUIPC,
    VEMU_OPCODE_JAL,
    VEMU_OPCODE_JALR,
    VEMU_OPCODE_BEQ,
    VEMU_OPCODE_BNE,
    VEMU_OPCODE_BLT,
    VEMU_OPCODE_BGE,
    VEMU_OPCODE_BLTU,
    VEMU_OPCODE_BGEU,
    VEMU_OPCODE_LB,
    VEMU_OPCODE_LH,
    VEMU_OPCODE_LW,
    VEMU_OPCODE_LBU,
    VEMU_OPCODE_LHU,
    VEMU_OPCODE_SB,
    VEMU_OPCODE_SH,
    VEMU_OPCODE_SW,
    VEMU_OPCODE_ADDI,
    VEMU_OPCODE_SLTI,
    VEMU_OPCODE_SLTIU,
    VEMU_OPCODE_XORI,
    VEMU_OPCODE_ORI,
    VEMU_OPCODE_ANDI,
    VEMU_OPCODE_SRLI,
    VEMU_OPCODE_SLLI,
    VEMU_OPCODE_SRAI,
    VEMU_OPCODE_ADD,
    VEMU_OPCODE_SUB,
    VEMU_OPCODE_SLL,
    VEMU_OPCODE_SLT,
    VEMU_OPCODE_SLTU,
    VEMU_OPCODE_XOR,
    VEMU_OPCODE_SRL,
    VEMU_OPCODE_SRA,
    VEMU_OPCODE_OR,
    VEMU_OPCODE_AND,
    VEMU_OPCODE_FENCE,
    VEMU_OPCODE_FENCE_TSO,
    VEMU_OPCODE_PAUSE,
    VEMU_OPCODE_ECALL,
    VEMU_OPCODE_EBREAK,
} vemu_opcode_t;

typedef enum {
    VEMU_FUNCT_BEQ          = 0x0,
    VEMU_FUNCT_BNE          = 0x1,
    VEMU_FUNCT_BLT          = 0x4,
    VEMU_FUNCT_BGE          = 0x5,
    VEMU_FUNCT_BLTU         = 0x6,
    VEMU_FUNCT_BGEU         = 0x7,

    VEMU_FUNCT_LB           = 0x0,
    VEMU_FUNCT_LH           = 0x1,
    VEMU_FUNCT_LW           = 0x2,
    VEMU_FUNCT_LBU          = 0x4,
    VEMU_FUNCT_LHU          = 0x5,

    VEMU_FUNCT_SB           = 0x0,
    VEMU_FUNCT_SH           = 0x1,
    VEMU_FUNCT_SW           = 0x2,

    VEMU_FUNCT_ADDI         = 0x0,
    VEMU_FUNCT_SLTI         = 0x2,
    VEMU_FUNCT_SLTIU        = 0x3,
    VEMU_FUNCT_XORI         = 0x4,
    VEMU_FUNCT_ORI          = 0x6,
    VEMU_FUNCT_ANDI         = 0x7,
    VEMU_FUNCT_SLLI         = 0x1,
    VEMU_FUNCT_SRLI_SRAI    = 0x5,

    VEMU_FUNCT_ADD_SUB      = 0x0,
    VEMU_FUNCT_SLL          = 0x1,
    VEMU_FUNCT_SLT          = 0x2,
    VEMU_FUNCT_SLTU         = 0x3,
    VEMU_FUNCT_XOR          = 0x4,
    VEMU_FUNCT_SRL_SRA      = 0x5,
    VEMU_FUNCT_OR           = 0x6,
    VEMU_FUNCT_AND          = 0x7,
} vemu_funct_t;

typedef enum {
    /* Quadrant 0 */
    VEMU_OPCODE_C_ADDI4SPN,

    /* Quadrant 1 */
    VEMU_OPCODE_C_ADDI,
    VEMU_OPCODE_C_LI,

    /* Quadrant 2 */
    VEMU_OPCODE_C_LWSP      = 0x2,
    VEMU_OPCODE_C_SWSP      = 0x6,
} vemu_compressed_opcode_t;

typedef enum {
    VEMU_OPCODE_R_LUI       = 0x37,
    VEMU_OPCODE_R_AUIPC     = 0x17,
    VEMU_OPCODE_R_JALR      = 0x67,
    VEMU_OPCODE_R_JAL       = 0x6F,
    VEMU_OPCODE_R_B         = 0x63,
    VEMU_OPCODE_R_L         = 0x03,
    VEMU_OPCODE_R_S         = 0x23,
    VEMU_OPCODE_R_I         = 0x13,
    VEMU_OPCODE_R_R         = 0x33,
    VEMU_OPCODE_R_FENCE     = 0x0F,
    VEMU_OPCODE_R_ECALL     = 0x73, /* TODO: ECALL / EBREAK */
} vemu_regular_opcode_t;

typedef enum {
    VEMU_FORMAT_R,
    VEMU_FORMAT_I,
    VEMU_FORMAT_S,
    VEMU_FORMAT_B,
    VEMU_FORMAT_U,
    VEMU_FORMAT_J,
} vemu_instruction_format_t;

typedef struct {
    uint32_t regs[VEMU_N_REGS];
    uint32_t ip;
    uint32_t next_ip;

    bool terminated;

    uint32_t trace;

    uint8_t **ram;
} vemu_cpu_t;

typedef struct {
    vemu_opcode_t opcode;
    uint8_t rd;
    uint8_t rs1;
    uint8_t rs2;
    uint32_t imm;
    bool c;
} vemu_decoded_t;

void vemu_disassemble(vemu_decoded_t *dec, uint32_t instr, uint32_t ip);

void vemu_cpu_init(vemu_cpu_t *cpu, uint8_t **ram);

void vemu_cpu_run(vemu_cpu_t *cpu, uint32_t entry);

#endif
