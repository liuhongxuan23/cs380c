CC=g++
CXXFLAGS=-std=gnu++11 -g

.PHONY: all

all: ccode dom debug

ccode: icode.o ssa.o ccode.o

dom: icode.o ssa.o dom.o

debug: icode.o ssa.o debug.o

clean:
	-rm *.o
