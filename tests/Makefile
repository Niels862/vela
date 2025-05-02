CC = riscv32-unknown-elf-gcc
CFLAGS = -Wall -O2 -nostdlib -nostartfiles -march=rv32imafdc -mabi=ilp32f
LDFLAGS =
LD_SCRIPT =

SRC := $(wildcard *.c)
OUT := $(SRC:.c=.elf)

# LD_SCRIPT = -T linker.ld
# LDFLAGS += $(LD_SCRIPT)

all: $(OUT)

%.elf: %.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *.elf
