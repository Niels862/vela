#include "cpu.h"
#include "ram.h"
#include "util.h"
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>

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
    [VEMU_FUNCT_ADDI]           = VEMU_OPCODE_ADDI
};

static char const *vemu_opcode_names[VEMU_MAX_OPCODES] = {
    [VEMU_OPCODE_ILLEGAL]       = "illegal",
    [VEMU_OPCODE_ADD]           = "add",
    [VEMU_OPCODE_ADDI]          = "addi",
    [VEMU_OPCODE_ECALL]         = "ecall",
    [VEMU_OPCODE_LB]            = "lb",
    [VEMU_OPCODE_LH]            = "lh",
    [VEMU_OPCODE_LW]            = "lw",
    [VEMU_OPCODE_LBU]           = "lbu",
    [VEMU_OPCODE_LHU]           = "lhu",
    [VEMU_OPCODE_SB]            = "sb",
    [VEMU_OPCODE_SH]            = "sh",
    [VEMU_OPCODE_SW]            = "sw",
    [VEMU_OPCODE_JALR]          = "jalr",
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

static void vemu_decode_format_i(uint32_t instr, vemu_decoded_t *dec, 
                                 vemu_regular_opcode_t ropcode) {
    uint32_t funct = (instr >> 12) & 0x7;
    switch (ropcode) {
        case VEMU_OPCODE_R_JALR:
            dec->opcode = VEMU_OPCODE_JALR;
            break;

        case VEMU_OPCODE_R_L:
            dec->opcode = vemu_lfunct_to_flat[funct];
            break;

        case VEMU_OPCODE_R_I:
            dec->opcode = vemu_ifunct_to_flat[funct];
            break;

        case VEMU_OPCODE_R_ECALL:
            dec->opcode = VEMU_OPCODE_ECALL;
            break;

        default:
            VEMU_UNREACHED();
    }
    VEMU_ASSERT(dec->opcode != 0);      

    dec->rd = ((instr) >> 7) & 0x1F;
    dec->rs1 = (instr >> 15) & 0x1F;
    dec->imm = vemu_sext((instr >> 20) & 0xFFF, 12);
}

static void vemu_decode_format_s(uint32_t instr, vemu_decoded_t *dec,
                                 vemu_regular_opcode_t ropcode) {
    VEMU_ASSERT(ropcode == VEMU_OPCODE_R_S);
    
    uint32_t funct = (instr >> 12) & 0x7;
    dec->opcode = vemu_sfunct_to_flat[funct];
    VEMU_ASSERT(dec->opcode != 0);      

    dec->rs1 = (instr >> 15) & 0x1F;
    dec->rs2 = (instr >> 20) & 0x1F;
    dec->imm = vemu_sext(((instr >> 7) & 0x1F)
                         | (((instr >> 25)) << 5), 12);
}

static void vemu_decode_regular(uint32_t instr, vemu_decoded_t *dec) {
    switch (instr & 0x7F) {
        case VEMU_OPCODE_R_JALR:
            vemu_decode_format_i(instr, dec, VEMU_OPCODE_R_JALR);
            break;

        case VEMU_OPCODE_R_L:
            vemu_decode_format_i(instr, dec, VEMU_OPCODE_R_L);
            break;

        case VEMU_OPCODE_R_S:
            vemu_decode_format_s(instr, dec, VEMU_OPCODE_R_S);
            break;

        case VEMU_OPCODE_R_I:
            vemu_decode_format_i(instr, dec, VEMU_OPCODE_R_I);
            break;

        case VEMU_OPCODE_R_ECALL:
            vemu_decode_format_i(instr, dec, VEMU_OPCODE_R_ECALL);
            break;

        default:
            VEMU_UNREACHED();
    }
}

static vemu_instruction_format_t vemu_opcode_to_format(vemu_opcode_t opcode) {
    switch (opcode) {
        case VEMU_OPCODE_NOP: /* TODO */
        case VEMU_OPCODE_ADD:
            return VEMU_INSTRFORMAT_R;

        case VEMU_OPCODE_JALR:
        case VEMU_OPCODE_ADDI:
        case VEMU_OPCODE_LB:
        case VEMU_OPCODE_LH:
        case VEMU_OPCODE_LW:
        case VEMU_OPCODE_LBU:
        case VEMU_OPCODE_LHU:
        case VEMU_OPCODE_ECALL:
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
    char const *rd = vemu_register_name(dec->rd);
    char const *rs1 = vemu_register_name(dec->rs1);
    char const *rs2 = vemu_register_name(dec->rs2);

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
    cpu->regs[2] = 0x20000;

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
            
        if (dec.opcode == VEMU_OPCODE_ILLEGAL) {
            fprintf(stderr, "illegal instruction\n");
        } else {
            vemu_disassemble(&dec);
        }

        uint32_t addr, value;
        switch (dec.opcode) {
            case VEMU_OPCODE_ILLEGAL:
                terminated = true;
                break;

            case VEMU_OPCODE_NOP:
                break;

            case VEMU_OPCODE_ADD:
                cpu->regs[dec.rd] = cpu->regs[dec.rs1] + cpu->regs[dec.rs2];
                break;

            case VEMU_OPCODE_ADDI:
                cpu->regs[dec.rd] = cpu->regs[dec.rs1] + dec.imm;
                break;

            case VEMU_OPCODE_LB:
                addr = cpu->regs[dec.rs1] + dec.imm;
                value = vemu_ram_load_byte(*cpu->ram, addr);
                cpu->regs[dec.rd] = vemu_sext(value, 8);
                break;

            case VEMU_OPCODE_LH:
                addr = cpu->regs[dec.rs1] + dec.imm;
                value = vemu_ram_load_half(*cpu->ram, addr);
                cpu->regs[dec.rd] = vemu_sext(value, 16);
                break;

            case VEMU_OPCODE_LW:
                addr = cpu->regs[dec.rs1] + dec.imm;
                cpu->regs[dec.rd] = vemu_ram_load_word(*cpu->ram, addr);
                break;

            case VEMU_OPCODE_LBU: 
                addr = cpu->regs[dec.rs1] + dec.imm;
                cpu->regs[dec.rd] = vemu_ram_load_byte(*cpu->ram, addr);
                break;

            case VEMU_OPCODE_LHU:
                addr = cpu->regs[dec.rs1] + dec.imm;
                cpu->regs[dec.rd] = vemu_ram_load_half(*cpu->ram, addr);
                break;

            case VEMU_OPCODE_SB:
                addr = cpu->regs[dec.rs1] + dec.imm;
                vemu_ram_store_byte(*cpu->ram, addr, cpu->regs[dec.rs2]);
                break;

            case VEMU_OPCODE_SH:
                addr = cpu->regs[dec.rs1] + dec.imm;
                vemu_ram_store_half(*cpu->ram, addr, cpu->regs[dec.rs2]);
                break;

            case VEMU_OPCODE_SW:
                addr = cpu->regs[dec.rs1] + dec.imm;
                vemu_ram_store_word(*cpu->ram, addr, cpu->regs[dec.rs2]);
                break;

            case VEMU_OPCODE_JALR:
                break;

            case VEMU_OPCODE_ECALL:
                switch (cpu->regs[VEMU_A7]) {
                    case 0:
                        fprintf(stderr, ">> %d\n", cpu->regs[VEMU_A0]);
                        break;
                }
        }
    }
}
