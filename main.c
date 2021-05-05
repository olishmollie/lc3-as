#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "emit.h"
#include "global.h"
#include "panic.h"
#include "parse.h"

FILE *infile;

void cleanup(void);

int main(int argc, char **argv)
{
    atexit(cleanup);

    if (argc == 1)
    {
        fprintf(stderr, "Usage: as <file>\n");
        exit(1);
    }

    infile = fopen(argv[1], "r");
    if (!infile)
        panic("error: could not read file '%s'", argv[1]);

    parse();

    fclose(infile);

    emit();

    return 0;
}

void cleanup(void) { remove(DATAFILE); }
