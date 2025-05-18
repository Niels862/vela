TARGET = libstd.a
CC = riscv32-unknown-elf-gcc
INC_DIR = inc ../common/inc

CFLAGS = -ffreestanding -nostdinc -nostdlib -nostartfiles -I../common/inc -Wall -Wextra -Wpedantic -O3 -march=rv32imafd -mabi=ilp32f

INCFLAGS = $(addprefix -I, $(INC_DIR))
SOURCES = $(sort $(shell find $(SRC_DIR) -name '*.c'))
OBJECTS = $(SOURCES:.c=.o)
DEPS = $(OBJECTS:.o=.d)

.PHONY: all clean

all: $(OBJECTS)
	ar -rcs $(TARGET) $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCFLAGS) -c -o $@ $^

clean:
	rm -f $(TARGET) $(OBJECTS) 
