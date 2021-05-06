#ifndef SYMBOL_H
#define SYMBOL_H

typedef struct sym_s
{
    char *lexeme;
    int offset;
    int defined;
} sym_t;

extern sym_t symtable[];

int lookup_sym(char *s);
int insert_sym(char *s, int offset);

#endif
