#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NUM_SYMBOLS 256
#define MAX_SYMBOL_LEN 11
#define MAX_STRING_LEN 1024
#define NUM_INSTRUCTIONS 35
#define IRFILE "intermediate.tmp"
#define OUTFILE "lc3.out"

/* opcodes */
enum {
    BR = 0x0,
    ADD = 0x1,
    LD = 0x2,
    ST = 0x3,
    JSR = 0x4,
    AND = 0x5,
    LDR = 0x6,
    STR = 0x7,
    RTI = 0x8,
    NOT = 0x9,
    LDI = 0xa,
    STI = 0xb,
    JMP = 0xc,
    LEA = 0xe,
    TRAP = 0xf
};

typedef struct s_operation {
    char *name;
    uint16_t opcode;
    int nargs;
    int n, z, p;
} Operation;

typedef struct s_symbol {
    char *name;
    int value;
    int type;

    uint8_t hash;
    struct s_symbol *next;
} Symbol;

typedef struct s_table {
    int size;
    int capacity;

    Symbol *buckets[MAX_NUM_SYMBOLS];
    Symbol store[MAX_NUM_SYMBOLS];
    int sp; /* indexes store */
} Table;

FILE *infile, *irfile, *outfile;
Operation opTable[NUM_INSTRUCTIONS];
Table symTable;
int lc = 0, ln = 1;

void fatal(char *msg) {
    fprintf(stderr, "fatal: %s - lc = %d\n", msg, lc);
    remove(IRFILE);
    remove(OUTFILE);
    fclose(irfile);
    fclose(infile);
    exit(1);
}

Operation newOperation(char *name, uint16_t opcode, int nargs) {
    Operation op;

    op.name = malloc(sizeof(char) * (strlen(name) + 1));
    strcpy(op.name, name);

    op.opcode = opcode;
    op.nargs = nargs;

    op.n = op.z = op.p = 0;

    return op;
}

int findOperation(char *name) {
    int i;
    for (i = 0; i < NUM_INSTRUCTIONS; i++) {
        if (strcmp(name, opTable[i].name) == 0) {
            return i;
        }
    }

    return -1;
}

void delOperation(Operation *instr) {
    free(instr->name);
}

void initOpTable(void) {
    opTable[0] = newOperation("ADD", ADD, 3);
    opTable[1] = newOperation("AND", AND, 3);

    opTable[2] = newOperation("BRn", BR, 1);
    opTable[2].n = 1;

    opTable[3] = newOperation("BRp", BR, 1);
    opTable[3].p = 1;

    opTable[4] = newOperation("BRz", BR, 1);
    opTable[4].z = 1;

    opTable[5] = newOperation("BR", BR, 1);
    opTable[5].n = opTable[5].p = opTable[5].z = 1;

    opTable[6] = newOperation("BRzp", BR, 1);
    opTable[6].z = opTable[6].p = 1;

    opTable[7] = newOperation("BRnp", BR, 1);
    opTable[7].n = opTable[7].p = 1;

    opTable[8] = newOperation("BRnz", BR, 1);
    opTable[8].z = opTable[8].n = 1;

    opTable[9] = newOperation("BRnzp", BR, 1);
    opTable[5].n = opTable[5].p = opTable[5].z = 1;

    opTable[10] = newOperation("JMP", JMP, 1);
    opTable[11] = newOperation("JSR", JSR, 1);
    opTable[12] = newOperation("JSRR", JSR, 1);
    opTable[13] = newOperation("LD", LD, 2);
    opTable[14] = newOperation("LDI", LDI, 2);
    opTable[15] = newOperation("LDR", LDR, 3);
    opTable[16] = newOperation("LEA", LEA, 2);
    opTable[17] = newOperation("NOT", NOT, 2);
    opTable[18] = newOperation("RET", JMP, 0);
    opTable[19] = newOperation("RTI", RTI, 0);
    opTable[20] = newOperation("ST", ST, 2);
    opTable[21] = newOperation("STI", STI, 2);
    opTable[22] = newOperation("STR", STR, 3);
    opTable[23] = newOperation("TRAP", TRAP, 1);

    /* trap routines */
    opTable[24] = newOperation("GETC", 0xf020, 0);
    opTable[25] = newOperation("OUT", 0xf021, 0);
    opTable[26] = newOperation("PUTS", 0xf022, 0);
    opTable[27] = newOperation("IN", 0xf023, 0);
    opTable[28] = newOperation("PUTSP", 0xf024, 0);
    opTable[29] = newOperation("HALT", 0xf025, 0);

    /* pseudo-ops */
    opTable[30] = newOperation(".ORIG", 0xffff, 1);
    opTable[31] = newOperation(".FILL", 0xffff, 1);
    opTable[32] = newOperation(".BLKW", 0xffff, 1);
    opTable[33] = newOperation(".STRINGZ", 0xffff, 1);
    opTable[34] = newOperation(".END", 0xffff, 0);
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

int isBranchInstr(int opidx) {
    return opidx >= 2 && opidx <= 9;
}

void deleteOpTable() {
    int i;
    for (i = 0; i < NUM_INSTRUCTIONS; i++) {
        delOperation(&opTable[i]);
    }
}

void deleteSymTable() {
    int i;
    for (i = 0; i < MAX_NUM_SYMBOLS; i++) {
        free(symTable.store[i].name);
    }
}

void initSymTable(Table *this) {
    this->size = 0;
    this->capacity = MAX_NUM_SYMBOLS;
    this->sp = 0;

    int i;
    for (i = 0; i < MAX_NUM_SYMBOLS; i++) {
        this->buckets[i] = NULL;
    }
}

uint8_t hash(char *str) {
    uint8_t hash = 53;
    uint8_t c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

void putSymbol(Table *table, Symbol *symbol) {
    if (table->size == MAX_NUM_SYMBOLS) {
        fatal("maximum number of symbols has been reached");
    }
    Symbol **entry = &table->buckets[symbol->hash];

    while (*entry) {
        *entry = (*entry)->next;
    }
    *entry = symbol;

    table->size++;
}

Symbol *getSymbol(Table *table, char *name) {
    if (table->size == 0) {
        return NULL;
    }
    uint16_t h = hash(name);
    Symbol *entry = table->buckets[h];

    while (entry) {
        if (entry->hash == h) {
            if (strcmp(entry->name, name) == 0) {
                return entry;
            }
        }
        entry = entry->next;
    }

    return NULL;
}

/*
 * NewSymbol first checks if name has already been interned in
 * the symbol table; if so, it returns it. Otherwise, it creates
 * and interns the symbol before returning it. If type is 1, the
 * symbol is a label.
 */
Symbol *newSymbol(char *name, int value, int type) {
    Symbol *symbol;
    if ((symbol = getSymbol(&symTable, name))) {
        if (value != -1) {
            symbol->value = value;
        }
        printf("%s->value = %d\n", symbol->name, symbol->value);
        return symbol;
    }

    if (symTable.sp == MAX_NUM_SYMBOLS) {
        fatal("maximum number of symbols reached");
    }

    symbol = &symTable.store[symTable.sp++];
    symbol->name = malloc(sizeof(char) * (strlen(name) + 1));
    strcpy(symbol->name, name);
    symbol->value = value;
    symbol->type = type;

    symbol->hash = hash(name);
    symbol->next = NULL;

    putSymbol(&symTable, symbol);
    return symbol;
}

void skipSpaces(char *line, int *pos) {
    char c = line[*pos];
    while (isspace(c) && c != '\n') {
        c = line[++(*pos)];
    }
}

/*
 *  ParseOperation parses one alphanumeric word from line.
 *  If the word is an Operation, it returns its opTable index.
 *  Otherwise, it assumes it's a label, adds it
 *  to the symbol table, and returns -1.
 */
int parseOperation(char *line, int *pos) {
    char buf[MAX_SYMBOL_LEN], *bufp, c;
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
        newSymbol(buf, lc, 0);
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
    char buf[MAX_SYMBOL_LEN], *bufp, c;

    bufp = buf;
    while ((c = line[*pos]) && !isspace(c) && c != ',' && c != '\n') {
        *bufp++ = c;
        (*pos)++;
        if (bufp - buf >= MAX_SYMBOL_LEN - 1) {
            fatal("maximum symbol length reached");
        }
    }
    *bufp = '\0';

    fprintf(irfile, "%s ", buf);

    newSymbol(buf, -1, 0);
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
    op = &opTable[opidx];

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
    op = opTable[opidx];

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
        char str[MAX_SYMBOL_LEN];
        nargs = fscanf(irfile, "%s", str);
        if (nargs != 1) {
            fatal("argument error in BR");
        }
        Symbol *sym = getSymbol(&symTable, str);
        if (!sym) {
            fatal("unbound symbol in BR");
        }
        uint16_t offset = sym->value - (lc + 1);
        *instr |=
            (op.n << 11) | (op.z << 10) | (op.p << 9) | (offset & 0x1ff);
        break;
    }
    case LD: {
        uint16_t dr;
        char str[MAX_SYMBOL_LEN];
        nargs = fscanf(irfile, "R%hu %s", &dr, str);
        if (nargs != 2) {
            fatal("argument error in LD");
        }
        Symbol *sym = getSymbol(&symTable, str);
        if (!sym) {
            fatal("unbound symbol in LD");
        }
        uint16_t offset = sym->value - (lc + 1);
        *instr |= (dr << 9) | (offset & 0x1f);
        break;
    }
    case LEA: {
        uint16_t dr;
        char str[MAX_SYMBOL_LEN];
        nargs = fscanf(irfile, "R%hu %s", &dr, str);
        if (nargs != 2) {
            fatal("argument error in LEA");
        }
        Symbol *sym = getSymbol(&symTable, str);
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
    *instr |= opTable[opidx].opcode;
    printf("writing TRAP \\x%x to file...\n", opTable[opidx].opcode & 0xfff);
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
    deleteOpTable();
    deleteSymTable();
}

int main(int argc, char **argv) {
    initOpTable();
    initSymTable(&symTable);

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
