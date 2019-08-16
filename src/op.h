#ifndef OPERATION_H
#define OPERATION_H

#include "common.h"

enum {
    /* Operations  */
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
    TRAP,

    /* Trap Routines */
    GETC = 0xf020,
    OUT = 0xf021,
    PUTS = 0xf022,
    IN = 0xf023,
    PUTSP = 0xf024,
    HALT = 0xf025,
    
    /* Directives */
    ORIG = 0xffff,
    FILL = 0xfffe,
    BLKW = 0xfffd,
    STRINGZ = 0xfffc,
    END = 0xfffb
};

typedef struct Operation {
    char *name;
    uint16_t opcode;
    uint16_t nargs;
    uint16_t nzp; /* used by BR */
    uint16_t ret; /* used by JMP */
} Operation;

extern Operation operations[];

/*
 * FindOperation attempts to find an operation by name.
 * If found, it returns the index into the operations array.
 * Otherwise, it returns -1.
 */
uint16_t findOperation(char *name);


#endif
