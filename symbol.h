#ifndef SYMBOL_H
#define SYMBOL_H

#include "common.h"

#define MAX_SYMBOL_LENGTH 11

typedef struct Symbol {
    char *name;
    int value;

    uint32_t hash;
    struct Symbol *next;
} Symbol;

typedef struct Table {
    int size;
    int capacity;

    Symbol **buckets;
} Table;

Symbol *newSymbol(Table *table, char *name, int value);

void initTable(Table *table);
Symbol *getSymbol(Table *table, char *name);
void deleteTable(Table *table);

/* Global Symbol Table */
Table symbolTable;

#endif
