CC=g++
CXXFLAGS=-std=gnu++11 -g

.PHONY: all

all: ccode cfg dom constpropa

ccode: icode.o ccode.o

cfg: icode.o cfg.o

dom: icode.o dom.o

constpropa: icode.o constpropa.o

clean:
	-rm *.o
