#include "system.h"
#include <stdlib.h>
#include <stddef.h>

void vemu_system_init(vemu_system_t *sys) {
    vemu_cpu_init(&sys->cpu, &sys->ram);
    sys->ram = NULL;
}

void vemu_system_destruct(vemu_system_t *sys) {
    if (sys->ram != NULL) {
        free(sys->ram);
    }
}

void vemu_system_add_ram(vemu_system_t *sys, uint8_t *ram) {
    sys->ram = ram;
}
