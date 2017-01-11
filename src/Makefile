CC=g++
CXXFLAGS=-std=gnu++11 -g

.PHONY: all

all: ccode cfg dom

ccode: icode.o ccode.o

cfg: icode.o cfg.o

dom: icode.o dom.o

clean:
	-rm *.o
