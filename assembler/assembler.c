#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NUM_SYMBOLS 256
#define MAX_SYMBOL_LEN 11
#define NUM_INSTRUCTIONS 35
#define INTERMEDIATE_FILE "intermediate.tmp"

/* opcodes */
enum {
    BR = 0x0,
    ADD,
    LD,
    ST,
    JSR,
    AND,
    LDR,
    STR,
    RTI,
    NOT,
    LDI,
    STI,
    JMP,
    LEA,
    TRAP
};

typedef struct s_operation {
    char *name;
    uint16_t opcode;
    int nargs;
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

FILE *fp, *ifp;
Operation opTable[NUM_INSTRUCTIONS];
Table symTable;
int lc = 0;

void fatal(char *msg) {
    fprintf(stderr, "fatal: %s, line %d\n", msg, lc + 1);
    remove(INTERMEDIATE_FILE);
    fclose(ifp);
    exit(1);
}

Operation newOperation(char *name, uint16_t opcode, int nargs) {
    Operation op;

    op.name = malloc(sizeof(char) * (strlen(name) + 1));
    strcpy(op.name, name);

    op.opcode = opcode;
    op.nargs = nargs;

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
    opTable[3] = newOperation("BRp", BR, 1);
    opTable[4] = newOperation("BRz", BR, 1);
    opTable[5] = newOperation("BR", BR, 1);
    opTable[6] = newOperation("BRzp", BR, 1);
    opTable[7] = newOperation("BRnp", BR, 1);
    opTable[8] = newOperation("BRnz", BR, 1);
    opTable[9] = newOperation("BRnzp", BR, 1);
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
 * the symbol table; if so, it updates its value and returns it.
 * Otherwise, it creates and interns the symbol before returning it.
 */
Symbol *newSymbol(char *name, int value, int type) {
    Symbol *symbol;
    if ((symbol = getSymbol(&symTable, name))) {
        if (symbol->value == -1) {
            symbol->value = value;
        }
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
int parseOperation(char *line, int *pos, FILE *ifp) {
    char buf[MAX_SYMBOL_LEN], *bufp, c;
    int opIndex;

    skipSpaces(line, pos);

    bufp = buf;
    while ((c = line[*pos]) && !isspace(c)) {
        *bufp++ = c;
        (*pos)++;
    }
    *bufp = '\0';

    skipSpaces(line, pos);

    opIndex = findOperation(buf);
    if (opIndex != -1) {
        return opIndex;
    }

    if (bufp - buf != 0) {
        newSymbol(buf, lc, 0);
    }

    return -1;
}

void parseArg(char *line, int *pos, FILE *ifp) {
    char buf[MAX_SYMBOL_LEN], *bufp, c;

    skipSpaces(line, pos);

    bufp = buf;
    while ((c = line[*pos]) && !isspace(c) && c != ',' && c != '\n') {
        *bufp++ = c;
        (*pos)++;
    }
    *bufp = '\0';

    Symbol *symbol;
    if (buf[0] == 'R' || buf[0] == '#' || buf[0] == 'x') {
        fprintf(ifp, "%s ", buf);
    } else {
        symbol = newSymbol(buf, -1, 0);
        fprintf(ifp, "%s ", buf);
    }

    skipSpaces(line, pos);
}

/*
 * ParseArgs attempts to parse op->nargs number of arguments from
 * the line at position pos. Once parsed, each argument is written to
 * the intermediate file, denoted by ifp.
 */
void parseArgs(char *line, int *pos, Operation *op, FILE *ifp) {
    int i;

    for (i = 0; i < op->nargs; i++) {
        parseArg(line, pos, ifp);

        if (i != op->nargs - 1) {
            if (line[*pos] != ',') {
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
 * to by ifp. If line does not end in a newline character, a fatal
 * error occurs.
 */
void parseLine(char *line, int pos, FILE *ifp) {
    int opIndex;
    Operation *op;

    fprintf(ifp, "%d ", lc);

    opIndex = parseOperation(line, &pos, NULL);

    if (opIndex == -1) {
        opIndex = parseOperation(line, &pos, ifp);
        if (opIndex == -1) {
            fatal("invalid syntax");
        }
    }
    op = &opTable[opIndex];

    fprintf(ifp, "%d ", opIndex);

    parseArgs(line, &pos, op, ifp);

    if (line[pos] != '\n') {
        printf("line[pos] = '%c'\n", line[pos]);
        fatal("expected newline");
    }

    fprintf(ifp, "\n");
    lc++;
}

void parse() {
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    while ((read = getline(&line, &len, fp)) != -1) {
        if (read == 1) {
            continue; /* skip blank lines */
        }
        parseLine(line, 0, ifp);
    }

    if (line) {
        free(line);
    }
}

void cleanup() {
    fclose(fp);
    fclose(ifp);
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

    fp = fopen(argv[1], "r");
    if (!fp) {
        fatal("could not open source file");
    }
    ifp = fopen(INTERMEDIATE_FILE, "w+");
    if (!ifp) {
        fatal("could not open new intermediate file");
    }

    parse();

    cleanup();

    return 0;
}
