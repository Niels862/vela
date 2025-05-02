.PHONY: all vemu tests clean

all: vemu tests

vemu:
	$(MAKE) -C vemu

tests:
	$(MAKE) -C tests

clean:
	$(MAKE) -C vemu clean
