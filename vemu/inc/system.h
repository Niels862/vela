#ifndef VEMU_SYSTEM_H
#define VEMU_SYSTEM_H

#include "cpu.h"
#include <inttypes.h>

typedef struct {
    vemu_cpu_t cpu;
    uint8_t *ram;
} vemu_system_t;

void vemu_system_init(vemu_system_t *sys);

void vemu_system_destruct(vemu_system_t *sys);

void vemu_system_add_ram(vemu_system_t *sys, uint8_t *ram);

#endif

