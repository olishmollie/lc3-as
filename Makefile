all: vm assembler

assembler: assembler.c operation.c symbol.c
	clang assembler.c operation.c symbol.c -o assembler

vm: vm.c
	clang vm.c -o vm
