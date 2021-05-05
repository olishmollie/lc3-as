#include <stdlib.h>

#include "panic.h"
#include "symbol.h"
#include "token.h"

char *tokstr(int token, int tokenval)
{
    switch (token)
    {
    case NUMBER:
        return "NUMBER";
    case OP:
        return "OP";
    case DIRECTIVE:
        return "DIRECTIVE";
    case SYMBOL:
        return tokenval > 0 ? symtable[tokenval].lexptr : "ID";
    case REG:
        return "REG";
    case COMMA:
        return ",";
    case DONE:
        return "DONE";
    default:
        panic("lexeme: unknown token %d", token);
        return NULL;
    }
}
