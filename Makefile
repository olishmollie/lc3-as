all: lc_3 assembler

assembler: assembler.c
	clang assembler.c -o assembler

lc_3: lc_3.c
	clang lc_3.c -o lc_3
