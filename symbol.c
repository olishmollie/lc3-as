#include "symbol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 256
#define LOAD_FACTOR 0.7

#define ALLOC(p, n)                                                            \
    do {                                                                       \
        *p = calloc((n), sizeof(**p));                                         \
        if (*p == NULL) {                                                      \
            fprintf(stderr, "out of memory");                                  \
            exit(1);                                                           \
        }                                                                      \
    } while (0)

void initTable(Table *table) {
    table->size = 0;
    table->capacity = INITIAL_CAPACITY;
    ALLOC(&table->buckets, INITIAL_CAPACITY);
}

uint32_t hash(char *str) {
    uint32_t hash = 53;
    char c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

void grow(Table *table) {
    int newCapacity;
    Symbol **newBuckets, **entry;
    Symbol *symbol;

    newCapacity = table->capacity * 2;
    ALLOC(&newBuckets, newCapacity);

    int i;
    for (i = 0; i < table->capacity; i++) {
        if (table->buckets[i]) {
            symbol = table->buckets[i];
            entry = &table->buckets[symbol->hash % newCapacity];
            while (*entry) {
                *entry = (*entry)->next;
            }
            *entry = symbol;
        }
    }

    free(table->buckets);
    table->buckets = newBuckets;
    table->capacity = newCapacity;
}

void putSymbol(Table *table, Symbol *symbol) {
    if (table->size >= table->capacity * LOAD_FACTOR) {
        grow(table);
    }
    Symbol **entry = &table->buckets[symbol->hash % table->capacity];

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
    uint32_t h = hash(name);
    Symbol *entry = table->buckets[h % table->capacity];

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

Symbol *newSymbol(char *name, int value) {
    Symbol *symbol;
    if ((symbol = getSymbol(&symbolTable, name))) {
        if (value != -1) {
            symbol->value = value;
        }
        return symbol;
    }

    ALLOC(&symbol, 1);

    ALLOC(&symbol->name, strlen(name) + 1);
    strcpy(symbol->name, name);

    symbol->value = value;

    symbol->hash = hash(name);
    symbol->next = NULL;

    putSymbol(&symbolTable, symbol);
    return symbol;
}

void deleteSymbol(Symbol *symbol) {
    free(symbol->name);
    free(symbol);
}

void deleteTable(Table *table) {
    int i;
    for (i = 0; i < table->capacity; i++) {
        Symbol *symbol;
        if ((symbol = table->buckets[i])) {
            deleteSymbol(symbol);
        }
    }
    free(table->buckets);
}
