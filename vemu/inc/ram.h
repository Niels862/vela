#ifndef VEMU_RAM_H
#define VEMU_RAM_H

#include <inttypes.h>

uint8_t vemu_ram_load_byte(uint8_t *ram, uint32_t addr);

void vemu_ram_store_byte(uint8_t *ram, uint32_t addr, uint8_t byte);

uint16_t vemu_ram_load_half(uint8_t *ram, uint32_t addr);

void vemu_ram_store_half(uint8_t *ram, uint32_t addr, uint16_t half);

uint32_t vemu_ram_load_word(uint8_t *ram, uint32_t addr);

void vemu_ram_store_word(uint8_t *ram, uint32_t addr, uint32_t word);

#endif
