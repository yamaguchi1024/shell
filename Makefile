.PHONY: all


shell: test2.cpp util.c util.h ringlist.c ringlist.h
	g++ -O2 -o shell test2.cpp util.c ringlist.c

all: shell
