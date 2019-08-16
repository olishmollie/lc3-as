#include "op.h"
#include "common.h"

#define NUM_OPS (uint16_t)sizeof(operations) / (uint16_t)sizeof(operations[0])

Operation operations[] = {
    /* Operations */
    {"ADD", ADD, 3, 0, 0},
    {"AND", AND, 3, 0, 0},
    {"BRn", BR, 1, 0x4, 0},
    {"BRp", BR, 1, 0x1, 0},
    {"BRZ", BR, 1, 0x2, 0},
    {"BR", BR, 1, 0x7, 0},
    {"BRZP", BR, 1, 0x3, 0},
    {"BRNP", BR, 1, 0x5, 0},
    {"BRNZ", BR, 1, 0x6, 0},
    {"BRNZP", BR, 1, 0x7, 0},
    {"JMP", JMP, 1, 0, 0},
    {"JSR", JSR, 1, 0, 0},
    {"JSRR", JSR, 1, 0, 0},
    {"LD", LD, 2, 0, 0},
    {"LDI", LDI, 2, 0, 0},
    {"LDR", LDR, 3, 0, 0},
    {"LEA", LEA, 2, 0, 0},
    {"NOT", NOT, 2, 0, 0},
    {"RET", JMP, 0, 0, 1},
    {"RTI", RTI, 0, 0, 0},
    {"ST", ST, 2, 0, 0},
    {"STI", STI, 2, 0, 0},
    {"STR", STR, 3, 0, 0},
    {"TRAP", TRAP, 1, 0, 0},

    /* Trap Routines */
    {"GETC", GETC, 0, 0, 0},
    {"OUT", OUT, 0, 0, 0},
    {"PUTS", PUTS, 0, 0, 0},
    {"IN", IN, 0, 0, 0},
    {"PUTSP", PUTSP, 0, 0, 0},
    {"HALT", HALT, 0, 0, 0},

    /* Directives */
    {".ORIG", ORIG, 1, 0, 0},
    {".FILL", FILL, 1, 0, 0},
    {".BLKW", BLKW, 1, 0, 0},
    {".STRINGZ", STRINGZ, 1, 0, 0},
    {".END", 0xfffb, 0, 0, 0}
};

uint16_t findOperation(char *name) {
    uint16_t i;
    for (i = 0; i < NUM_OPS; i++) {
        if (strcmp(name, operations[i].name) == 0) {
            return i;
        }
    }

    return 0xffff;
}
