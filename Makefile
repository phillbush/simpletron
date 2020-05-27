PROG = simpletron/simpletron simple/simple

all: ${PROG}

simpletron/simpletron: simpletron/*.c simpletron/*.h
	cd simpletron && ${MAKE} all

simple/simple: simple/*.c simple/*.h
	cd simple && ${MAKE} all

include Makefile.test

clean: clean-test
	cd simpletron && ${MAKE} clean
	cd simple && ${MAKE} clean

.PHONY: all clean test
