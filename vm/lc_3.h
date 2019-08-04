#ifndef LC3_H
#define LC3_H

#include <stdint.h>

/* addressable memory */
uint16_t mem[UINT16_MAX];

/* registers stored from 0xfe00 to 0xffff*/
uint16_t *reg = &mem[0xfe00];

/* registers */
enum {
      R_R0 = 0,
      R_R1,
      R_R2,
      R_R3,
      R_R4,
      R_R5,
      R_R6,
      R_R7,
      R_PC,
      R_PSR,
      R_COUNT
};

/* opcodes */
enum {
      OP_BR = 0,
      OP_ADD,
      OP_LD,
      OP_ST,
      OP_JSR,
      OP_AND,
      OP_LDR,
      OP_STR,
      OP_RTI,
      OP_NOT,
      OP_LDI,
      OP_STI,
      OP_JMP,
      OP_RES,
      OP_LEA,
      OP_TRAP
};

/* traps */
enum {
      TRAP_GETC = 0x20,
      TRAP_OUT,
      TRAP_PUTS,
      TRAP_IN,
      TRAP_PUTSP,
      TRAP_HALT
};

#endif
