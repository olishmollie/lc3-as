#ifndef SYMBOL_H
#define SYMBOL_H

#include <stdint.h>

#define MAX_SYMBOL_LENGTH 11

typedef struct Symbol {
    char *name;
    int value;

    uint32_t hash;
    struct Symbol *next;
} Symbol;

/*
 * NewSymbol attempts to find name in the symbol table.
 * If the symbol is already interned, it returns it. 
 * Otherwise, it allocates memory for the new symbol,
 * interns it in the symbol table, and returns it.
 */
Symbol *newSymbol(char *name, int value);

typedef struct Table {
    int size;
    int capacity;

    Symbol **buckets;
} Table;

void initTable(Table *table);
Symbol *getSymbol(Table *table, char *name);
void deleteTable(Table *table);

/* Global Symbol Table */
Table symbolTable;

#endif
