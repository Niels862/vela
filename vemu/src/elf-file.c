#include "elf-file.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void vemu_elf_init(vemu_elf_t *elf) {
    elf->file = NULL;
    elf->strtab = NULL;
}

bool vemu_elf_open(vemu_elf_t *elf, char const *filename) {
    elf->file = fopen(filename, "rb");
    if (elf->file == NULL) {
        fprintf(stderr, "could not open file: '%s'\n", filename);
        return false;
    }

    if (!vemu_read_elf_header(elf->file, &elf->h)) {
        return false;
    }

    if (!vemu_validate_elf_header(&elf->h)) {
        return false;
    }

    vemu_elf_section_header_t shstrtab;
    if (!vemu_read_section_header(elf->file, &elf->h, &shstrtab, 
                                  elf->h.e_shstrndx)) {
        return false;
    }

    elf->strtab = vemu_load_section_content(elf->file, &shstrtab);
    if (elf->strtab == NULL) {
        return false;
    }

    return true;
}

bool vemu_elf_load(vemu_elf_t *elf, uint8_t *ram) {
    for (size_t i = 0; i < elf->h.e_phnum; i++) {
        vemu_elf_program_header_t ph;
        if (!vemu_read_program_header(elf->file, &elf->h, &ph, i)) {
            return false;
        }

        if (ph.p_type != ELF_PT_LOAD) {
            continue;
        }

        if (!vemu_load_program(elf->file, &ph, ram)) {
            fprintf(stderr, "failed to load program\n");
            return false;
        }
    }

    return true;
}

void vemu_elf_destruct(vemu_elf_t *elf) {
    if (elf->file != NULL) {
        fclose(elf->file);
    }

    if (elf->strtab != NULL) {
        free(elf->strtab);
    }
}

bool vemu_read_elf_header(FILE *file, vemu_elf_header_t *elf) {
    size_t size = fread(elf, 1, sizeof(vemu_elf_header_t), file);
    if (size < sizeof(vemu_elf_header_t)) {
        return false;
    }
    
    return true;
}

bool vemu_validate_elf_header(vemu_elf_header_t *elf) {
    if (elf->e_ident.ei_mag[0] != ELF_MAG0
            || elf->e_ident.ei_mag[1] != ELF_MAG1
            || elf->e_ident.ei_mag[2] != ELF_MAG2
            || elf->e_ident.ei_mag[3] != ELF_MAG3) {   
        fprintf(stderr, "ELF magic number mismatch");
        return false;
    }
    
    if (elf->e_ident.ei_class != ELF_CLASS32) {
        fprintf(stderr, "Only ELF 32-bit format is supported\n");
        return false;
    }

    if (elf->e_ident.ei_data != ELF_LTLENDN) {
        fprintf(stderr, "Only ELF little endian is supported\n");
        return false;
    }

    if (elf->e_type != ELF_ET_EXEC) {
        fprintf(stderr, "Only ELF executable format is supported\n");
        return false;
    }

    if (elf->e_machine != ELF_RISCV) {
        fprintf(stderr, "Only RISC-V format is supported\n");
        return false;
    }

    return true;
}

bool vemu_read_program_header(FILE *file, vemu_elf_header_t *elf, 
                              vemu_elf_program_header_t *ph,
                              size_t i) {
    assert(elf->e_phentsize == sizeof(vemu_elf_program_header_t));
    
    fseek(file, elf->e_phoff + i * elf->e_phentsize, SEEK_SET);

    size_t size = fread(ph, 1, sizeof(vemu_elf_program_header_t), file);
    if (size < sizeof(vemu_elf_program_header_t)) {
        return false;
    }
    
    return true;                        
}

bool vemu_load_program(FILE *file, vemu_elf_program_header_t *ph,
                       uint8_t *ram) {
    assert(ph->p_type == ELF_PT_LOAD);
    assert(ph->p_filesz <= ph->p_memsz);

    if (ph->p_filesz > 0) {
        fseek(file, ph->p_offset, SEEK_SET);

        size_t size = fread(ram + ph->p_vaddr, 1, ph->p_filesz, file);
        if (size < ph->p_filesz) {
            return false;
        }
    }

    if (ph->p_memsz > ph->p_filesz) {
        memset(ram + ph->p_vaddr + ph->p_filesz, 0, ph->p_memsz - ph->p_filesz);
    }

    return true;
}

bool vemu_read_section_header(FILE *file, vemu_elf_header_t *elf, 
                              vemu_elf_section_header_t *sh,
                              size_t i) {
    assert(elf->e_shentsize == sizeof(vemu_elf_section_header_t));

    fseek(file, elf->e_shoff + i * elf->e_shentsize, SEEK_SET);

    size_t size = fread(sh, 1, sizeof(vemu_elf_section_header_t), file);
    if (size < sizeof(vemu_elf_section_header_t)) {
        return false;
    }
    
    return true;                                                
}

uint8_t *vemu_load_section_content(FILE *file, vemu_elf_section_header_t *sh) {
    if (sh->sh_size == 0) {
        return NULL;
    }

    uint8_t *data = malloc(sh->sh_size);
    if (data == NULL) {
        return NULL;
    }

    fseek(file, sh->sh_offset, SEEK_SET);

    size_t size = fread(data, 1, sh->sh_size, file);
    if (size < sh->sh_size) {
        return NULL;
    }

    return data;
}
