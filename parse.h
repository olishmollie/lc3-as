#ifndef PARSE_H
#define PARSE_H

/* Parser lookahead token */
extern int lookahead;

/* Stores value of lookahead */
extern int tokenval;

void parse(void);

#endif
