#include "core.h"
#include "op.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {

    if (argc != 2) {
        printf("usage: blah blah blah\n");
    }

    VM vm;
    boot(&vm);

    for (int i = 0; i < UINT16_MAX; i++) {
        printf("mem[\\x%x] = \\x%x\n", i, vm.mem[i]);
    }

    uint16_t baser, cond, dr, flgs, imm5, instr, offset6, op, pcoffset9,
        pcoffset11, pmode, sr, sr1, sr2, start, trapvect8;

    start = read_obj(&vm, argv[1]);
    vm.reg[PC] = start;

    /* MCR[15] controls the clock. If 1, we run. If none, we're done. */
    while (*vm.mcr) {

        instr = mem_read(&vm, vm.reg[PC]++);
        op = instr >> 12;

        switch (op) {
        case ADD:
            /* printf("ADD \\x%x\n", instr); */
            dr = (instr >> 9) & 0x7;
            sr1 = (instr >> 6) & 0x7;
            if ((instr >> 5) & 0x1) {
                imm5 = instr & 0x1f;
		/* printf("imm5 = %d\n", imm5); */
                vm.reg[dr] = vm.reg[sr1] + sext(imm5, 5);
            } else {
                sr2 = instr & 0x7;
		/* printf("sr2 = %d\n", sr2); */
                vm.reg[dr] = vm.reg[sr1] + vm.reg[sr2];
            }
	    /* printf("reg[%d] = \\x%x\n", dr, vm.reg[dr]); */
            setcc(&vm, dr);
            break;
        case AND:
            /* printf("AND \\x%x\n", instr); */
            dr = (instr >> 9) & 0x7;
            sr1 = (instr >> 6) & 0x7;
            if ((instr >> 5) & 0x1) {
                imm5 = instr & 0x1f;
                vm.reg[dr] = vm.reg[sr1] & sext(imm5, 5);
            } else {
                sr2 = instr & 0x7;
                vm.reg[dr] = vm.reg[sr1] & vm.reg[sr2];
            }
            setcc(&vm, dr);
            break;
        case BR:
            /* printf("BR \\x%x\n", instr); */
            pcoffset9 = sext(instr & 0x1ff, 9);
            flgs = vm.reg[PSR] & 0x7;
            cond = (instr >> 9) & 0x7;
            /* printf("flgs = \\x%x\n", flgs); */
            /* printf("cond = \\x%x\n", cond); */
            if (flgs & cond) {
                /* printf("jumping %d spots...\n", (int16_t)pcoffset9); */
                vm.reg[PC] += pcoffset9;
            }
            break;
        case JMP:
            /* printf("JMP \\x%x\n", instr); */
            baser = (instr >> 6) & 0x7;
            vm.reg[PC] = vm.reg[baser];
            break;
        case JSR:
            /* printf("JSR \\x%x\n", instr); */
            vm.reg[R7] = vm.reg[PC];
            if (instr >> 11) {
                pcoffset11 = instr & 0x7ff;
                vm.reg[PC] += sext(pcoffset11, 11);
            } else {
                baser = (instr >> 6) & 0x7;
                vm.reg[PC] = vm.reg[baser];
            }
            break;
        case LD:
            /* printf("LD \\x%x\n", instr); */
            pcoffset9 = instr & 0x1ff;
            dr = (instr >> 9) & 0x7;
            vm.reg[dr] = mem_read(&vm, vm.reg[PC] + sext(pcoffset9, 9));
            setcc(&vm, dr);
            break;
        case LDI:
            /* printf("LDI \\x%x\n", instr); */
            pcoffset9 = instr & 0x1ff;
            dr = (instr >> 9) & 0x7;
            vm.reg[dr] =
                mem_read(&vm, mem_read(&vm, vm.reg[PC] + sext(pcoffset9, 9)));
            setcc(&vm, dr);
            break;
        case LDR:
            /* printf("LDR \\x%x\n", instr); */
            offset6 = instr & 0x3f;
            baser = (instr >> 6) & 0x7;
            dr = (instr >> 9) & 0x7;
            vm.reg[dr] = mem_read(&vm, vm.reg[baser] + sext(offset6, 6));
            /* printf("reg[%d] = '%c'\n", dr, (char)vm.reg[dr]); */
            setcc(&vm, dr);
            break;
        case LEA:
            /* printf("LEA \\x%x\n", instr); */
            pcoffset9 = instr & 0x1ff;
            dr = (instr >> 9) & 0x7;
            vm.reg[dr] = vm.reg[PC] + sext(pcoffset9, 9);
            setcc(&vm, dr);
            break;
        case NOT:
            /* printf("NOT \\x%x\n", instr); */
            sr = (instr >> 6) & 0x7;
            dr = (instr >> 9) & 0x7;
            vm.reg[dr] = ~vm.reg[sr];
            setcc(&vm, dr);
            break;
        case RTI:
            /* printf("RTI \\x%x\n", instr); */
            pmode = (vm.reg[PSR] >> 15) & 0x0;
            if (pmode) {
                vm.reg[PC] = vm.mem[vm.reg[R6]++]; /* R6 stores SSP */
                vm.reg[PSR] = vm.mem[vm.reg[R6]++];
            } else {
                fprintf(stderr, "privilege mode exception\n");
                exit(1);
            }
            break;
        case ST:
            /* printf("ST \\x%x\n", instr); */
            pcoffset9 = instr & 0x1ff;
            sr = (instr >> 9) & 0x7;
            mem_write(&vm, vm.reg[PC] + sext(pcoffset9, 9), vm.reg[sr]);
            break;
        case STI:
            /* printf("STI \\x%x\n", instr); */
            pcoffset9 = instr & 0x1ff;
            sr = (instr >> 9) & 0x7;
            mem_write(&vm, mem_read(&vm, vm.reg[PC] + sext(pcoffset9, 9)),
                      vm.reg[sr]);
            break;
        case STR:
            /* printf("STR \\x%x\n", instr); */
            offset6 = instr & 0x3f;
            baser = (instr >> 6) & 0x7;
            sr = (instr >> 9) & 0x3;
            mem_write(&vm, vm.reg[baser] + sext(offset6, 6), vm.reg[sr]);
            break;
        case TRAP:
            /* printf("TRAP \\x%x\n", instr); */
            trapvect8 = instr & 0xff;
            /* Save current PC in R7 */
            vm.reg[R7] = vm.reg[PC];
            /* Set PC to memory location TRAP routine */
            vm.reg[PC] = vm.mem[trapvect8];
            break;
        default:
            fprintf(stderr, "illegal opcode exception: \\x%4x\n", op);
            exit(1);
        }
    }

    return 0;
}

void boot(VM *vm) {
    /* Zero out memory */
    memset(vm->mem, 0, sizeof(vm->mem));

    /* KBSR - Keyboard Status Register */
    vm->kbsr = &vm->mem[0xfe00];
    *vm->kbsr = 0x8000;

    /* KBDR - Keyboard Data Register */
    vm->kbdr = &vm->mem[0xfe02];
    *vm->kbdr = 0x0;

    /* DSR - Display Status Register */
    vm->dsr = &vm->mem[0xfe04];
    *vm->dsr = 0x8000;

    /* DDR - Display Data Register */
    vm->ddr = &vm->mem[0xfe06];
    *vm->ddr = 0x0;

    /* MCR - Machine Control Register */
    vm->mcr = &vm->mem[0xfffe];
    *vm->mcr = 0x8000;

    /* Trap table */
    vm->mem[GETC & 0xff] = 0x0400;
    vm->mem[OUT & 0xff] = 0x0430;
    vm->mem[PUTS & 0xff] = 0x0450;

    vm->mem[IN & 0xff] = 0x04a0;
    uint16_t in[] = {0x321b,        0x341b,        0x361b,        0x241f,
                     0xa61a,        0x7fe,         0xb419,        0xe21c,
                     0x6040,        0x405,         0xa614,        0x7fe,
                     0xb013,        0x1261,        0xff9,         0xa611,
                     0x7fe,         0xa010,        0xa60c,        0x7fe,
                     0xb00b,        0xa609,        0x7fe,         0xb408,
                     0x2203,        0x2403,        0x2603,        0xc1c0,
                     0x0000,        0x0000,        0x0000,        0xfe04,
                     0xfe06,        0xfe00,        0xfe02,        0x000a,
                     (uint16_t)'I', (uint16_t)'n', (uint16_t)'p', (uint16_t)'u',
                     (uint16_t)'t', (uint16_t)' ', (uint16_t)'a', (uint16_t)' ',
                     (uint16_t)'c', (uint16_t)'h', (uint16_t)'a', (uint16_t)'r',
                     (uint16_t)'a', (uint16_t)'c', (uint16_t)'t', (uint16_t)'e',
                     (uint16_t)'r', (uint16_t)'>', (uint16_t)' ', 0x0000};
    memcpy(&vm->mem[0x04a0], &in, sizeof(in));

    vm->mem[PUTSP & 0xff] = 0x04e0;

    vm->mem[HALT & 0xff] = 0xfd70;
    uint16_t halt[] = {0x3008, 0x3208, 0xa208, 0x2008, 0x5040, 0xb005, 0x2002,
                       0x2202, 0xc1c0, 0x0000, 0x0000, 0xfffe, 0x7fff};
    memcpy(&vm->mem[0xfd70], &halt, sizeof(halt));

    /* uint16_t out[] = {0x3207, 0xa204, 0x0601, 0xb003, 0x2203, 0xc1c0}; */
    /* memcpy(&vm->mem[0x0430], &out, sizeof(out)); */
}

uint16_t mem_read(VM *vm, uint16_t loc) {
    if (&vm->mem[loc] == vm->kbdr) {
        /* printf("reading from kbdr\n"); */
        *vm->kbsr &= 0x7fff;
        *vm->kbdr = (uint16_t)getc(stdin);
        *vm->kbsr = 0x8000;
    }
    return vm->mem[loc];
}

void mem_write(VM *vm, uint16_t loc, uint16_t val) {
    if (&vm->mem[loc] == vm->ddr) {
        *vm->dsr &= 0x7fff;
        putc((char)val, stdout);
        *vm->dsr = 0x8000;
    }
    vm->mem[loc] = val;
}

uint16_t read_obj(VM *vm, const char *path) {
    uint16_t start;
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        fprintf(stderr, "unable to open img\n");
        exit(1);
    }
    start = read_obj_file(vm, fp);

    fclose(fp);

    return start;
}

uint16_t read_obj_file(VM *vm, FILE *file) {
    /* first word of file should be origin */
    uint16_t origin;
    fread(&origin, sizeof(origin), 1, file);

    /* read the rest of the program */
    uint16_t max_read, *p;
    max_read = UINT16_MAX - origin;
    p = vm->mem + origin;
    fread(p, sizeof(uint16_t), max_read, file);

    return origin;
}

uint16_t sext(uint16_t x, uint16_t nbits) {
    if ((x >> (nbits - 1)) & 1) {
        x |= (0xffff << nbits);
    }
    return x;
}

void setcc(VM *vm, uint16_t r) {
    vm->reg[PSR] &= 0x0;
    uint16_t t = vm->reg[r], c;
    if (t >> 15) {
        c = 0x4;
    } else if (t == 0) {
        c = 0x2;
    } else {
        c = 0x1;
    }
    vm->reg[PSR] |= c;
}
