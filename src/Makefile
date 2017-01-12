CC=g++
CXXFLAGS=-std=gnu++11 -g

.PHONY: all

all: dom debug main

dom: icode.o ssa.o dom.o

debug: icode.o ssa.o debug.o

main: icode.o main.o

clean:
	-rm *.o
