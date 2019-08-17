#include "asm.h"
#include "op.h"
#include "parse.h"
#include "sym.h"

#define DEFAULT_OUTFILE "a.out"

int main(int argc, char **argv) {
    initTable(&symbolTable);

    if (argc != 2) {
        printf("usage: blah blah blah\n");
        exit(1);
    }

    FILE *istream = fopen(argv[1], "r");
    if (!istream) {
	fprintf(stderr, "unable to open '%s'\n", argv[1]);
	exit(1);
    }

    Program *prog = parse(istream);

    FILE *ostream = fopen(DEFAULT_OUTFILE, "wb");
    if (!ostream) {
	fprintf(stderr, "unable to open '%s'\n", DEFAULT_OUTFILE);
	exit(1);
    }

    assemble(prog, ostream);

    fclose(ostream);

    return 0;
}
