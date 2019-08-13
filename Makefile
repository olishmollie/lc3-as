all: core assembler

assembler: src/lca.c src/asm.c src/op.c src/parse.c src/sym.c
	clang src/lca.c src/asm.c src/op.c src/parse.c src/sym.c -o lca

core: src/core.c
	clang src/core.c -o lc3
