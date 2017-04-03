.PHONY: all


shell: main.cpp util.c util.h ringlist.c ringlist.h CpuMemstats.h CpuMemstats.c
	g++ -O2 -o shell main.cpp util.c ringlist.c CpuMemstats.c

all: shell
