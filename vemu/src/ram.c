#include "ram.h"

uint8_t vemu_ram_load_byte(uint8_t *ram, uint32_t addr) {
    return ram[addr];
}

void vemu_ram_store_byte(uint8_t *ram, uint32_t addr, uint8_t byte) {
    ram[addr] = byte;
}

uint16_t vemu_ram_load_half(uint8_t *ram, uint32_t addr) {
    return (ram[addr]) | (ram[addr + 1] << 8);
}

void vemu_ram_store_half(uint8_t *ram, uint32_t addr, uint16_t half) {
    ram[addr] = half & 0xFF;
    ram[addr + 1] = (half >> 8) & 0xFF;
}

uint32_t vemu_ram_load_word(uint8_t *ram, uint32_t addr) {
    return ram[addr] | (ram[addr + 1] << 8)
        | (ram[addr + 2] << 16) | (ram[addr + 3] << 24);
}

void vemu_ram_store_word(uint8_t *ram, uint32_t addr, uint32_t word) {
    ram[addr] = word & 0xFF;
    ram[addr + 1] = (word >> 8) & 0xFF;
    ram[addr + 2] = (word >> 16) & 0xFF;
    ram[addr + 3] = (word >> 24) & 0xFF;
}
