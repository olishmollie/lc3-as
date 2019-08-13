#ifndef PARSE_H
#define PARSE_H

#include "common.h"
#include "op.h"
#include "sym.h"

#define MAX_NUM_INSTR 0xfdff
#define MAX_SYM_LEN 11

typedef struct Instr {
    uint16_t lc;
    Operation *op;

    Symbol *label;
    Symbol *pcoffset9;
    Symbol *pcoffset11;

    uint16_t dr;
    uint16_t sr1;
    uint16_t sr2;
    uint16_t imm5;
    uint16_t offset6;
    uint16_t baser;
    uint16_t trapvect8;

    uint16_t val;
} Instr;

typedef struct Program {
    uint16_t orig;
    uint16_t lc;
    Instr instructions[MAX_NUM_INSTR];

    Table symbolTable;
} Program;

Program *parse(FILE *istream);

#endif
