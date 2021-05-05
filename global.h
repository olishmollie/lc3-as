#ifndef GLOBAL_H
#define GLOBAL_H

#include <stdint.h>
#include <stdio.h>

#define DATAFILE "p.data"
#define OUTFILE "o.lc3"

typedef uint16_t word;

#define INSTR_WIDTH sizeof(word)

/* Input file object */
extern FILE *infile;

#endif
