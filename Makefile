all: vm assembler parse

assembler: assembler.c operation.c symbol.c
	clang assembler.c operation.c symbol.c -o assembler

vm: vm.c
	clang vm.c -o vm

parse: parse.c operation.c symbol.c 
	clang parse.c operation.c symbol.c -o parse
