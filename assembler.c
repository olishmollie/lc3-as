#include "operation.h"
#include "symbol.h"

#define MAX_NUM_SYMBOLS 256
#define MAX_STRING_LEN 1024
#define NUM_INSTRUCTIONS 35
#define IRFILE "intermediate.tmp"
#define OUTFILE "lc3.out"

FILE *infile, *irfile, *outfile;
int lc = 0, ln = 1;

void fatal(char *msg) {
    fprintf(stderr, "fatal: %s - lc = %d\n", msg, lc);
    remove(IRFILE);
    remove(OUTFILE);
    fclose(irfile);
    fclose(infile);
    exit(1);
}

int isOp(int opidx) {
    return opidx >= 0 && opidx <= 23;
}

int isTrap(int opidx) {
    return opidx >= 24 && opidx <= 29;
}

int isDirective(int opidx) {
    return opidx >= 30 && opidx <= 34;
}

void skipSpaces(char *line, int *pos) {
    char c = line[*pos];
    while (isspace(c) && c != '\n') {
        c = line[++(*pos)];
    }
}

/*
 *  ParseOperation parses one alphanumeric word from line.
 *  If the word is an Operation, it returns its operations index.
 *  Otherwise, it assumes it's a label, adds it
 *  to the symbol table, and returns -1.
 */
int parseOperation(char *line, int *pos) {
    char buf[MAX_SYMBOL_LENGTH], *bufp, c;
    int opidx;

    skipSpaces(line, pos);

    bufp = buf;
    while ((c = line[*pos]) && !isspace(c)) {
        *bufp++ = c;
        (*pos)++;
    }
    *bufp = '\0';

    skipSpaces(line, pos);

    opidx = findOperation(buf);
    if (opidx != -1) {
        return opidx;
    }

    if (bufp - buf != 0) {
        newSymbol(buf, lc);
    }

    return -1;
}

void parseString(char *line, int *pos) {
    fprintf(irfile, "%c", line[(*pos)++]);

    int count = 0;
    char c;
    while ((c = line[*pos]) && c != '"') {
        if (++count == MAX_STRING_LEN) {
            fatal("max string length reached");
        }
        lc++;
        fprintf(irfile, "%c", c);
        (*pos)++;
    }

    lc++; /* add one more space for zero byte */

    fprintf(irfile, "%c ", line[(*pos)++]);
}

void parseLabel(char *line, int *pos) {
    char buf[MAX_SYMBOL_LENGTH], *bufp, c;

    bufp = buf;
    while ((c = line[*pos]) && !isspace(c) && c != ',' && c != '\n') {
        *bufp++ = c;
        (*pos)++;
        if (bufp - buf >= MAX_SYMBOL_LENGTH - 1) {
            fatal("maximum symbol length reached");
        }
    }
    *bufp = '\0';

    fprintf(irfile, "%s ", buf);

    newSymbol(buf, -1);
}

void parseLiteral(char *line, int *pos) {
    char c;

    while ((c = line[*pos]) && !isspace(c) && c != ',' && c != '\n') {
        fprintf(irfile, "%c", c);
        (*pos)++;
    }

    fprintf(irfile, " ");
}

void parseArg(char *line, int *pos) {
    skipSpaces(line, pos);

    switch (line[*pos]) {
    case '"':
        parseString(line, pos);
        break;
    case 'R':
    case '#':
    case 'x':
        parseLiteral(line, pos);
        break;
    default:
        parseLabel(line, pos);
    }
}

/*
 * ParseArgs attempts to parse op->nargs number of arguments from
 * the line at position pos. Once parsed, each argument is written to
 * the intermediate file, denoted by irfile.
 */
void parseArgs(char *line, int *pos, Operation *op) {
    int i;

    /* if (op->nargs == 0) { */
    /*     lc++; */
    /* } */

    for (i = 0; i < op->nargs; i++) {
        parseArg(line, pos);
        skipSpaces(line, pos);

        if (i != op->nargs - 1) {
            if (line[*pos] != ',') {
                printf("line[*pos] = '%c'\n", line[*pos]);
                fatal("expected ',' after argument");
            }
            (*pos)++;
        }
    }

    skipSpaces(line, pos);
}

/*
 * ParseLine parses an instruction from line, tracks its position
 * with pos, and writes information to an intermediate file pointed
 * to by irfile. If line does not end in a newline character, a fatal
 * error occurs.
 */
void parseLine(char *line, int pos) {
    skipSpaces(line, &pos);

    /* skip comments */
    if (line[pos] == ';') {
        ln++;
        return;
    }

    int opidx;
    Operation *op;

    opidx = parseOperation(line, &pos);

    if (opidx == -1) {
        opidx = parseOperation(line, &pos);
        if (opidx == -1) {
            fatal("invalid syntax");
        }
    }
    op = &operations[opidx];

    fprintf(irfile, "%d ", lc);

    if (isDirective(opidx)) {
        fprintf(irfile, "D ");
    } else {
        fprintf(irfile, "%c ", isTrap(opidx) ? 'T' : 'O');
    }

    fprintf(irfile, "%d ", opidx);

    parseArgs(line, &pos, op);

    /* skip inline comments */
    if (line[pos] == ';') {
        while (line[pos] != '\n') {
            pos++;
        }
    }

    if (line[pos] != '\n') {
        fatal("expected newline");
    }
    fprintf(irfile, "%c", line[pos++]);

    if (opidx != 30) {
        lc++;
    }
    ln++;
}

void parse() {
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    while ((read = getline(&line, &len, infile)) != -1) {
        if (read == 1) {
            continue; /* skip blank lines */
        }
        parseLine(line, 0);
    }

    if (line) {
        free(line);
    }
}

void assembleOp(uint16_t *instr) {
    Operation op;
    int opidx, nargs;

    fscanf(irfile, "%d ", &opidx);
    op = operations[opidx];

    *instr |= (op.opcode << 12);

    switch (op.opcode) {
    case ADD: {
        uint16_t dr, sr1, sr2, imm5;
        nargs = fscanf(irfile, "R%hu R%hu ", &dr, &sr1);
        if (nargs != 2) {
            fatal("argument error in ADD");
        }
        nargs = fscanf(irfile, "R%hu", &sr2);
        if (nargs) {
            *instr |= (dr << 9) | (sr1 << 6) | sr2;
        } else {
            nargs = fscanf(irfile, "#%hu", &imm5);
            if (!nargs) {
                fatal("argument error in ADD");
            }
            *instr |= (dr << 9) | (sr1 << 6) | (0x1 << 5) | (imm5 & 0x1f);
        }
        break;
    }
    case AND: {
        uint16_t dr, sr1, sr2, imm5;
        nargs = fscanf(irfile, "R%hu R%hu ", &dr, &sr1);
        if (nargs != 2) {
            fatal("argument error in AND");
        }
        nargs = fscanf(irfile, "R%hu", &sr2);
        if (nargs) {
            *instr |= (dr << 9) | (sr1 << 6) | sr2;
        } else {
            nargs = fscanf(irfile, "#%hu", &imm5);
            if (!nargs) {
                fatal("argument error in AND");
            }
            *instr |= (dr << 9) | (sr1 << 6) | (0x1 << 5) | (imm5 & 0x1f);
        }
        break;
    }
    case BR: {
        char str[MAX_SYMBOL_LENGTH];
        nargs = fscanf(irfile, "%s", str);
        if (nargs != 1) {
            fatal("argument error in BR");
        }
        Symbol *sym = getSymbol(&symbolTable, str);
        if (!sym) {
            fatal("unbound symbol in BR");
        }
        uint16_t offset = sym->value - (lc + 1);
        *instr |= (op.nzp << 9) | (offset & 0x1ff);
        break;
    }
    case LD: {
        uint16_t dr;
        char str[MAX_SYMBOL_LENGTH];
        nargs = fscanf(irfile, "R%hu %s", &dr, str);
        if (nargs != 2) {
            fatal("argument error in LD");
        }
        Symbol *sym = getSymbol(&symbolTable, str);
        if (!sym) {
            fatal("unbound symbol in LD");
        }
        uint16_t offset = sym->value - (lc + 1);
        *instr |= (dr << 9) | (offset & 0x1f);
        break;
    }
    case LEA: {
        uint16_t dr;
        char str[MAX_SYMBOL_LENGTH];
        nargs = fscanf(irfile, "R%hu %s", &dr, str);
        if (nargs != 2) {
            fatal("argument error in LEA");
        }
        Symbol *sym = getSymbol(&symbolTable, str);
        if (!sym) {
            fatal("unbound symbol in LEA");
        }
        uint16_t offset = sym->value - (lc + 1);
        *instr |= (dr << 9) | (offset & 0x1f);
        break;
    }
    default:
        fatal("instruction isn't implemented");
    }

    printf("writing \\x%x to file...\n", *instr);
    fwrite(instr, sizeof(uint16_t), 1, outfile);

    while (getc(irfile) != '\n') {
    }
}

void assembleTrap(uint16_t *instr) {
    int opidx;
    fscanf(irfile, "%d", &opidx);
    *instr |= operations[opidx].opcode;
    printf("writing TRAP \\x%x to file...\n", operations[opidx].opcode & 0xfff);
    fwrite(instr, sizeof(uint16_t), 1, outfile);
    while (getc(irfile) != '\n') {
    }
}

void assembleDirective(void) {
    int opidx, nargs;

    fscanf(irfile, "%d", &opidx);

    switch (opidx) {
    case 30: { /* .ORIG */
        unsigned int orig;
        nargs = fscanf(irfile, " x%x", &orig);
        if (nargs != 1) {
            fatal("argument error in .ORIG");
        }
        printf("writing .ORIG \\x%x to file...\n", (uint16_t)orig);
        fwrite((uint16_t *)&orig, sizeof(uint16_t), 1, outfile);
        break;
    }
    case 31: { /* .FILL */
        unsigned int val;
        nargs = fscanf(irfile, " x%x", &val);
        if (nargs != 1) {
            fatal("argument error in .FILL");
        }
        printf("writing FILL \\x%x to file...\n", (uint16_t)val);
        fwrite((uint16_t *)&val, sizeof(uint16_t), 1, outfile);
        break;
    }
    case 32: { /* .BLKW */
        unsigned int n, i;
        nargs = fscanf(irfile, " #%d", &n);
        if (nargs != 1) {
            fatal("argument error in .BLKW");
        }
        printf("writing BLKW %d to file...\n", n);
        for (i = 0; i < n; i++) {
            uint16_t zero = 0;
            fwrite(&zero, sizeof(uint16_t), 1, outfile);
        }
        break;
    }
    case 33: { /* .STRINGZ */
        char buf[MAX_STRING_LEN], *bufp;
        nargs = fscanf(irfile, " \"%1024[^\"]\"", buf);
        if (nargs != 1) {
            fatal("argument error in .STRINGZ");
        }
        printf("writing STRINGZ \"%s\" to file...\n", buf);

        /* write each char as uint16_t */
        bufp = buf;
        while (*bufp) {
            uint16_t c = (uint16_t)*bufp++;
            fwrite(&c, sizeof(uint16_t), 1, outfile);
        }
        fwrite(bufp, sizeof(uint16_t), 1, outfile);
        break;
    }
    case 34: /* .END */
        break;
    }

    while (getc(irfile) != '\n') {
    }
}

/*
 * AssembleInstr reads one line from the intermediate file.
 */
int assembleInstr(uint16_t *instr) {
    char c;

    c = getc(irfile);
    switch (c) {
    case 'O':
        assembleOp(instr);
        break;
    case 'T':
        assembleTrap(instr);
        break;
    case 'D':
        assembleDirective();
        break;
    case EOF:
        return 0;
    default:
        printf("c = '%c'\n", c);
        fatal("unknown instruction type");
    }

    return 1;
}

/*
 * Assemble reads the intermediate file, writing instructions to outfile
 * until EOF.
 */
void assemble() {
    fseek(irfile, 0, SEEK_SET);

    outfile = fopen(OUTFILE, "wb");
    if (!outfile) {
        fatal("could not open outfile");
    }

    int running = 1;
    while (running) {
        fscanf(irfile, "%d ", &lc);
        uint16_t instr = 0;
        running = assembleInstr(&instr);
    }

    fclose(outfile);
}

void cleanup() {
    fclose(irfile);
    fclose(infile);
    remove(IRFILE);
    deleteTable(&symbolTable);
}

int main(int argc, char **argv) {
    initTable(&symbolTable);

    if (argc != 2) {
        printf("usage: blah blah blah\n");
        exit(1);
    }

    infile = fopen(argv[1], "r");
    if (!infile) {
        fatal("could not open source file");
    }
    irfile = fopen(IRFILE, "w+");
    if (!irfile) {
        fatal("could not open new intermediate file");
    }

    parse();

    assemble();

    cleanup();

    return 0;
}
