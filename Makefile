.PHONY: all


shell: main.cpp util.c util.h ringlist.c ringlist.h
	g++ -O2 -o shell main.cpp util.c ringlist.c

all: shell
