#ifndef SYMBOL_H
#define SYMBOL_H

typedef struct sym_s
{
    char *lexptr;
    int offset;
    int defined;
} sym_t;

extern sym_t symtable[];

int lookup_sym(char *s);
int insert_sym(char *s, int offset);

#endif
