#include <stdio.h>

#include "directive.h"
#include "instr.h"
#include "lexeme.h"
#include "op.h"
#include "symbol.h"
#include "token.h"

void print_raw(char *s)
{
    while (*s)
    {
        switch (*s)
        {
        case '\n':
            printf("\\n");
            break;
        case '\t':
            printf("\\t");
            break;
        default:
            printf("%c", *s);
        }

        s++;
    }
}

void instr_debug(instr_t *instr)
{
    printf("%s {\n", instr->type == OP ? "op" : "directive");

    if (instr->labelp > -1)
        printf("\tlabel: %s\n", symtable[instr->labelp].lexeme);

    printf("\tmnemonic: %s\n",
           instr->type == OP ? optable[instr->p].mnemonic : dirtable[instr->p]);

    if (instr->type == OP)
    {
        op_t *op = &optable[instr->p];

        switch (op->opcode)
        {
        case ADD:
        case AND:
            printf("\tDR: %d\n\tSR1: %d\n\t%s: %d\n", instr->arg1, instr->arg2,
                   instr->alt ? "imm5" : "SR2", instr->arg3);
            break;
        case BR:
            printf("\tPCoffset9: %s\n", symtable[instr->arg1].lexeme);
            break;
        case JMP:
            if (!op->attr)
                printf("\tbase: %d\n", instr->arg1);
            break;
        case JSR:
            if (op->attr)
                printf("\tBaseR: %d\n", instr->arg1);
            else
                printf("\tPCoffset11: %s", symtable[instr->arg1].lexeme);
            break;
        case LD:
        case LDI:
        case LEA:
            printf("\tDR: %d\n\tPCoffset9: %s\n", instr->arg1,
                   symtable[instr->arg2].lexeme);
            break;
        case LDR:
            printf("\tDR: %d\n\tBaseR: %d\n\toffset6: %d\n", instr->arg1,
                   instr->arg2, instr->arg3);
            break;
        case NOT:
            printf("\tDR: %d\n\tSR: %d\n", instr->arg1, instr->arg2);
            break;
        case ST:
        case STI:
            printf("\tSR: %d\n\tPCoffset9: %s\n", instr->arg1,
                   symtable[instr->arg2].lexeme);
            break;
        case STR:
            printf("\tSR: %d\n\tBaseR: %d\n\toffset6: %d\n", instr->arg1,
                   instr->arg2, instr->arg3);
            break;
        case TRAP:
            if (!op->attr)
                printf("\ttrapvect8: 0x%x\n", instr->arg1);
        case RTI:
        case RES:
            break;
        }
    }
    else
    {
        switch (instr->p)
        {
        case ORIG:
        case FILL:
        case BLKW:
            printf("\targ: 0x%x\n", instr->arg1);
            break;
        case STRINGZ:
            printf("\targ: \"");
            print_raw(&lextable[instr->arg1]);
            printf("\"\n");
            break;
        case END:
            break;
        }
    }

    printf("}\n\n");
}
