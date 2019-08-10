all: vm assembler

assembler: assembler.c symbol.c
	clang assembler.c symbol.c -o assembler

vm: vm.c
	clang vm.c -o vm
