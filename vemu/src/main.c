#include "elf-file.h"
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

    FILE *file = fopen(args.filename, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file: '%s'\n", args.filename);
        return 1;
    }

    vemu_elf_header_t elf;
    if (!vemu_read_elf_header(file, &elf)) {
        return 1;
    }

    if (!vemu_validate_elf_header(&elf)) {
        return 1;
    }

    fprintf(stderr, "found %d program headers...\n", elf.e_phnum);
    for (size_t i = 0; i < elf.e_phnum; i++) {
        vemu_elf_program_header_t ph;
        if (!vemu_read_program_header(file, &elf, &ph, i)) {
            return 1;
        }

        fprintf(stderr, "%ld: %x at %d: size %d, %d in mem\n", 
                i, ph.p_type, ph.p_vaddr, ph.p_filesz, ph.p_memsz);
    }

    fprintf(stderr, "found %d section headers...\n", elf.e_phnum);
    for (size_t i = 0; i < elf.e_phnum; i++) {
        vemu_elf_section_header_t sh;
        if (!vemu_read_section_header(file, &elf, &sh, i)) {
            return 1;
        }

        fprintf(stderr, "%ld: %x at %d: size %d\n", 
                i, sh.sh_type, sh.sh_addr, sh.sh_size);
    }

    return 0;
}
