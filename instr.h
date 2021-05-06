#ifndef INSTR_H
#define INSTR_H

typedef struct instr_s
{
    int labelp;
    int type;
    int lc;
    int p;
    int arg1;
    int arg2;
    int arg3;
    int alt;
} instr_t;

void instr_debug(instr_t *instr);

#endif
