#include "elf-file.h"
#include "system.h"
#include <stdlib.h>
#include <stdio.h>
#include <argp.h>
#include <string.h>

static struct argp_option options[] = {
    { "verbose", 'v', 0, 0, "Enable verbose output", 0 },
    { 0 }
};

typedef struct {
    char *filename;
    int verbose;
} vemu_args_t;

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    vemu_args_t *args = state->input;

    switch (key) {
        case 'v':
            args->verbose = 1;
            break;

        case ARGP_KEY_ARG:
            if (state->arg_num == 0) {
                args->filename = arg;
            } else {
                argp_usage(state);
            }
            break;

        case ARGP_KEY_END:
            if (state->arg_num < 1) {
                argp_usage(state);
            }
            break;

        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = { options, parse_opt, NULL, NULL, NULL, NULL, NULL };

int main(int argc, char **argv) {
    vemu_args_t args = { 0 };
    argp_parse(&argp, argc, argv, 0, 0, &args);

    int res = 0;

    vemu_system_t sys;
    vemu_system_init(&sys);

    uint8_t *ram = malloc(1024 * 1024 * 1024);
    if (ram == NULL) {
        return 1;
    }

    vemu_system_add_ram(&sys, ram);

    vemu_elf_t elf;
    vemu_elf_init(&elf);

    if (!vemu_elf_open(&elf, args.filename)) {
        res = 1;
        goto end;
    }

    if (!vemu_elf_load(&elf, sys.ram)) {
        res = 1;
        goto end;
    }

    vemu_cpu_run(&sys.cpu, elf.h.e_entry);

end:
    vemu_elf_destruct(&elf);
    vemu_system_destruct(&sys);

    return res;
}
