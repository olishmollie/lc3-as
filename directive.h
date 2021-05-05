#ifndef DIRECTIVE_H
#define DIRECTIVE_H

#define ORIG 0
#define FILL 1
#define BLKW 2
#define STRINGZ 3

extern char *dirtable[];

int lookup_directive(char *lexeme);

#endif
