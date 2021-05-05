#include <ctype.h>
#include <stdio.h>

#include "directive.h"
#include "global.h"
#include "lexeme.h"
#include "op.h"
#include "panic.h"
#include "symbol.h"
#include "token.h"

#define BSIZE 20

int lineno = 1;
int tokenval = 0;

char lexbuf[BSIZE];

/* Return 1 if c is a valid hexadecimal digit, 0 otherwise. */
int ishex(int c)
{
    return isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

/* Convert a hexadecimal char to decimal. */
int hex2dec(int c)
{
    if ('0' <= c && c <= '9')
    {
        return c - '0';
    }
    switch (toupper(c))
    {
    case 'A':
        return 10;
    case 'B':
        return 11;
    case 'C':
        return 12;
    case 'D':
        return 13;
    case 'E':
        return 14;
    case 'F':
        return 15;
    }

    panic("unexpected token '%c', lineno %d", c, lineno);

    return -1;
}

/* Return escape characters. */
int escaped(char c)
{
    if (c == '\\')
    {
        c = fgetc(infile);
        switch (c)
        {
        case 'n':
            return '\n';
        case 'r':
            return '\r';
        case 't':
            return '\t';
        case 'v':
            return '\v';
        default:
            return c;
        }
    }
    return c;
}

/* Return the next token in the stream. */
int lexan(void)
{
    int c;
    while (1)
    {
        c = fgetc(infile);

        if (isspace(c))
        {
            if (c == '\n')
                ++lineno;
            continue;
        }
        else if (c == ';')
        {
            while (c != EOF && c != '\n')
                c = fgetc(infile);
            if (c != EOF)
                ungetc(c, infile);
            continue;
        }
        else if (c == '.')
        {
            int p, b = 0;

            c = fgetc(infile);

            if (!isalpha(c))
                panic("invalid directive, line %d", lineno);

            while (isalpha(c))
            {
                lexbuf[b++] = c;
                if (b >= BSIZE)
                    panic("directive too long, line %d", lineno);
                c = fgetc(infile);
            }
            lexbuf[b] = '\0';

            if (c != EOF)
                ungetc(c, infile);

            p = lookup_directive(lexbuf);
            if (p == -1)
                panic("invalid directive '%s', line %d", lexbuf, lineno);

            tokenval = p;
            return DIRECTIVE;
        }
        else if (c == '#')
        {
            int neg = 0;
            int n = 0;

            c = fgetc(infile);

            if (c != '-' && !isdigit(c))
                panic("unexpected token '%c', line %d", lineno);

            if (c == '-')
            {
                neg = 1;
                c = fgetc(infile);
                if (!isdigit(c))
                    panic("unexpected token '-', line %d", lineno);
            }

            while (isdigit(c))
            {
                tokenval = (word)(n * 10 + c - '0');
                if ((word)tokenval < n) /* overflow */
                    break;
                n = tokenval;
                c = fgetc(infile);
            }

            if (c != EOF)
                ungetc(c, infile);

            if (neg)
                tokenval = -tokenval;

            return NUMBER;
        }
        else if (c == 'x')
        {
            int n = 0;

            c = fgetc(infile);

            if (!ishex(c))
                panic("unexpected token '%c', line %d", lineno);

            while (ishex(c))
            {
                tokenval = (word)(n * 16 + hex2dec(c));
                if ((word)tokenval < n) /* overflow */
                    break;
                n = tokenval;
                c = fgetc(infile);
            }

            if (c != EOF)
                ungetc(c, infile);

            return NUMBER;
        }
        else if (c == 'R')
        {
            c = fgetc(infile);

            if (!isdigit(c) || c < '0' || c > '7')
                panic("invalid register R%c, line %d", c, lineno);

            tokenval = c - '0';
            return REG;
        }
        else if (isalpha(c))
        {
            int p, b = 0;

            while (isalnum(c))
            {
                lexbuf[b++] = c;
                if (b >= BSIZE)
                    panic("label too long, line %d", lineno);
                c = fgetc(infile);
            }
            lexbuf[b] = '\0';

            if (c != EOF)
                ungetc(c, infile);

            /* Handle ops */
            p = lookup_op(lexbuf);
            if (p > -1)
            {
                tokenval = p;
                return OP;
            }

            /* Handle symbols */
            p = lookup_sym(lexbuf);
            if (p == -1)
                p = insert_sym(lexbuf, -1);

            tokenval = p;
            return SYMBOL;
        }
        else if (c == '"')
        {
            int p, b = 0;
            c = fgetc(infile);
            while (c != EOF && c != '"')
            {
                lexbuf[b++] = escaped(c);
                c = fgetc(infile);
            }
            lexbuf[b] = '\0';

            if (c != '"')
                panic("unclosed quote, line %d", lineno);

            p = insert_lexeme(lexbuf);

            tokenval = p;
            return STRING;
        }
        else if (c == ',')
        {
            tokenval = NONE;
            return COMMA;
        }
        else if (c == EOF)
        {
            return DONE;
        }

        panic("unexpected token '%c', line %d", c, lineno);
    }
}
