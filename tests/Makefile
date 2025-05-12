CC = riscv32-unknown-elf-gcc
INC_DIR = ../common/inc

CFLAGS = -ffreestanding -nostdinc -nostdlib -nostartfiles -isystem ../libc/inc -I../common/inc -Wall -Wextra -Wpedantic -O0 -march=rv32imafd -mabi=ilp32f
LDFLAGS =
LD_SCRIPT =

INCFLAGS = $(addprefix -I, $(INC_DIR))
SOURCES = $(sort $(shell find $(SRC_DIR) -name '*.c'))
OBJECTS = $(SOURCES:.c=.elf)
DEPS = $(OBJECTS:.elf=.d)

SRC := $(wildcard *.c)
OUT := $(SRC:.c=.elf)

# LD_SCRIPT = -T linker.ld
# LDFLAGS += $(LD_SCRIPT)

.PHONY: all clean

all: $(OUT)

%.elf: %.c
	$(CC) $(CFLAGS) $(INCFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *.elf
