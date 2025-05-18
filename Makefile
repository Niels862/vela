.PHONY: all vemu libc tests rebuild clean

all: vemu libc tests

vemu:
	$(MAKE) -C vemu

libc:
	$(MAKE) -C libc

tests:
	$(MAKE) -C tests

rebuild:
	$(MAKE) -B -C vemu
	$(MAKE) -B -C libc
	$(MAKE) -B -C tests

clean:
	$(MAKE) -C vemu clean
	$(MAKE) -C libc clean
	$(MAKE) -C tests clean
