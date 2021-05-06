CCFLAGS := -std=c99 -g -Wall -Werror -Wpedantic
OBJ := directive.o emit.o instr.o lex.o lexeme.o op.o panic.o parse.o symbol.o token.o
AS := lcas
VM := lc3

all: $(AS) $(VM)

$(AS): main.c $(OBJ)
	$(CC) $(CCFLAGS) -o $@ $^

$(VM): core.c
	$(CC) $(CCFLAGS) -o $@ $<

%.o: %.c %.h
	$(CC) $(CCFLAGS) $< -c -o $@

.PHONY: all clean
clean:
	rm -rf $(VM) $(VM).dSYM $(AS) $(AS).dSYM *.o *.lc3 *.data
