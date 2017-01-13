CC=g++
CXXFLAGS=-std=gnu++11 -g

.PHONY: all

all: debug main

debug: icode.o ssa.o debug.o

main: icode.o main.o ssa.o

clean:
	-rm *.o
