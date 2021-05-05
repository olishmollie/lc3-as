#include <string.h>

#include "lexeme.h"
#include "panic.h"
#include "symbol.h"

#define SYMMAX 128

sym_t symtable[SYMMAX];

int lastsym = -1;

int lookup_sym(char *s)
{
    int p;
    for (p = lastsym; p > -1; --p)
        if (strcmp(symtable[p].lexptr, s) == 0)
            return p;
    return -1;
}

int insert_sym(char *s, int offset)
{
    int p;

    if (lastsym + 1 >= SYMMAX)
        panic("symbol table overflow");

    ++lastsym;
    symtable[lastsym].offset = offset;

    p = insert_lexeme(s);
    symtable[lastsym].lexptr = &lextable[p];

    return lastsym;
}
