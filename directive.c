#include <string.h>

char *dirtable[] = {"ORIG", "FILL", "BLKW", "STRINGZ"};

int lookup_directive(char *lexeme)
{
    unsigned i;
    for (i = 0; i < sizeof(dirtable) / sizeof(dirtable[0]); ++i)
        if (strcmp(lexeme, dirtable[i]) == 0)
            return i;
    return -1;
}
