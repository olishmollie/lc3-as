#include "core.h"
#include "op.h"
#include "trap.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {

    if (argc != 2) {
        printf("usage: blah blah blah\n");
    }

    VM vm;
    boot(&vm);

    uint16_t baser, cond, dr, flgs, imm5, instr, offset6, op, pcoffset9,
        pcoffset11, pmode, sr, sr1, sr2, start, trapvect8;

    start = read_obj(&vm, argv[1]);
    vm.reg[PC] = start;

    /* MCR[15] controls the clock. If 1, we run; if none, we're done. */
    while (*vm.mcr) {

        instr = mem_read(&vm, vm.reg[PC]++);
        op = instr >> 12;

        switch (op) {
        case ADD:
            dr = (instr >> 9) & 0x7;
            sr1 = (instr >> 6) & 0x7;
            if ((instr >> 5) & 0x1) {
                imm5 = instr & 0x1f;
                vm.reg[dr] = vm.reg[sr1] + sext(imm5, 5);
            } else {
                sr2 = instr & 0x7;
                vm.reg[dr] = vm.reg[sr1] + vm.reg[sr2];
            }
            setcc(&vm, dr);
            break;
        case AND:
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
            pcoffset9 = sext(instr & 0x1ff, 9);
            flgs = vm.reg[PSR] & 0x7;
            cond = (instr >> 9) & 0x7;
            if (flgs & cond) {
                vm.reg[PC] += pcoffset9;
            }
            break;
        case JMP:
            baser = (instr >> 6) & 0x7;
            vm.reg[PC] = vm.reg[baser];
            break;
        case JSR:
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
            pcoffset9 = instr & 0x1ff;
            dr = (instr >> 9) & 0x7;
            vm.reg[dr] = mem_read(&vm, vm.reg[PC] + sext(pcoffset9, 9));
            setcc(&vm, dr);
            break;
        case LDI:
            pcoffset9 = instr & 0x1ff;
            dr = (instr >> 9) & 0x7;
            vm.reg[dr] =
                mem_read(&vm, mem_read(&vm, vm.reg[PC] + sext(pcoffset9, 9)));
            setcc(&vm, dr);
            break;
        case LDR:
            offset6 = instr & 0x3f;
            baser = (instr >> 6) & 0x7;
            dr = (instr >> 9) & 0x7;
            vm.reg[dr] = mem_read(&vm, vm.reg[baser] + sext(offset6, 6));
            setcc(&vm, dr);
            break;
        case LEA:
            pcoffset9 = instr & 0x1ff;
            dr = (instr >> 9) & 0x7;
            vm.reg[dr] = vm.reg[PC] + sext(pcoffset9, 9);
            setcc(&vm, dr);
            break;
        case NOT:
            sr = (instr >> 6) & 0x7;
            dr = (instr >> 9) & 0x7;
            vm.reg[dr] = ~vm.reg[sr];
            setcc(&vm, dr);
            break;
        case RTI:
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
            pcoffset9 = sext(instr & 0x1ff, 9);
            sr = (instr >> 9) & 0x7;
            mem_write(&vm, vm.reg[PC] + pcoffset9, vm.reg[sr]);
            break;
        case STI:
            pcoffset9 = sext(instr & 0x1ff, 9);
            sr = (instr >> 9) & 0x7;
            mem_write(&vm, mem_read(&vm, vm.reg[PC] + pcoffset9), vm.reg[sr]);
            break;
        case STR:
            offset6 = instr & 0x3f;
            baser = (instr >> 6) & 0x7;
            sr = (instr >> 9) & 0x3;
            mem_write(&vm, vm.reg[baser] + sext(offset6, 6), vm.reg[sr]);
            break;
        case TRAP:
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
    vm->mem[PUTSP & 0xff] = 0x04e0;
    vm->mem[HALT & 0xff] = 0xfd70;

    /* Load trap routines */
    memcpy(&vm->mem[0x0400], &tr_getc, sizeof(tr_getc));
    memcpy(&vm->mem[0x0430], &tr_out, sizeof(tr_out));
    memcpy(&vm->mem[0x0450], &tr_puts, sizeof(tr_puts));
    memcpy(&vm->mem[0x04a0], &tr_in, sizeof(tr_in));
    memcpy(&vm->mem[0x04e0], &tr_putsp, sizeof(tr_putsp));
    memcpy(&vm->mem[0xfd70], &tr_halt, sizeof(tr_halt));
}

uint16_t mem_read(VM *vm, uint16_t loc) {
    /* If reading from KBDR, get char from keyboard */
    if (&vm->mem[loc] == vm->kbdr) {
        *vm->kbsr &= 0x7fff;
        *vm->kbdr = (uint16_t)getc(stdin);
        *vm->kbsr = 0x8000;
    }
    return vm->mem[loc];
}

void mem_write(VM *vm, uint16_t loc, uint16_t val) {
    /* If writing to DDR, display val on screen */
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
    /* First word of file should be origin */
    uint16_t origin;
    fread(&origin, sizeof(origin), 1, file);

    /* Read the rest of the program */
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
