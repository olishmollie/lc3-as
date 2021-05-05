#include <fcntl.h>
#include <stdarg.h>
#include <unistd.h>

#include "directive.h"
#include "global.h"
#include "lex.h"
#include "op.h"
#include "panic.h"
#include "parse.h"
#include "string.h"
#include "symbol.h"
#include "token.h"

/* Location counter */
int lc;

/* Output file descriptor */
int ofd;

/* Listing file descriptor */
int lfd;

int lookahead = NONE;

/* Advance the emitter */
int advance() { return lookahead = lexan(); }

/* Match token and advance the parser */
void match(int token)
{
    if (lookahead == token)
        advance();
    else
        panic("unexpected token '%s', expected '%s', line %d",
              tokstr(lookahead, tokenval), tokstr(token, NONE), lineno);
}

void writeto(int fd, void *buf, size_t size)
{
    if (write(fd, buf, size) == -1)
        panic("writeto: unable to write to file");
}

void label(instr_t *instr)
{
    instr->labelp = tokenval;
    match(SYMBOL);
    if (symtable[instr->labelp].offset != -1)
        panic("multiply defined label '%s'\n", symtable[instr->labelp].lexptr);
    symtable[instr->labelp].offset = lc;
}

void opadd(instr_t *instr)
{
    instr->arg1 = tokenval;
    match(REG);
    match(COMMA);
    instr->arg2 = tokenval;
    match(REG);
    match(COMMA);

    if (lookahead == REG)
    {
        instr->arg3 = tokenval;
        instr->alt = 0;
        match(REG);
    }
    else
    {
        instr->arg3 = tokenval;
        instr->alt = 1;
        match(NUMBER);
    }
}

void opand(instr_t *instr)
{
    instr->arg1 = tokenval;
    match(REG);
    match(COMMA);
    instr->arg2 = tokenval;
    match(REG);
    match(COMMA);

    if (lookahead == REG)
    {
        instr->arg3 = tokenval;
        instr->alt = 0;
        match(REG);
    }
    else
    {
        instr->arg3 = tokenval;
        instr->alt = 1;
        match(NUMBER);
    }
}

void opbr(instr_t *instr)
{
    instr->arg1 = tokenval;
    match(SYMBOL);
}

void opjmp(instr_t *instr)
{
    op_t op = optable[instr->p];

    if (op.attr)
    {
        instr->alt = 1;
    }
    else
    {
        instr->arg1 = tokenval;
        instr->alt = 0;
        match(REG);
    }
}

void opjsr(instr_t *instr)
{
    op_t op = optable[instr->p];

    if (op.attr)
    {
        instr->arg1 = tokenval;
        match(REG);
    }
    else
    {
        instr->arg1 = tokenval;
        match(SYMBOL);
    }
}

void opld(instr_t *instr)
{
    instr->arg1 = tokenval;
    match(REG);
    match(COMMA);
    instr->arg2 = tokenval;
    match(SYMBOL);
}

void opldi(instr_t *instr)
{
    instr->arg1 = tokenval;
    match(REG);
    match(COMMA);
    instr->arg2 = tokenval;
    match(SYMBOL);
}

void opldr(instr_t *instr)
{
    instr->arg1 = tokenval;
    match(REG);
    match(COMMA);
    instr->arg2 = tokenval;
    match(REG);
    match(COMMA);
    instr->arg3 = tokenval;
    match(NUMBER);
}

void oplea(instr_t *instr)
{
    instr->arg1 = tokenval;
    match(REG);
    match(COMMA);
    instr->arg2 = tokenval;
    match(SYMBOL);
}

void opnot(instr_t *instr)
{
    instr->arg1 = tokenval;
    match(REG);
    match(COMMA);
    instr->arg2 = tokenval;
    match(REG);
}

void opst(instr_t *instr)
{
    instr->arg1 = tokenval;
    match(REG);
    match(COMMA);
    instr->arg2 = tokenval;
    match(SYMBOL);
}

void opsti(instr_t *instr)
{
    instr->arg1 = tokenval;
    match(REG);
    match(COMMA);
    instr->arg2 = tokenval;
    match(SYMBOL);
}

void opstr(instr_t *instr)
{
    instr->arg1 = tokenval;
    match(REG);
    match(COMMA);
    instr->arg2 = tokenval;
    match(REG);
    match(COMMA);
    instr->arg3 = tokenval;
    match(NUMBER);
}

void optrap(instr_t *instr)
{
    op_t op = optable[instr->p];
    if (!op.attr)
    {
        instr->arg1 = tokenval;
        match(NUMBER);
    }
}

void instruction(instr_t *instr)
{
    int p = tokenval;
    match(OP);

    instr->type = OP;
    instr->p = p;

    switch (optable[p].opcode)
    {
    case ADD:
        opadd(instr);
        break;
    case AND:
        opand(instr);
        break;
    case BR:
        opbr(instr);
        break;
    case JMP:
        opjmp(instr);
        break;
    case JSR:
        opjsr(instr);
        break;
    case LD:
        opld(instr);
        break;
    case LDI:
        opldi(instr);
        break;
    case LDR:
        opldr(instr);
        break;
    case LEA:
        oplea(instr);
        break;
    case NOT:
        opnot(instr);
        break;
    case ST:
        opst(instr);
        break;
    case STI:
        opsti(instr);
        break;
    case STR:
        opstr(instr);
        break;
    case TRAP:
        optrap(instr);
        break;
    }
}

void directive(instr_t *instr)
{
    int p = tokenval;
    match(DIRECTIVE);

    instr->type = DIRECTIVE;
    instr->p = p;

    switch (p)
    {
    case ORIG:
    case FILL:
    case BLKW:
        instr->arg1 = tokenval;
        match(NUMBER);
        break;
    case STRINGZ:
        instr->arg1 = tokenval;
        match(STRING);
        break;
    }
}

void line()
{
    instr_t instr;

    instr.labelp = -1;
    instr.lc = lc;

    if (lookahead == SYMBOL)
        label(&instr);

    if (lookahead == OP)
        instruction(&instr);
    else if (lookahead == DIRECTIVE)
        directive(&instr);
    else
        panic("unexpected token %s, line %d", tokstr(lookahead, tokenval),
              lineno);

    writeto(lfd, &instr, sizeof(instr));

    ++lc;
}

void program()
{
    while (!feof(infile))
        line();
}

void pinstr(instr_t *instr)
{
    printf("instr_t{\n");
    printf("\tTYPE: %s\n", instr->type == OP ? "OP" : "DIRECTIVE");

    if (instr->labelp > -1)
        printf("\tLABEL: %s\n", symtable[instr->labelp].lexptr);

    printf("\tMNEMONIC: %s\n",
           instr->type == OP ? optable[instr->p].mnemonic : dirtable[instr->p]);

    printf("\tARG1: %d\n", instr->arg1);
    printf("\tARG2: %d\n", instr->arg2);
    printf("\tARG3: %d\n", instr->arg3);

    printf("}\n");
}

void parse()
{
    lfd = open(DATAFILE, O_WRONLY | O_CREAT, 0744);

    if (lfd == -1)
        panic("parse: unable to open '%s'", DATAFILE);

    lookahead = lexan();

    program();

    close(lfd);
}
