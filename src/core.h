#ifndef CORE_H
#define CORE_H

#include "common.h"

typedef struct VM {
    uint16_t mem[UINT16_MAX];
    uint16_t reg[10];

    uint16_t *kbsr;
    uint16_t *kbdr;
    uint16_t *dsr;
    uint16_t *ddr;
    uint16_t *mcr;

} VM;

/* Registers */
enum {
      R0 = 0,
      R1,
      R2,
      R3,
      R4,
      R5,
      R6,
      R7,
      PC,
      PSR,
};

void boot(VM *vm);

uint16_t mem_read(VM *vm, uint16_t loc);
void mem_write(VM *vm, uint16_t loc, uint16_t val);

uint16_t read_obj(VM *vm, const char *path);
uint16_t read_obj_file(VM *vm, FILE *file);
uint16_t sext(uint16_t x, uint16_t nbits);
void setcc(VM *vm, uint16_t r);

#endif
