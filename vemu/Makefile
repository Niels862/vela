TARGET = vemu
CC = gcc
INC_DIR = inc ../common/inc
SRC_DIR = src

CFLAGS = -Wall -Wextra -Wpedantic -Werror -Wfatal-errors -std=c99 -O3 -g

INCFLAGS = $(addprefix -I, $(INC_DIR))
SOURCES = $(sort $(shell find $(SRC_DIR) -name '*.c'))
OBJECTS = $(SOURCES:.c=.o)
DEPS = $(OBJECTS:.o=.d)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(INCFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(INCFLAGS) -MMD -o $@ -c $<

clean:
	rm -f $(OBJECTS) $(DEPS) $(TARGET)

-include $(DEPS)
