#ifndef VEMU_ELF_H
#define VEMU_ELF_H

#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>

typedef struct {
    struct {
        uint8_t ei_mag[4];
        uint8_t ei_class;
        uint8_t ei_data;
        uint8_t ei_version;
        uint8_t ei_osabi;
        uint8_t ei_abiversion;
        uint8_t ei_pad[7];
    } e_ident;
    uint16_t    e_type;
    uint16_t    e_machine;
    uint32_t    e_version;
    uint32_t    e_entry;
    uint32_t    e_phoff;
    uint32_t    e_shoff;
    uint32_t    e_flags;
    uint16_t    e_ehsize;
    uint16_t    e_phentsize;
    uint16_t    e_phnum;
    uint16_t    e_shentsize;
    uint16_t    e_shnum;
    uint16_t    e_shstrndx;
} vemu_elf_header_t;

#define ELF_MAG0        0x7F
#define ELF_MAG1        'E'
#define ELF_MAG2        'L'
#define ELF_MAG3        'F'

#define ELF_CLASS32     1
#define ELF_LTLENDN     1
#define ELF_ET_EXEC     2

#define ELF_RISCV       0xF3

typedef struct {
    uint32_t    p_type;
    uint32_t    p_offset;
    uint32_t    p_vaddr;
    uint32_t    p_paddr;
    uint32_t    p_filesz;
    uint32_t    p_memsz;
    uint32_t    p_flags;
    uint32_t    p_align;
} vemu_elf_program_header_t;

#define ELF_PT_LOAD     1

typedef struct {
    uint32_t sh_name;
    uint32_t sh_type;
    uint32_t sh_flags;
    uint32_t sh_addr;
    uint32_t sh_offset;
    uint32_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint32_t sh_addralign;
    uint32_t sh_entsize;
} vemu_elf_section_header_t;

typedef struct {
    FILE *file;
    uint8_t *strtab;
    vemu_elf_header_t h;
} vemu_elf_t;

void vemu_elf_init(vemu_elf_t *elf);

bool vemu_elf_open(vemu_elf_t *elf, char const *filename);

bool vemu_elf_load(vemu_elf_t *elf, uint8_t *ram);

void vemu_elf_destruct(vemu_elf_t *elf);

bool vemu_read_elf_header(FILE *file, vemu_elf_header_t *elf);

bool vemu_validate_elf_header(vemu_elf_header_t *elf);

bool vemu_read_program_header(FILE *file, vemu_elf_header_t *elf, 
                              vemu_elf_program_header_t *ph,
                              size_t i);

bool vemu_load_program(FILE *file, vemu_elf_program_header_t *ph,
                       uint8_t *ram);

bool vemu_read_section_header(FILE *file, vemu_elf_header_t *elf, 
                              vemu_elf_section_header_t *sh,
                              size_t i);
  
uint8_t *vemu_load_section_content(FILE *file, vemu_elf_section_header_t *sh);

#endif
