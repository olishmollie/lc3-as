#ifndef OPERATION_H
#define OPERATION_H

#define BR 0
#define ADD 1
#define LD 2
#define ST 3
#define JSR 4
#define AND 5
#define LDR 6
#define STR 7
#define RTI 8
#define NOT 9
#define LDI 10
#define STI 11
#define JMP 12
#define RES 13
#define LEA 14
#define TRAP 15

#define GETC 0x20
#define OUT 0x21
#define PUTS 0x22
#define IN 0x23
#define PUTSP 0x24
#define HALT 0x25

typedef struct op_s
{
    char *mnemonic;
    int opcode;
    int attr;
} op_t;

int lookup_op(char *str);

extern op_t optable[];

#endif
