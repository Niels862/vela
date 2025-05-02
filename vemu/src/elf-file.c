#include "elf-file.h"
#include "assert.h"

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
