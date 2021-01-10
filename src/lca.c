#include "asm.h"
#include "op.h"
#include "parse.h"
#include "sym.h"

#include <unistd.h>

#define DEFAULT_OUTFILE "o.lc3"

int main(int argc, char **argv) {
    Options opts;
    int c;

    /* opterr = 0; */
    opts.outfile = NULL;

    while ((c = getopt(argc, argv, "o:")) != -1) {
        switch (c) {
        case 'o':
            opts.outfile = optarg;
            break;
        case '?':
            if (optopt == 'o') {
                fprintf(stderr, "option -o requires an argument\n");
            } else if (isprint(optopt)) {
                fprintf(stderr, "unknown option %c", optopt);
            } else {
                fprintf(stderr, "unknown option \\x%x\n", optopt);
            }
            return 1;
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "no input file\n");
        return 1;
    }

    FILE *istream = fopen(argv[optind], "r");
    if (!istream) {
        fprintf(stderr, "unable to open '%s'\n", argv[1]);
        exit(1);
    }

    Program *prog = parse(istream);

    FILE *ostream = fopen(opts.outfile ? opts.outfile : DEFAULT_OUTFILE, "wb");
    if (!ostream) {
        fprintf(stderr, "unable to open '%s'\n", DEFAULT_OUTFILE);
        exit(1);
    }

    assemble(prog, ostream);

    fclose(ostream);

    return 0;
}
