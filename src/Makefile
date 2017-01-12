CC=g++
CXXFLAGS=-std=gnu++11 -g

.PHONY: all

all: ccode cfg dom constpropa debug

ccode: icode.o ssa.o ccode.o

dom: icode.o ssa.o dom.o

cfg: icode.o cfg.o

debug: icode.o ssa.o debug.o

constpropa: icode.o constpropa.o

clean:
	-rm *.o
