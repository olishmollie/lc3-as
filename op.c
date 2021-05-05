#include <string.h>

#include "op.h"

op_t optable[] = {
    {"ADD", ADD, 0},        {"AND", AND, 0},      {"BR", BR, 0},
    {"BRn", BR, 4},         {"BRnz", BR},         {"BRnzp", BR, 7},
    {"BRz", BR, 2},         {"BRzp", BR, 3},      {"BRp", BR, 1},
    {"JMP", JMP, 0},        {"JSR", JSR, 0},      {"JSRR", JSR, 1},
    {"LD", LD, 0},          {"LDI", LDI, 0},      {"LDR", LDR, 0},
    {"LEA", LEA, 0},        {"NOT", NOT, 0},      {"RET", JMP, 1},
    {"RTI", RTI, 0},        {"ST", ST, 0},        {"STI", STI, 0},
    {"STR", STR, 0},        {"TRAP", TRAP, 0},    {"GETC", TRAP, GETC},
    {"OUT", TRAP, OUT},     {"PUTS", TRAP, PUTS}, {"IN", TRAP, IN},
    {"PUTSP", TRAP, PUTSP}, {"HALT", TRAP, HALT}};

#define NOPS sizeof(optable) / sizeof(optable[0])

int lookup_op(char *str)
{
    unsigned i;
    for (i = 0; i < NOPS; ++i)
        if (strcmp(str, optable[i].mnemonic) == 0)
            return i;
    return -1;
}
