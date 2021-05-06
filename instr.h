#ifndef INSTR_H
#define INSTR_H

typedef struct instr_s
{
    int labelp;
    int type;
    int lc;
    int p;
    int alt;
    int arg1;
    int arg2;
    int arg3;
} instr_t;

void instr_debug(instr_t *instr);

#endif
