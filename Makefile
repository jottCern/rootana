MAKEFLAGS = --no-print-directory -r -s

DIRS := base ra dc dra

-include Makefile.local

all:
	@for d in $(DIRS); do echo Building directory $$d; ( $(MAKE) -C $$d ) || break; done


clean:
	@for d in $(DIRS); do ( $(MAKE) -C $$d clean-subdir ) || break; done
