#ifndef TOKEN_H
#define TOKEN_H

#define NUMBER 256
#define SYMBOL 257
#define DIRECTIVE 258
#define OP 259
#define REG 261
#define COMMA 262
#define STRING 263
#define DONE 264

#define NONE -1

char *tokstr(int token, int tokenval);

#endif
