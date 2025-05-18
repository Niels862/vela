#include "cpu.h"
#include "ram.h"
#include "ecall-codes.h"
#include "util.h"
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>

static vemu_opcode_t const vemu_bfunct_to_flat[VEMU_MAX_FUNCT3] = {
    [VEMU_FUNCT_BEQ]            = VEMU_OPCODE_BEQ,
    [VEMU_FUNCT_BNE]            = VEMU_OPCODE_BNE,
    [VEMU_FUNCT_BLT]            = VEMU_OPCODE_BLT,
    [VEMU_FUNCT_BGE]            = VEMU_OPCODE_BGE,
    [VEMU_FUNCT_BLTU]           = VEMU_OPCODE_BLTU,
    [VEMU_FUNCT_BGEU]           = VEMU_OPCODE_BGEU,
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

static vemu_opcode_t const vemu_ifunct_to_flat[VEMU_MAX_FUNCT3] = {
    [VEMU_FUNCT_ADDI]           = VEMU_OPCODE_ADDI,
    [VEMU_FUNCT_SLTI]           = VEMU_OPCODE_SLTI,
    [VEMU_FUNCT_SLTIU]          = VEMU_OPCODE_SLTIU,
    [VEMU_FUNCT_XORI]           = VEMU_OPCODE_XORI,
    [VEMU_FUNCT_ORI]            = VEMU_OPCODE_ORI,
    [VEMU_FUNCT_ANDI]           = VEMU_OPCODE_ANDI,
    [VEMU_FUNCT_SLLI]           = VEMU_OPCODE_ILLEGAL,
    [VEMU_FUNCT_SRLI_SRAI]      = VEMU_OPCODE_ILLEGAL, /* TODO */
};

static vemu_opcode_t const vemu_rfunct_to_flat[VEMU_MAX_FUNCT3] = {
    [VEMU_FUNCT_ADD_SUB]        = VEMU_OPCODE_ILLEGAL,
    [VEMU_FUNCT_SLL]            = VEMU_OPCODE_SLL,
    [VEMU_FUNCT_SLT]            = VEMU_OPCODE_SLT,
    [VEMU_FUNCT_SLTU]           = VEMU_OPCODE_SLTU,
    [VEMU_FUNCT_XOR]            = VEMU_OPCODE_XOR,
    [VEMU_FUNCT_SRL_SRA]        = VEMU_OPCODE_ILLEGAL,
    [VEMU_FUNCT_OR]             = VEMU_OPCODE_OR,
    [VEMU_FUNCT_AND]            = VEMU_OPCODE_AND,
};

static char const *vemu_opcode_names[VEMU_MAX_OPCODES] = {
    [VEMU_OPCODE_ILLEGAL]       = "illegal",
    [VEMU_OPCODE_NOP]           = "nop",
    [VEMU_OPCODE_LUI]           = "lui",
    [VEMU_OPCODE_AUIPC]         = "auipc",
    [VEMU_OPCODE_JAL]           = "jal",
    [VEMU_OPCODE_JALR]          = "jalr",
    [VEMU_OPCODE_BEQ]           = "beq",
    [VEMU_OPCODE_BNE]           = "bne",
    [VEMU_OPCODE_BLT]           = "blt",
    [VEMU_OPCODE_BGE]           = "bge",
    [VEMU_OPCODE_BLTU]          = "bltu",
    [VEMU_OPCODE_BGEU]          = "bgeu",
    [VEMU_OPCODE_LB]            = "lb",
    [VEMU_OPCODE_LH]            = "lh",
    [VEMU_OPCODE_LW]            = "lw",
    [VEMU_OPCODE_LBU]           = "lbu",
    [VEMU_OPCODE_LHU]           = "lhu",
    [VEMU_OPCODE_SB]            = "sb",
    [VEMU_OPCODE_SH]            = "sh",
    [VEMU_OPCODE_SW]            = "sw",
    [VEMU_OPCODE_ADDI]          = "addi",
    [VEMU_OPCODE_SLTI]          = "slti",
    [VEMU_OPCODE_SLTIU]         = "sltiu",
    [VEMU_OPCODE_XORI]          = "xori",
    [VEMU_OPCODE_ORI]           = "ori",
    [VEMU_OPCODE_ANDI]          = "andi",
    [VEMU_OPCODE_SRLI]          = "srli",
    [VEMU_OPCODE_SLLI]          = "slli",
    [VEMU_OPCODE_SRAI]          = "srai",
    [VEMU_OPCODE_ADD]           = "add",
    [VEMU_OPCODE_SUB]           = "sub",
    [VEMU_OPCODE_SLL]           = "sll",
    [VEMU_OPCODE_SLT]           = "slt",
    [VEMU_OPCODE_SLTU]          = "sltu",
    [VEMU_OPCODE_XOR]           = "xor",
    [VEMU_OPCODE_SRL]           = "srl",
    [VEMU_OPCODE_SRA]           = "sra",
    [VEMU_OPCODE_OR]            = "or",
    [VEMU_OPCODE_AND]           = "and",
    [VEMU_OPCODE_FENCE]         = "fence",
    [VEMU_OPCODE_FENCE_TSO]     = "fence.tso",
    [VEMU_OPCODE_PAUSE]         = "pause",
    [VEMU_OPCODE_ECALL]         = "ecall",
    [VEMU_OPCODE_EBREAK]        = "ebreak",
};

static inline uint32_t vemu_sext(uint32_t value, uint32_t bits) {
    return (int32_t)(value << (32 - bits)) >> (32 - bits);
}

static inline void vemu_set_illegal_if_zero(vemu_decoded_t *dec, uint32_t nz) {
    if (nz == 0) {
        dec->opcode = VEMU_OPCODE_ILLEGAL;
    }
}

static void vemu_decode_c_addi4spn(uint32_t instr, vemu_decoded_t *dec) {
    dec->opcode = VEMU_OPCODE_ADDI;
    dec->rd = 8 + ((instr >> 2) & 0x3);
    dec->rs1 = VEMU_SP;
    dec->imm = (((instr >> 5) & 0x1) << 3)
             | (((instr >> 6) & 0x1) << 2)
             | (((instr >> 7) & 0xF) << 6)
             | (((instr >> 11) & 0x3) << 4);

    vemu_set_illegal_if_zero(dec, dec->imm);
}

static void vemu_decode_c_addi(uint32_t instr, vemu_decoded_t *dec) {
    dec->opcode = VEMU_OPCODE_ADDI;
    dec->rd = dec->rs1 = (instr >> 7) & 0x1F;
    dec->imm = vemu_sext(((instr >> 2) & 0x1F) 
                         | (((instr >> 12) & 0x1) << 5), 6);

    if (dec->rd == 0 && dec->imm == 0) {
        dec->opcode = VEMU_OPCODE_NOP;
    } else {
        dec->opcode = VEMU_OPCODE_ADDI;
        vemu_set_illegal_if_zero(dec, dec->imm);
    }
}

static void vemu_decode_c_li(uint32_t instr, vemu_decoded_t *dec) {
    dec->opcode = VEMU_OPCODE_ADDI;
    dec->rd = (instr >> 7) & 0x1F;
    dec->imm = vemu_sext(((instr >> 2) & 0x1F) 
                         | (((instr >> 12) & 0x1) << 5), 6);

    vemu_set_illegal_if_zero(dec, dec->rd);
}

static void vemu_decode_c_addi16sp_lui(uint32_t instr, vemu_decoded_t *dec) {
    uint8_t r = (instr >> 7) & 0x1F;

    if (r == VEMU_ZERO) {
        dec->opcode = VEMU_OPCODE_ILLEGAL;
    } else if (r == VEMU_SP) {
        dec->opcode = VEMU_OPCODE_ADDI;
        dec->rd = dec->rs1 = 2;
        uint32_t u = (((instr >> 2) & 0x1) << 5)
                   | (((instr >> 3) & 0x3) << 7)
                   | (((instr >> 5) & 0x1) << 6)
                   | (((instr >> 6) & 0x1) << 4)
                   | (((instr >> 12) & 0x1) << 9);
        dec->imm = vemu_sext(u, 10);
    } else {
        dec->imm = vemu_sext((((instr >> 2) & 0x1F) << 12)
                             | (((instr >> 12) & 0x1) << 17), 18);
    }
}

static void vemu_decode_c_jr_mv(uint32_t instr, vemu_decoded_t *dec) {
    uint8_t lo = (instr >> 2) & 0x1F;
    uint8_t hi = (instr >> 7) & 0x1F;

    if (lo != VEMU_ZERO && hi != VEMU_ZERO) {
        dec->opcode = VEMU_OPCODE_ADD;
        dec->rs2 = lo;
        dec->rd = hi;
    } else if (lo == VEMU_ZERO && hi != VEMU_ZERO) {
        dec->opcode = VEMU_OPCODE_JALR;
        dec->rs1 = hi;
    } else {
        dec->opcode = VEMU_OPCODE_ILLEGAL;
    }
}

static void vemu_decode_c_lwsp(uint32_t instr, vemu_decoded_t *dec) {
    dec->opcode = VEMU_OPCODE_LW;
    dec->rs1 = VEMU_SP;
    dec->rd = (instr >> 7) & 0x1F;
    dec->imm = (((instr >> 2) & 0x3) << 6)
             | (((instr >> 4) & 0x7) << 2)
             | (((instr >> 12) & 0x1) << 5);

    vemu_set_illegal_if_zero(dec, dec->rd);
}

static void vemu_decode_c_ebreak_jalr_add(uint32_t instr, vemu_decoded_t *dec) {
    uint8_t lo = (instr >> 2) & 0x1F;
    uint8_t hi = (instr >> 7) & 0x1F;

    if (lo != VEMU_ZERO && hi != VEMU_ZERO) {
        dec->opcode = VEMU_OPCODE_ADD;
        dec->rs2 = lo;
        dec->rs1 = hi;
        dec->rd = hi;
    } else if (lo == VEMU_ZERO && hi != VEMU_ZERO) {
        dec->opcode = VEMU_OPCODE_JALR;
        dec->rs1 = hi;
    } else if (lo == VEMU_ZERO && hi == VEMU_ZERO) {
        // ebreak
    } else {
        dec->opcode = VEMU_OPCODE_ILLEGAL;
    }
}

static void vemu_decode_c_swsp(uint32_t instr, vemu_decoded_t *dec) {
    dec->opcode = VEMU_OPCODE_SW;
    dec->rs1 = VEMU_SP;
    dec->rs2 = (instr >> 7) & 0x1F;
    dec->imm = (((instr >> 7) & 0x3) << 6) 
             | (((instr >> 9) & 0xF) << 2);
}

static void vemu_decode_compressed_q0(uint32_t instr, vemu_decoded_t *dec) {
    switch ((instr >> 13) & 0x7) {
        case 0x0:
            vemu_decode_c_addi4spn(instr, dec);
            break;

        default:
            break;
    }
}

static void vemu_decode_compressed_q1(uint32_t instr, vemu_decoded_t *dec) {    
    switch ((instr >> 13) & 0x7) {
        case 0x0:
            vemu_decode_c_addi(instr, dec);
            break;

        case 0x2:
            vemu_decode_c_li(instr, dec);
            break;

        case 0x3:
            vemu_decode_c_addi16sp_lui(instr, dec);
            break;

        default:
            break;
    }
}

static void vemu_decode_compressed_q2(uint32_t instr, vemu_decoded_t *dec) {
    switch ((instr >> 13) & 0x7) {
        case VEMU_OPCODE_C_LWSP:
            vemu_decode_c_lwsp(instr, dec);
            break;

        case 0x4:
            if (((instr >> 12) & 0x1) == 0x0) {
                vemu_decode_c_jr_mv(instr, dec);
            } else {
                vemu_decode_c_ebreak_jalr_add(instr, dec);
            }
            break;

        case VEMU_OPCODE_C_SWSP:
            vemu_decode_c_swsp(instr, dec);
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

static vemu_opcode_t vemu_decode_r_opcode(vemu_funct_t funct, uint32_t d) {
    vemu_opcode_t opcode = vemu_rfunct_to_flat[funct];

    switch (funct) {
        case VEMU_FUNCT_ADD_SUB:
            if (d == 0x00) {
                return VEMU_OPCODE_ADD;
            } else if (d == 0x20) {
                return VEMU_OPCODE_SUB;
            }
            break;

        case VEMU_FUNCT_SRL_SRA:    
            if (d == 0x00) {
                return VEMU_OPCODE_SRL;
            } else if (d == 0x20) {
                return VEMU_OPCODE_SRA;
            }
            break;

        default:
            if (d == 0x00) {
                return opcode;
            }
    }

    return VEMU_OPCODE_ILLEGAL;
}

static void vemu_decode_format_r(uint32_t instr, vemu_decoded_t *dec,
                                 vemu_regular_opcode_t ropcode) {
    VEMU_ASSERT(ropcode == VEMU_OPCODE_R_R);

    vemu_funct_t funct = (instr >> 12) & 0x7;
    uint32_t d = (instr >> 25) & 0x7F;
    dec->opcode = vemu_decode_r_opcode(funct, d);
    
    dec->rd = (instr >> 7) & 0x1F;
    dec->rs1 = (instr >> 15) & 0x1F;
    dec->rs2 = (instr >> 20) & 0x1F;                           
}

static vemu_opcode_t vemu_decode_shift_opcode(vemu_funct_t funct, uint32_t d) {    
    switch (funct) {
        case VEMU_FUNCT_SLLI:
            if (d == 0x00) {
                return VEMU_OPCODE_SLLI;
            }
            break;

        case VEMU_FUNCT_SRLI_SRAI:
            if (d == 0x00) {
                return VEMU_OPCODE_SRLI;
            } else if (d == 0x20) {
                return VEMU_OPCODE_SRAI;
            }
            break;

        default:
            break;
    }

    return VEMU_OPCODE_ILLEGAL;
}

static void vemu_decode_format_i(uint32_t instr, vemu_decoded_t *dec, 
                                 vemu_regular_opcode_t ropcode) {
    vemu_funct_t funct = (instr >> 12) & 0x7;
    switch (ropcode) {
        case VEMU_OPCODE_R_JALR:
            dec->opcode = VEMU_OPCODE_JALR;
            break;

        case VEMU_OPCODE_R_L:
            dec->opcode = vemu_lfunct_to_flat[funct];
            break;

        case VEMU_OPCODE_R_I:
            dec->opcode = vemu_ifunct_to_flat[funct];
            if (dec->opcode == VEMU_OPCODE_ILLEGAL) {
                uint32_t d = (instr >> 25) & 0x7F;
                dec->opcode = vemu_decode_shift_opcode(funct, d);
            }
            break;

        case VEMU_OPCODE_R_ECALL:
            dec->opcode = VEMU_OPCODE_ECALL;
            break;

        default:
            break;
    }

    dec->rd = ((instr) >> 7) & 0x1F;
    dec->rs1 = (instr >> 15) & 0x1F;

    switch (dec->opcode) {
        case VEMU_OPCODE_SLLI:
        case VEMU_OPCODE_SRLI:
        case VEMU_OPCODE_SRAI:  
            dec->imm = (instr >> 20) & 0x1F;
            break;

        default:    
            dec->imm = vemu_sext((instr >> 20) & 0xFFF, 12);
            break;
    }
}

static void vemu_decode_format_s(uint32_t instr, vemu_decoded_t *dec,
                                 vemu_regular_opcode_t ropcode) {
    VEMU_ASSERT(ropcode == VEMU_OPCODE_R_S);
    
    vemu_funct_t funct = (instr >> 12) & 0x7;
    dec->opcode = vemu_sfunct_to_flat[funct];

    dec->rs1 = (instr >> 15) & 0x1F;
    dec->rs2 = (instr >> 20) & 0x1F;
    dec->imm = vemu_sext(((instr >> 7) & 0x1F)
                         | (((instr >> 25)) << 5), 12);
}

static void vemu_decode_format_b(uint32_t instr, vemu_decoded_t *dec,
                                 vemu_regular_opcode_t ropcode) {
    VEMU_ASSERT(ropcode == VEMU_OPCODE_R_B);

    uint32_t funct = (instr >> 12) & 0x7;
    dec->opcode = vemu_bfunct_to_flat[funct];
               
    dec->rs1 = (instr >> 15) & 0x1F;
    dec->rs2 = (instr >> 20) & 0x1F;    

    uint32_t u = (((instr >> 7) & 0x1) << 11)
               | (((instr >> 8) & 0xF) << 1)
               | (((instr >> 25) & 0x3F) << 5)
               | (((instr >> 31) & 0x1) << 12);
    dec->imm = vemu_sext(u, 13);
}

static void vemu_decode_format_u(uint32_t instr, vemu_decoded_t *dec,
                                 vemu_regular_opcode_t ropcode) {
    switch (ropcode) {
        case VEMU_OPCODE_R_LUI:
            dec->opcode = VEMU_OPCODE_LUI;
            break;

        case VEMU_OPCODE_R_AUIPC:
            dec->opcode = VEMU_OPCODE_AUIPC;
            break;

        default:
            VEMU_UNREACHED();
    }

    dec->rd = (instr >> 7) & 0x1F;
    dec->imm = ((instr >> 12) & 0xFFFFF) << 12;
}

static void vemu_decode_format_j(uint32_t instr, vemu_decoded_t *dec,
                                 vemu_regular_opcode_t ropcode) {
    assert(ropcode == VEMU_OPCODE_R_JAL);
    dec->opcode = VEMU_OPCODE_JAL;
                    
    dec->rd = (instr >> 7) & 0x1F;

    uint32_t u = (((instr >> 12) & 0xFF) << 12)
               | (((instr >> 20) & 0x1) << 11)
               | (((instr >> 21) & 0x3FF) << 1)
               | (((instr >> 31) & 0x1) << 20);
    dec->imm = vemu_sext(u, 21);
}

static void vemu_decode_regular(uint32_t instr, vemu_decoded_t *dec) {
    vemu_regular_opcode_t ropcode = instr & 0x7F;
    
    switch (ropcode) {
        case VEMU_OPCODE_R_LUI:
        case VEMU_OPCODE_R_AUIPC:
            vemu_decode_format_u(instr, dec, ropcode);
            break;

        case VEMU_OPCODE_R_JAL:
            vemu_decode_format_j(instr, dec, ropcode);
            break;

        case VEMU_OPCODE_R_JALR:
            vemu_decode_format_i(instr, dec, ropcode);
            break;

        case VEMU_OPCODE_R_B:
            vemu_decode_format_b(instr, dec, ropcode);
            break;

        case VEMU_OPCODE_R_L:
            vemu_decode_format_i(instr, dec, ropcode);
            break;

        case VEMU_OPCODE_R_S:
            vemu_decode_format_s(instr, dec, ropcode);
            break;

        case VEMU_OPCODE_R_I:
            vemu_decode_format_i(instr, dec, ropcode);
            break;

        case VEMU_OPCODE_R_R:
            vemu_decode_format_r(instr, dec, ropcode);
            break;

        case VEMU_OPCODE_R_FENCE:
            // ?
            break;

        case VEMU_OPCODE_R_ECALL:
            vemu_decode_format_i(instr, dec, ropcode);
            break;

        default:
            break;
    }
}

static vemu_instruction_format_t vemu_opcode_to_format(vemu_opcode_t opcode) {
    switch (opcode) {
        case VEMU_OPCODE_ADD:
        case VEMU_OPCODE_SUB:
        case VEMU_OPCODE_SLL:
        case VEMU_OPCODE_SLT:
        case VEMU_OPCODE_SLTU:
        case VEMU_OPCODE_XOR:
        case VEMU_OPCODE_SRL:
        case VEMU_OPCODE_SRA:
        case VEMU_OPCODE_OR:
        case VEMU_OPCODE_AND:
            return VEMU_FORMAT_R;
            
        case VEMU_OPCODE_JALR:
        case VEMU_OPCODE_LB:
        case VEMU_OPCODE_LH:
        case VEMU_OPCODE_LW:
        case VEMU_OPCODE_LBU:
        case VEMU_OPCODE_LHU:
        case VEMU_OPCODE_ADDI:
        case VEMU_OPCODE_SLTI:
        case VEMU_OPCODE_SLTIU:
        case VEMU_OPCODE_XORI:
        case VEMU_OPCODE_ORI:
        case VEMU_OPCODE_ANDI:
        case VEMU_OPCODE_SRLI:
        case VEMU_OPCODE_SLLI:
        case VEMU_OPCODE_SRAI:
            return VEMU_FORMAT_I;

        case VEMU_OPCODE_SB:
        case VEMU_OPCODE_SH:
        case VEMU_OPCODE_SW:
            return VEMU_FORMAT_S;

        case VEMU_OPCODE_BEQ:
        case VEMU_OPCODE_BNE:
        case VEMU_OPCODE_BLT:
        case VEMU_OPCODE_BGE:
        case VEMU_OPCODE_BLTU:
        case VEMU_OPCODE_BGEU:
            return VEMU_FORMAT_B;

        case VEMU_OPCODE_LUI:
        case VEMU_OPCODE_AUIPC:
            return VEMU_FORMAT_U;

        case VEMU_OPCODE_JAL:
            return VEMU_FORMAT_J;

        case VEMU_OPCODE_NOP: /* TODO */
        case VEMU_OPCODE_FENCE:
        case VEMU_OPCODE_FENCE_TSO:
        case VEMU_OPCODE_PAUSE:
        case VEMU_OPCODE_ECALL:
        case VEMU_OPCODE_EBREAK:
            return VEMU_FORMAT_I;
        
        case VEMU_OPCODE_ILLEGAL:
            break;
    }

    VEMU_UNREACHED();
}

void vemu_disassemble(vemu_decoded_t *dec, uint32_t instr, uint32_t ip) {
    char const *opcode = vemu_opcode_names[dec->opcode];
    char const *rd = vemu_register_name(dec->rd);
    char const *rs1 = vemu_register_name(dec->rs1);
    char const *rs2 = vemu_register_name(dec->rs2);

    fprintf(stderr, "%8x: ", ip);

    if (dec->c) {
        fprintf(stderr, "    %04x    ", instr);
    } else {
        fprintf(stderr, "%08x    ", instr);
    }

    if (dec->opcode == VEMU_OPCODE_ILLEGAL) {
        fprintf(stderr, "illegal instruction\n");
        return;
    }

    vemu_instruction_format_t format = vemu_opcode_to_format(dec->opcode);

    switch (format) {
        case VEMU_FORMAT_R:
            fprintf(stderr, "%s %s,%s,%s\n", opcode, rd, rs1, rs2);
            break;

        case VEMU_FORMAT_I:
            fprintf(stderr, "%s %s,%s,%d\n", opcode, rd, rs1, dec->imm);
            break;

        case VEMU_FORMAT_S:
            fprintf(stderr, "%s %s,%d(%s)\n", opcode, rs2, dec->imm, rs1);
            break;

        case VEMU_FORMAT_B:
            fprintf(stderr, "%s %s,%s,%x\n", opcode, rs1, rs2, ip + dec->imm);
            break;

        case VEMU_FORMAT_U:
            fprintf(stderr, "%s %s,0x%x\n", opcode, rd, dec->imm);
            break;

        case VEMU_FORMAT_J:
            fprintf(stderr, "%s %s,%x\n", opcode, rd, ip + dec->imm);
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

#define EXEC_FUNC(op) \
        static inline void vemu_exec_##op(vemu_cpu_t *cpu, vemu_decoded_t *dec)

EXEC_FUNC(ILLEGAL) {
    (void)dec;
    cpu->terminated = true;
}

EXEC_FUNC(NOP) {
    (void)cpu, (void)dec;
}

EXEC_FUNC(LUI) {
    cpu->regs[dec->rd] = dec->imm;
}

EXEC_FUNC(AUIPC) {
    cpu->regs[dec->rd] = cpu->ip + dec->imm;
}

EXEC_FUNC(JAL) {
    cpu->regs[dec->rd] = cpu->next_ip;
    cpu->next_ip = cpu->ip + dec->imm;
}

EXEC_FUNC(JALR) {
    cpu->regs[dec->rd] = cpu->next_ip;
    cpu->next_ip = (cpu->regs[dec->rs1] + dec->imm) & 0xFFFFFFFE;
}

EXEC_FUNC(BEQ) {
    if (cpu->regs[dec->rs1] == cpu->regs[dec->rs2]) {
        cpu->next_ip = cpu->ip + dec->imm;
    }
}

EXEC_FUNC(BNE) {
    if (cpu->regs[dec->rs1] != cpu->regs[dec->rs2]) {
        cpu->next_ip = cpu->ip + dec->imm;
    }
}

EXEC_FUNC(BLT) {
    if ((int32_t)cpu->regs[dec->rs1] < (int32_t)cpu->regs[dec->rs2]) {
        cpu->next_ip = cpu->ip + dec->imm;
    }
}

EXEC_FUNC(BGE) {
    if ((int32_t)cpu->regs[dec->rs1] >= (int32_t)cpu->regs[dec->rs2]) {
        cpu->next_ip = cpu->ip + dec->imm;
    }
}

EXEC_FUNC(BLTU) {
    if (cpu->regs[dec->rs1] < cpu->regs[dec->rs2]) {
        cpu->next_ip = cpu->ip + dec->imm;
    }
}

EXEC_FUNC(BGEU) {
    if (cpu->regs[dec->rs1] >= cpu->regs[dec->rs2]) {
        cpu->next_ip = cpu->ip + dec->imm;
    }
}

EXEC_FUNC(LB) {
    uint32_t addr = cpu->regs[dec->rs1] + dec->imm;
    uint32_t value = vemu_ram_load_byte(*cpu->ram, addr);
    cpu->regs[dec->rd] = vemu_sext(value, 8);
}

EXEC_FUNC(LH) {
    uint32_t addr = cpu->regs[dec->rs1] + dec->imm;
    uint32_t value = vemu_ram_load_half(*cpu->ram, addr);
    cpu->regs[dec->rd] = vemu_sext(value, 16);
}

EXEC_FUNC(LW) {
    uint32_t addr = cpu->regs[dec->rs1] + dec->imm;
    uint32_t value = vemu_ram_load_word(*cpu->ram, addr);
    cpu->regs[dec->rd] = value;
}

EXEC_FUNC(LBU) {
    uint32_t addr = cpu->regs[dec->rs1] + dec->imm;
    uint32_t value = vemu_ram_load_byte(*cpu->ram, addr);
    cpu->regs[dec->rd] = value;
}

EXEC_FUNC(LHU) {
    uint32_t addr = cpu->regs[dec->rs1] + dec->imm;
    cpu->regs[dec->rd] = vemu_ram_load_half(*cpu->ram, addr);
}

EXEC_FUNC(SB) {
    uint32_t addr = cpu->regs[dec->rs1] + dec->imm;
    vemu_ram_store_byte(*cpu->ram, addr, cpu->regs[dec->rs2]);
}

EXEC_FUNC(SH) {
    uint32_t addr = cpu->regs[dec->rs1] + dec->imm;
    vemu_ram_store_half(*cpu->ram, addr, cpu->regs[dec->rs2]);
}

EXEC_FUNC(SW) {
    uint32_t addr = cpu->regs[dec->rs1] + dec->imm;
    vemu_ram_store_word(*cpu->ram, addr, cpu->regs[dec->rs2]);
}

EXEC_FUNC(ADDI) {
    cpu->regs[dec->rd] = cpu->regs[dec->rs1] + dec->imm;
}

EXEC_FUNC(SLTI) {
    cpu->regs[dec->rd] = (int32_t)cpu->regs[dec->rs1] < (int32_t)dec->imm;
}

EXEC_FUNC(SLTIU) {
    cpu->regs[dec->rd] = cpu->regs[dec->rs1] < dec->imm;
}

EXEC_FUNC(XORI) {
    cpu->regs[dec->rd] = cpu->regs[dec->rs1] ^ dec->imm;
}

EXEC_FUNC(ORI) {
    cpu->regs[dec->rd] = cpu->regs[dec->rs1] | dec->imm;
}

EXEC_FUNC(ANDI) {
    cpu->regs[dec->rd] = cpu->regs[dec->rs1] & dec->imm;
}

EXEC_FUNC(SRLI) {
    cpu->regs[dec->rd] = cpu->regs[dec->rs1] >> dec->imm;
}

EXEC_FUNC(SLLI) {
    cpu->regs[dec->rd] = cpu->regs[dec->rs1] << dec->imm;
}

EXEC_FUNC(SRAI) {
    cpu->regs[dec->rd] = (int32_t)cpu->regs[dec->rs1] >> dec->imm;
}

EXEC_FUNC(ADD) {
    cpu->regs[dec->rd] = cpu->regs[dec->rs1] + cpu->regs[dec->rs2];
}

EXEC_FUNC(SUB) {
    cpu->regs[dec->rd] = cpu->regs[dec->rs1] - cpu->regs[dec->rs2];
}

EXEC_FUNC(SLL) {
    cpu->regs[dec->rd] = cpu->regs[dec->rs1] >> cpu->regs[dec->rs2];
}

EXEC_FUNC(SLT) {
    int32_t x = cpu->regs[dec->rs1], y = cpu->regs[dec->rs2];
    cpu->regs[dec->rd] = x < y;
}

EXEC_FUNC(SLTU) {
    uint32_t shamt = cpu->regs[dec->rs2] & 0x1F;
    cpu->regs[dec->rd] = cpu->regs[dec->rs1] < shamt;
}

EXEC_FUNC(XOR) {
    cpu->regs[dec->rd] = cpu->regs[dec->rs1] ^ cpu->regs[dec->rs2];
}

EXEC_FUNC(SRL) {
    uint32_t shamt = cpu->regs[dec->rs2] & 0x1F;
    cpu->regs[dec->rd] = cpu->regs[dec->rs1] >> shamt;
}

EXEC_FUNC(SRA) {
    uint32_t shamt = cpu->regs[dec->rs2] & 0x1F;
    cpu->regs[dec->rd] = (int32_t)cpu->regs[dec->rs1] >> shamt;
}

EXEC_FUNC(OR) {
    cpu->regs[dec->rd] = cpu->regs[dec->rs1] | cpu->regs[dec->rs2];
}

EXEC_FUNC(AND) {
    cpu->regs[dec->rd] = cpu->regs[dec->rs1] & cpu->regs[dec->rs2];
}

EXEC_FUNC(FENCE) {
    (void)cpu, (void)dec;
}

EXEC_FUNC(FENCE_TSO) {
    (void)cpu, (void)dec;
}

EXEC_FUNC(PAUSE) {
    (void)cpu, (void)dec;
}

EXEC_FUNC(ECALL) {
    (void)dec;

    uint32_t res = 0;

    switch (cpu->regs[VEMU_A7]) {
        case VEMU_ECALL_PRINT_INT:
            printf(">> %d\n", cpu->regs[VEMU_A0]);
            break;

        case VEMU_ECALL_PRINT_CHAR:
            printf("%c", cpu->regs[VEMU_A0]);
            break;

        case VEMU_ECALL_START_TRACE:
            cpu->trace = 0;
            break;

        case VEMU_ECALL_TRACE_RESULT:
            res = cpu->trace;
            break;

        case VEMU_ECALL_TEST_ASSERT: {
            uint32_t x = cpu->regs[VEMU_A1], y = cpu->regs[VEMU_A2];
            if (x != y) {
                fprintf(stderr, "line %d: assertion failed: %d != %d\n", 
                        cpu->regs[VEMU_A0], x, y);
            }
            break;
        }

        default:
            fprintf(stderr, "unsupported ecall: %d\n", 
                    cpu->regs[VEMU_A7]);
            cpu->terminated = true;
            break;
    }

    cpu->regs[VEMU_A0] = res;
}

EXEC_FUNC(EBREAK) {
    (void)cpu, (void)dec;
}

#define DISPATCH(op) case VEMU_OPCODE_##op: vemu_exec_##op(cpu, &dec); break;

static void vemu_fetch_and_decode(vemu_cpu_t *cpu, vemu_decoded_t *dec) {
    uint32_t instr = vemu_ram_load_half(*cpu->ram, cpu->ip);
    
    if (VEMU_IS_COMPRESSED(instr)) {
        vemu_decode_compressed(instr, dec);

        cpu->next_ip = cpu->ip + 2;
    } else {
        instr = instr | (vemu_ram_load_half(*cpu->ram, cpu->ip + 2) << 16);
        vemu_decode_regular(instr, dec);

        cpu->next_ip = cpu->ip + 4;
    }

    if (0) {
        vemu_disassemble(dec, instr, cpu->ip);
    }
}

void vemu_cpu_run(vemu_cpu_t *cpu, uint32_t entry) {
    cpu->ip = entry;
    cpu->regs[2] = 0x20000;

    while (!cpu->terminated) {
        cpu->regs[VEMU_ZERO] = 0;

        vemu_decoded_t dec = { 0, };
        vemu_fetch_and_decode(cpu, &dec);

        switch (dec.opcode) {
            DISPATCH(ILLEGAL)
            DISPATCH(NOP)
            DISPATCH(LUI);
            DISPATCH(AUIPC);
            DISPATCH(JAL)
            DISPATCH(JALR)
            DISPATCH(BEQ)
            DISPATCH(BNE)
            DISPATCH(BLT)
            DISPATCH(BGE)
            DISPATCH(BLTU)
            DISPATCH(BGEU)
            DISPATCH(LB)
            DISPATCH(LH)
            DISPATCH(LW)
            DISPATCH(LBU)
            DISPATCH(LHU)
            DISPATCH(SB)
            DISPATCH(SH)
            DISPATCH(SW)
            DISPATCH(ADDI)
            DISPATCH(SLTI)
            DISPATCH(SLTIU)
            DISPATCH(XORI)
            DISPATCH(ORI)
            DISPATCH(ANDI)
            DISPATCH(SRLI)
            DISPATCH(SLLI)
            DISPATCH(SRAI)
            DISPATCH(ADD)
            DISPATCH(SUB)
            DISPATCH(SLL)
            DISPATCH(SLT)
            DISPATCH(SLTU)
            DISPATCH(XOR)
            DISPATCH(SRL)
            DISPATCH(SRA)
            DISPATCH(OR)
            DISPATCH(AND)
            DISPATCH(FENCE)
            DISPATCH(FENCE_TSO)
            DISPATCH(PAUSE)
            DISPATCH(ECALL)
            DISPATCH(EBREAK)
        }

        cpu->ip = cpu->next_ip;
        cpu->trace++;
    }
}
