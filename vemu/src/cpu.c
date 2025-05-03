#include "cpu.h"
#include "ram.h"
#include "util.h"
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>

static char const * const vemu_reg_names[VEMU_N_REGS] = {
    "zero",
    "ra",
    "sp",
    "gp",
    "tp",
    "t0",
    "t1",
    "t2",
    "fp",
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

static vemu_opcode_t const vemu_compressed_to_flat[VEMU_MAX_OPCODES] = {
    [VEMU_OPCODE_C_ADDI4SPN]    = VEMU_OPCODE_ADDI,
    [VEMU_OPCODE_C_ADDI]        = VEMU_OPCODE_ADDI,
    [VEMU_OPCODE_C_LI]          = VEMU_OPCODE_LI,
    [VEMU_OPCODE_C_ADD]         = VEMU_OPCODE_ADD,
    [VEMU_OPCODE_C_MV]          = VEMU_OPCODE_ADD,
    [VEMU_OPCODE_C_SWSP]        = VEMU_OPCODE_SW,
};

static vemu_opcode_t const vemu_lfunct_to_flat[VEMU_MAX_FUNCT3] = {
    [VEMU_FUNCT_LB]             = VEMU_OPCODE_LB,
    [VEMU_FUNCT_LH]             = VEMU_OPCODE_LH,
    [VEMU_FUNCT_LW]             = VEMU_OPCODE_LW,
    [VEMU_FUNCT_LBU]            = VEMU_OPCODE_LBU,
    [VEMU_FUNCT_LHU]            = VEMU_OPCODE_LHU,
};

static vemu_opcode_t const vemu_sfunct_to_flat[VEMU_MAX_FUNCT3] = {
    [VEMU_FUNCT_SB]             = VEMU_OPCODE_SB,
    [VEMU_FUNCT_SH]             = VEMU_OPCODE_SH,
    [VEMU_FUNCT_SW]             = VEMU_OPCODE_SW
};

static char const *vemu_opcode_names[VEMU_MAX_OPCODES] = {
    [VEMU_OPCODE_ILLEGAL]       = "illegal",
    [VEMU_OPCODE_ADD]           = "add",
    [VEMU_OPCODE_ADDI]          = "addi",
    [VEMU_OPCODE_LI]            = "li",
    [VEMU_OPCODE_LB]            = "lb",
    [VEMU_OPCODE_LH]            = "lh",
    [VEMU_OPCODE_LW]            = "lw",
    [VEMU_OPCODE_LBU]           = "lbu",
    [VEMU_OPCODE_LHU]           = "lhu",
    [VEMU_OPCODE_SB]            = "sb",
    [VEMU_OPCODE_SH]            = "sh",
    [VEMU_OPCODE_SW]            = "sw"
};

static inline uint32_t vemu_sext(uint32_t value, uint32_t bits) {
    return (int32_t)(value << (32 - bits)) >> (32 - bits);
}

static void vemu_decode_format_cr(uint32_t instr, vemu_decoded_t *dec, 
                                  vemu_compressed_opcode_t copcode) {
    dec->rd = (instr >> 7) & 0x1F;
    dec->rs1 = dec->rs2 = (instr >> 2) & 0x1F;
    dec->imm = 0;

    switch (copcode) {
        case VEMU_OPCODE_C_ADD:
            if (dec->rs2 == 0) {
                // ebreak / jalr
            } else {
                dec->opcode = VEMU_OPCODE_ADD;
            }
            break;

        case VEMU_OPCODE_C_MV:
            if (dec->rs2 == 0) {
                // jr
            } else {
                dec->opcode = VEMU_OPCODE_ADD;
                dec->rs1 = 0;
            }
            break;
        
        default:
            VEMU_UNREACHED();
    }
}

static void vemu_decode_format_ci(uint32_t instr, vemu_decoded_t *dec,
                                  vemu_compressed_opcode_t copcode) {
    dec->opcode = vemu_compressed_to_flat[copcode];
    VEMU_ASSERT(dec->opcode != 0);

    dec->rd = dec->rs1 = (instr >> 7) & 0x1F;
    dec->rs2 = 0;
    dec->imm = vemu_sext(((instr >> 2) & 0x1F) 
                         | (((instr >> 12) & 0x1) << 5), 6);
}

static void vemu_decode_format_css(uint32_t instr, vemu_decoded_t *dec,
                                   vemu_compressed_opcode_t copcode) {
    dec->opcode = vemu_compressed_to_flat[copcode];
    VEMU_ASSERT(dec->opcode != 0);

    dec->rd = 0;
    dec->rs1 = 2;
    dec->rs2 = (instr >> 2) & 0x1F;

    if (copcode == VEMU_OPCODE_C_SWSP) {
        dec->imm = (((instr >> 7) & 0x3) << 6) 
                 | (((instr >> 9) & 0xF) << 2);
    } else {
        VEMU_UNREACHED();
    }
}

static void vemu_decode_format_ciw(uint32_t instr, vemu_decoded_t *dec,
                                   vemu_compressed_opcode_t copcode) {
    dec->opcode = vemu_compressed_to_flat[copcode];
    VEMU_ASSERT(dec->opcode != 0);

    dec->rd = 8 + ((instr >> 2) & 0xF);
    dec->rs1 = dec->rs2 = 0;

    if (copcode == VEMU_OPCODE_C_ADDI4SPN) {
        dec->imm = (((instr >> 5) & 0x1) << 3)
                 | (((instr >> 6) & 0x1) << 2)
                 | (((instr >> 7) & 0xF) << 6)
                 | (((instr >> 11) & 0x3) << 4);
    } else {
        VEMU_UNREACHED();
    }
}

static void vemu_decode_compressed_q0(uint32_t instr, vemu_decoded_t *dec) {
    switch ((instr >> 13) & 0x7) {
        case 0x0:
            vemu_decode_format_ciw(instr, dec, VEMU_OPCODE_C_ADDI4SPN);
            break;

        default:
            break;
    }
}

static void vemu_decode_compressed_q1(uint32_t instr, vemu_decoded_t *dec) {    
    switch ((instr >> 13) & 0x7) {
        case 0x0:
            vemu_decode_format_ci(instr, dec, VEMU_OPCODE_C_ADDI);
            break;

        case 0x2:
            vemu_decode_format_ci(instr, dec, VEMU_OPCODE_C_LI);
            break;

        default:
            break;
    }
}

static void vemu_decode_compressed_q2(uint32_t instr, vemu_decoded_t *dec) {
    switch ((instr >> 13) & 0x7) {
        case 0x4:
            if (((instr >> 12) & 0x1) == 0x1) {
                vemu_decode_format_cr(instr, dec, VEMU_OPCODE_C_ADD);
            } else {
                vemu_decode_format_cr(instr, dec, VEMU_OPCODE_C_MV);
            }
            break;

        case 0x6:
            vemu_decode_format_css(instr, dec, VEMU_OPCODE_C_SWSP);
            break;

        default:
            break;
    }
}

static void vemu_decode_compressed(uint32_t instr, vemu_decoded_t *dec) {
    dec->opcode = dec->rd = dec->rs1 = dec->rs2 = dec->imm = 0;

    switch (instr & 0x3) {
        case 0x0:
            vemu_decode_compressed_q0(instr, dec);
            break;

        case 0x1:
            vemu_decode_compressed_q1(instr, dec);
            break;

        case 0x2:
            vemu_decode_compressed_q2(instr, dec);
            break;

        default:
            break;
    }
}

static void vemu_decode_format_i(uint32_t instr, vemu_decoded_t *dec, 
                                 vemu_regular_opcode_t ropcode) {
    uint32_t funct = (instr >> 12) & 0x7;
    switch (ropcode) {
        case VEMU_OPCODE_R_L:
            dec->opcode = vemu_lfunct_to_flat[funct];
            break;

        default:
            VEMU_UNREACHED();
    }
    VEMU_ASSERT(dec->opcode != 0);      

    dec->rd = ((instr) >> 7) & 0x1F;
    dec->rs1 = (instr >> 15) & 0x1F;
    dec->rs2 = 0;
    dec->imm = vemu_sext((instr >> 20) & 0xFFF, 12);
}

//fec42703
// 1111 1110 1100 0100  0010 0111 0000 0011
// 

static void vemu_decode_format_s(uint32_t instr, vemu_decoded_t *dec,
                                 vemu_regular_opcode_t ropcode) {
    VEMU_ASSERT(ropcode == VEMU_OPCODE_R_S);
    
    uint32_t funct = (instr >> 12) & 0x7;
    dec->opcode = vemu_sfunct_to_flat[funct];
    VEMU_ASSERT(dec->opcode != 0);      

    dec->rd = 0;
    dec->rs1 = (instr >> 15) & 0x1F;
    dec->rs2 = (instr >> 20) & 0x1F;
    dec->imm = vemu_sext(((instr >> 7) & 0x1F)
                         | (((instr >> 25)) << 5), 12);
}

static void vemu_decode_regular(uint32_t instr, vemu_decoded_t *dec) {
    dec->opcode = dec->rd = dec->rs1 = dec->rs2 = dec->imm = 0;

    switch (instr & 0x7F) {
        case VEMU_OPCODE_R_L:
            vemu_decode_format_i(instr, dec, VEMU_OPCODE_R_L);
            break;

        case VEMU_OPCODE_R_S:
            vemu_decode_format_s(instr, dec, VEMU_OPCODE_R_S);
            break;

        default:
            VEMU_UNREACHED();
    }
}

static vemu_instruction_format_t vemu_opcode_to_format(vemu_opcode_t opcode) {
    switch (opcode) {
        case VEMU_OPCODE_ADD:
            return VEMU_INSTRFORMAT_R;

        case VEMU_OPCODE_ADDI:
        case VEMU_OPCODE_LI:
        case VEMU_OPCODE_LB:
        case VEMU_OPCODE_LH:
        case VEMU_OPCODE_LW:
        case VEMU_OPCODE_LBU:
        case VEMU_OPCODE_LHU:
            return VEMU_INSTRFORMAT_I;

        case VEMU_OPCODE_SB:
        case VEMU_OPCODE_SH:
        case VEMU_OPCODE_SW:
            return VEMU_INSTRFORMAT_S;

        case VEMU_OPCODE_ILLEGAL:
            break;
    }

    VEMU_UNREACHED();
}

void vemu_disassemble(vemu_decoded_t *dec) {
    vemu_instruction_format_t format = vemu_opcode_to_format(dec->opcode);

    char const *opcode = vemu_opcode_names[dec->opcode];
    char const *rd = vemu_reg_names[dec->rd];
    char const *rs1 = vemu_reg_names[dec->rs1];
    char const *rs2 = vemu_reg_names[dec->rs2];

    switch (format) {
        case VEMU_INSTRFORMAT_R:
            fprintf(stderr, "%s %s,%s,%s\n", opcode, rd, rs1, rs2);
            break;

        case VEMU_INSTRFORMAT_I:
            fprintf(stderr, "%s %s,%s,%d\n", opcode, rd, rs1, dec->imm);
            break;

        case VEMU_INSTRFORMAT_S:
            fprintf(stderr, "%s %s,%d(%s)\n", opcode, rs2, dec->imm, rs1);
            break;

        case VEMU_INSTRFORMAT_B:
        case VEMU_INSTRFORMAT_U:
        case VEMU_INSTRFORMAT_J:
            fprintf(stderr, "todo\n");
            break;
    }
}

void vemu_cpu_init(vemu_cpu_t *cpu, uint8_t **ram) {
    for (size_t i = 0; i < VEMU_N_REGS; i++) {
        cpu->regs[i] = 0;
    }
    cpu->ip = 0;

    cpu->ram = ram;
}

void vemu_cpu_run(vemu_cpu_t *cpu, uint32_t entry) {
    cpu->ip = entry;

    bool terminated = false;
    while (!terminated) {
        uint32_t instr = vemu_ram_load_half(*cpu->ram, cpu->ip);

        fprintf(stderr, "%8x: ", cpu->ip);

        vemu_decoded_t dec = { 0 };
        if (VEMU_IS_COMPRESSED(instr)) {
            vemu_decode_compressed(instr, &dec);

            cpu->ip += 2;
    
            fprintf(stderr, "    %04x    ", instr);
        } else {
            instr = instr | (vemu_ram_load_half(*cpu->ram, cpu->ip + 2) << 16);
            vemu_decode_regular(instr, &dec);

            cpu->ip += 4;
    
            fprintf(stderr, "%08x    ", instr);
        }
    
        vemu_disassemble(&dec);
        
        if (dec.opcode == VEMU_OPCODE_ILLEGAL) {
            terminated = true;
        }
    }
}
