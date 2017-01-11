CC=g++
CXXFLAGS=-std=gnu++11 -g

.PHONY: all

all: lab1 basicblock

lab1: icode.o lab1.o

basicblock: icode.o basicblock.o basicblock_main.o

clean:
	-rm *.o
