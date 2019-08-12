#ifndef OPERATION_H
#define OPERATION_H

#include "common.h"

/* opcodes */
enum {
    BR = 0x0,
    ADD,
    LD,
    ST,
    JSR,
    AND,
    LDR,
    STR,
    RTI,
    NOT,
    LDI,
    STI,
    JMP,
    RES, /* reserved */
    LEA,
    TRAP
};

typedef struct s_operation {
    char *name;
    uint16_t opcode;
    uint16_t nargs;
    uint16_t nzp; /* used by BR */
} Operation;

extern Operation operations[];

/*
 * FindOperation attempts to find an operation by name.
 * If found, it returns the index into the operations array.
 * Otherwise, it returns -1.
 */
uint16_t findOperation(char *name);


#endif
