#include <string.h>

#include "lexeme.h"
#include "panic.h"

/* Maximum chars in lexeme table */
#define CHARMAX 999

char lextable[CHARMAX];

int lastchar = 0;

int insert_lexeme(char *s)
{
    int len = strlen(s);

    if (lastchar + len + 1 >= CHARMAX)
        panic("lexeme table overflow");

    strcpy(&lextable[lastchar], s);

    lastchar += len + 1;

    return lastchar - len - 1;
}
