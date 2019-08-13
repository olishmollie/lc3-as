#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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

#define PC_START 0x3000

uint16_t dr, start;

uint16_t read_obj(const char *path);
uint16_t read_obj_file(FILE *file);
uint16_t swap16(uint16_t x);
uint16_t sext(uint16_t x, uint16_t nbits);
void setcc(void);

int main(int argc, char **argv) {

    if (argc != 2) {
        printf("usage: blah blah blah\n");
    }

    start = read_obj(argv[1]);
    reg[R_PC] = start;

    uint16_t baser, cond, flgs, imm5, instr, offset6, op, pcoffset9, pcoffset11,
        pmode, run, sr, sr1, sr2, trapvect8;

    run = 1;
    while (run) {

        instr = mem[reg[R_PC]++];
        op = instr >> 12;

        switch (op) {
        case OP_ADD:
            dr = (instr >> 9) & 0x7;
            sr1 = (instr >> 6) & 0x7;
            if ((instr >> 5) & 0x1) {
                imm5 = instr & 0x1f;
                reg[dr] = reg[sr1] + sext(imm5, 5);
            } else {
                sr2 = instr & 0x7;
                reg[dr] = reg[sr1] + reg[sr2];
            }
            setcc();
            break;
        case OP_AND: {
            dr = (instr >> 9) & 0x7;
            sr1 = (instr >> 6) & 0x7;
            if ((instr >> 5) & 0x1) {
                imm5 = instr & 0x1f;
                reg[dr] = reg[sr1] & sext(imm5, 5);
            } else {
                sr2 = instr & 0x7;
                reg[dr] = sr1 & sr2;
            }
            setcc();
            break;
        }
        case OP_BR: {
            pcoffset9 = sext(instr & 0x1ff, 9);
            flgs = reg[R_PSR] & 0x7;
            cond = (instr >> 9) & 0x7;
            if (flgs & cond) {
                reg[R_PC] += pcoffset9;
            }
            break;
        }
        case OP_JMP:
            baser = (instr >> 6) & 0x7;
            reg[R_PC] = reg[baser];
            break;
        case OP_JSR:
            reg[R_R7] = reg[R_PC];
            if (instr >> 11) {
                pcoffset11 = instr & 0x7ff;
                reg[R_PC] += sext(pcoffset11, 11);
            } else {
                baser = (instr >> 6) & 0x7;
                reg[R_PC] = reg[baser];
            }
            break;
        case OP_LD:
            pcoffset9 = instr & 0x1ff;
            dr = (instr >> 9) & 0x7;
            reg[dr] = mem[reg[R_PC] + sext(pcoffset9, 9)];
            setcc();
            break;
        case OP_LDI:
            pcoffset9 = instr & 0x1ff;
            dr = (instr >> 9) & 0x7;
            reg[dr] = mem[mem[reg[R_PC] + sext(pcoffset9, 9)]];
            setcc();
            break;
        case OP_LDR:
            offset6 = instr & 0x3f;
            baser = (instr >> 6) & 0x7;
            dr = (instr >> 9) & 0x7;
            reg[dr] = mem[reg[baser] + sext(offset6, 6)];
            setcc();
            break;
        case OP_LEA:
            pcoffset9 = instr & 0x1ff;
            dr = (instr >> 9) & 0x7;
            reg[dr] = reg[R_PC] + sext(pcoffset9, 9);
            setcc();
            break;
        case OP_NOT:
            sr = (instr >> 6) & 0x7;
            dr = (instr >> 9) & 0x7;
            reg[dr] = ~reg[sr];
            setcc();
            break;
        case OP_RTI:
            pmode = (reg[R_PSR] >> 15) & 0x0;
            if (pmode) {
                reg[R_PC] = mem[reg[R_R6]++]; /* R6 stores SSP */
                reg[R_PSR] = mem[reg[R_R6]++];
            } else {
                fprintf(stderr, "privilege mode exception\n");
                exit(1);
            }
            break;
        case OP_ST:
            pcoffset9 = instr & 0x1ff;
            sr = (instr >> 9) & 0x7;
            mem[reg[R_PC] + sext(pcoffset9, 9)] = reg[sr];
            break;
        case OP_STI:
            pcoffset9 = instr & 0x1ff;
            sr = (instr >> 9) & 0x7;
            mem[mem[reg[R_PC] + sext(pcoffset9, 9)]] = reg[sr];
            break;
        case OP_STR:
            offset6 = instr & 0x3f;
            baser = (instr >> 6) & 0x7;
            sr = (instr >> 9) & 0x3;
            mem[reg[baser] + sext(offset6, 6)] = reg[sr];
            break;
        case OP_TRAP:
            trapvect8 = instr & 0xff;
            reg[R_R7] = reg[R_PC];
            uint16_t c;
            switch (trapvect8) {
            case TRAP_GETC:
                c = (uint16_t)getc(stdin);
                reg[R_R0] = c;
                break;
            case TRAP_OUT:
                c = reg[R_R0];
                putc((char)c, stdout);
                fflush(stdout);
                break;
            case TRAP_PUTS:
                while ((c = mem[reg[R_R0]++])) {
                    putc((char)c, stdout);
                }
                break;
            case TRAP_IN:
                printf("Enter a character: ");
                c = (uint16_t)getc(stdin);
                putc((char)c, stdout);
                reg[R_R0] = c;
                break;
            case TRAP_PUTSP:
                while ((c = reg[R_R0]++)) {
                    char a, b;
                    a = c & 0xff;
                    b = c >> 8;
                    putc(a, stdout);
                    if (b)
                        putc(b, stdout);
                    else
                        break;
                }
                fflush(stdout);
                break;
            case TRAP_HALT:
                /* printf("HALT\n"); */
                fflush(stdout);
                run = 0;
                break;
            default:
                fprintf(stderr, "illegal trap exception: %x\n", trapvect8);
                exit(1);
            }
            break;
        default:
            fprintf(stderr, "illegal opcode exception: \\x%4x\n", op);
            exit(1);
        }
    }

    return 0;
}

uint16_t read_obj(const char *path) {
    uint16_t start;
    FILE *file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "unable to open img\n");
        exit(1);
    }
    start = read_obj_file(file);

    fclose(file);

    return start;
}

uint16_t read_obj_file(FILE *file) {
    /* first word of file should be origin */
    uint16_t origin;
    fread(&origin, sizeof(origin), 1, file);

    /* read the rest of the program */
    uint16_t max_read, *p;
    size_t read;
    max_read = UINT16_MAX - origin;
    p = mem + origin;
    read = fread(p, sizeof(uint16_t), max_read, file);

    return origin;
}

uint16_t swap16(uint16_t x) {
    return (x << 8) | (x >> 8);
}

uint16_t sext(uint16_t x, uint16_t nbits) {
    if ((x >> (nbits - 1)) & 1) {
        x |= (0xffff << nbits);
    }
    return x;
}

void setcc() {
    reg[R_PSR] &= 0x0;
    uint16_t t = reg[dr], c;
    if (t > 0) {
        c = 0x1;
    } else if (t < 0) {
        c = 0x6;
    } else {
        c = 0x4;
    }
    reg[R_PSR] |= c;
}
