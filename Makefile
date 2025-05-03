.PHONY: all vemu tests rebuild clean

all: vemu tests

vemu:
	$(MAKE) -C vemu

tests:
	$(MAKE) -C tests

rebuild:
	$(MAKE) -B -C vemu
	$(MAKE) -B -C tests

clean:
	$(MAKE) -C vemu clean
	$(MAKE) -C tests clean
