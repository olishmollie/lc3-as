#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "directive.h"
#include "emit.h"
#include "global.h"
#include "instr.h"
#include "lexeme.h"
#include "op.h"
#include "panic.h"
#include "parse.h"
#include "symbol.h"
#include "token.h"

/* Data file descriptor. */
int lfd = 0;

/* Output file descriptor */
int ofd = 0;

/* Flag raised on .END directive */
int done = 0;

void emit_op(instr_t *instr)
{
    op_t *op;
    sym_t *sym;
    word code = 0;

    op = &optable[instr->p];
    code = op->opcode << 12;

    switch (op->opcode)
    {
    case ADD:
        code |= (instr->arg1 << 9) | (instr->arg2 << 6);
        if (instr->alt)
            code |= instr->arg3;
        else
            code |= (1 << 5) | (instr->arg3 & 0x1f);
        write(ofd, &code, sizeof(code));
        break;
    case AND:
        code |= (instr->arg1 << 9) | (instr->arg2 << 6);
        if (instr->alt)
            code |= instr->arg3;
        else
            code |= (1 << 5) | (instr->arg3 & 0x1f);
        write(ofd, &code, sizeof(code));
        break;
    case BR:
        sym = &symtable[instr->arg1];
        if (sym->offset == -1)
            panic("undefined symbol '%s'", sym->lexeme);
        code |= op->attr << 9; /* nzp */
        code |= (sym->offset - instr->lc - 1) & 0x1ff;
        write(ofd, &code, sizeof(code));
        break;
    case JMP:
        if (op->attr)
            code |= 0x1c0;
        else
            code |= instr->arg1 << 6;
        write(ofd, &code, sizeof(code));
        break;
    case JSR:
        if (instr->alt)
            code |= instr->arg1 << 6;
        else
        {
            sym = &symtable[instr->arg1];
            if (sym->offset == -1)
                panic("undefined symbol '%s'", sym->lexeme);
            code |= (0x1 << 11) | ((sym->offset - instr->lc - 1) & 0x3ff);
        }
        write(ofd, &code, sizeof(code));
        break;
    case LD:
        code |= instr->arg1 << 9;
        sym = &symtable[instr->arg2];
        if (sym->offset == -1)
            panic("undefined symbol '%s'", sym->lexeme);
        code |= (sym->offset - instr->lc - 1) & 0x1ff;
        write(ofd, &code, sizeof(code));
        break;
    case LDI:
        code |= instr->arg1 << 9;
        sym = &symtable[instr->arg2];
        if (sym->offset == -1)
            panic("undefined symbol '%s'", sym->lexeme);
        code |= (sym->offset - instr->lc - 1) & 0x1ff;
        write(ofd, &code, sizeof(code));
        break;
    case LEA:
        code |= instr->arg1 << 9;
        sym = &symtable[instr->arg2];
        if (sym->offset == -1)
            panic("undefined symbol '%s'", sym->lexeme);
        code |= (sym->offset - instr->lc - 1) & 0x1ff;
        write(ofd, &code, sizeof(code));
        break;
    case LDR:
        code |= (instr->arg1 << 9) | (instr->arg2 << 6) | instr->arg3;
        write(ofd, &code, sizeof(code));
        break;
    case NOT:
        code |= (instr->arg1 << 9) | (instr->arg2 << 6) | 0x3f;
        write(ofd, &code, sizeof(code));
        break;
    case RTI:
        code |= 0x1c0;
        write(ofd, &code, sizeof(code));
        break;
    case ST:
        code |= instr->arg1 << 9;
        sym = &symtable[instr->arg2];
        if (sym->offset == -1)
            panic("undefined symbol '%s'", sym->lexeme);
        code |= (sym->offset - instr->lc - 1) & 0x1ff;
        write(ofd, &code, sizeof(code));
        break;
    case STI:
        code |= instr->arg1 << 9;
        sym = &symtable[instr->arg2];
        if (sym->offset == -1)
            panic("undefined symbol '%s'", sym->lexeme);
        code |= (sym->offset - instr->lc - 1) & 0x1ff;
        write(ofd, &code, sizeof(code));
        break;
    case STR:
        code |= (instr->arg1 << 9) | (instr->arg2 << 6) | instr->arg3;
        write(ofd, &code, sizeof(code));
        break;
    case TRAP:
        if (op->attr)
            code |= op->attr;
        else
            code |= instr->arg1;
        write(ofd, &code, sizeof(code));
        break;
    }
}

void emit_dir(instr_t *instr)
{
    word *n, c;
    char *s;

    switch (instr->p)
    {
    case ORIG:
    case FILL:
        write(ofd, &instr->arg1, INSTR_WIDTH);
        break;
    case BLKW:
        n = calloc(instr->arg1, INSTR_WIDTH);
        write(ofd, n, sizeof(*n));
        free(n);
        break;
    case STRINGZ:
        s = &lextable[instr->arg1];
        while ((c = *s++))
            write(ofd, &c, INSTR_WIDTH);
        write(ofd, &c, INSTR_WIDTH); /* null word */
        break;
    case END:
        done = 1;
        break;
    }
}

void emit()
{
    instr_t instr;

    lfd = open(DATAFILE, O_RDONLY);
    if (lfd == -1)
        panic("emit: unable to open data file");

    ofd = open(OUTFILE, O_WRONLY | O_CREAT | O_TRUNC, 0744);
    if (ofd == -1)
        panic("emit: unable to open output file");

    while (!done && read(lfd, &instr, sizeof(instr)))
    {
        if (instr.type == OP)
            emit_op(&instr);
        else if (instr.type == DIRECTIVE)
            emit_dir(&instr);
        else
            panic("emit: unknown instruction type %d", instr.type);
    }

    close(ofd);
    close(lfd);
}
