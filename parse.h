#ifndef PARSE_H
#define PARSE_H

/*
 * Records are written to an intermediate file in
 * the first pass and are the input to the second.
 */
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

/* Parser lookahead token */
extern int lookahead;

/* Stores value of lookahead */
extern int tokenval;

void parse(void);

#endif
