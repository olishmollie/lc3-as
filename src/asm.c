#include "asm.h"
#include "parse.h"

void assemble(Program *prog, FILE *ostream) {
    Symbol *sym;
    uint16_t i, code;
    Instr instr;

    /* write origin to file */
    fwrite(&prog->orig, sizeof(prog->orig), 1, ostream);
    uint16_t instructions[prog->lc - 1];

    for (i = 0; i < prog->lc; i++) {
        instr = prog->instructions[i];
        code = instr.op->opcode << 12;

        switch (instr.op->opcode) {
        case ADD:
            code |= (instr.dr << 9) | (instr.sr1 << 6);
            if (instr.sr2 == 0xffff) {
                code |= (1 << 5) | (instr.imm5 & 0x1f);
            } else {
                code |= instr.sr2;
            }
	    instructions[i] = code;
            fwrite(&code, sizeof(code), 1, ostream);
            break;
        case AND:
            code |= (instr.dr << 9) | (instr.sr1 << 6);
            if (instr.sr2 == 0xffff) {
                code |= (1 << 5) | (instr.imm5 & 0x1f);
            } else {
                code |= instr.sr2;
            }
            fwrite(&code, sizeof(code), 1, ostream);
	    instructions[i] = code;
            break;
        case BR:
            code |= (instr.op->nzp << 9);
            sym = getSymbol(&prog->symbolTable, instr.pcoffset9->name);
            if (!sym) {
                fprintf(stderr, "unbound symbol '%s'\n", instr.pcoffset9->name);
                exit(1);
            }
            code |= (sym->value - instr.lc - 1) & 0x1ff;
	    instructions[i] = code;
            fwrite(&code, sizeof(code), 1, ostream);
            break;
        case JMP:
            code |= instr.baser << 6;
	    instructions[i] = code;
            fwrite(&code, sizeof(code), 1, ostream);
            break;
        case JSR:
            if (instr.pcoffset11) {
                sym = getSymbol(&prog->symbolTable, instr.pcoffset11->name);
                if (!sym) {
                    fprintf(stderr, "unbound symbol '%s'\n",
                            instr.pcoffset11->name);
                    exit(1);
                }
                code |= (0x1 << 11) | ((sym->value - instr.lc - 1) & 0x3ff);
            } else {
                code |= instr.baser << 6;
            }
	    instructions[i] = code;
            fwrite(&code, sizeof(code), 1, ostream);
            break;
        case LD:
            code |= instr.dr << 9;
            sym = getSymbol(&prog->symbolTable, instr.pcoffset9->name);
            if (!sym) {
                fprintf(stderr, "unbound symbol '%s'\n", instr.pcoffset9->name);
                exit(1);
            }
            code |= (sym->value - instr.lc - 1) & 0x1ff;
	    instructions[i] = code;
            fwrite(&code, sizeof(code), 1, ostream);
            break;
        case LDI:
            code |= instr.dr << 9;
            sym = getSymbol(&prog->symbolTable, instr.pcoffset9->name);
            if (!sym) {
                fprintf(stderr, "unbound symbol '%s'\n", instr.pcoffset9->name);
                exit(1);
            }
            code |= (sym->value - instr.lc - 1) & 0x1ff;
	    instructions[i] = code;
            fwrite(&code, sizeof(code), 1, ostream);
            break;
        case LEA:
            code |= instr.dr << 9;
            sym = getSymbol(&prog->symbolTable, instr.pcoffset9->name);
            if (!sym) {
                fprintf(stderr, "unbound symbol '%s'\n", instr.pcoffset9->name);
                exit(1);
            }
            code |= (sym->value - instr.lc - 1) & 0x1ff;
	    instructions[i] = code;
            fwrite(&code, sizeof(code), 1, ostream);
            break;
        case LDR:
            code |= (instr.dr << 9) | (instr.baser << 6) | instr.offset6;
	    instructions[i] = code;
            fwrite(&code, sizeof(code), 1, ostream);
            break;
        case NOT:
            code |= (instr.dr << 9) | (instr.sr1 << 6) | 0x3f;
	    instructions[i] = code;
            fwrite(&code, sizeof(code), 1, ostream);
            break;
        case RTI:
            code = instr.op->opcode;
	    instructions[i] = code;
            fwrite(&code, sizeof(code), 1, ostream);
            break;
        case ST:
            code |= instr.sr1 << 9;
            sym = getSymbol(&prog->symbolTable, instr.pcoffset9->name);
            if (!sym) {
                fprintf(stderr, "unbound symbol '%s'\n", instr.pcoffset9->name);
                exit(1);
            }
            code |= (sym->value - instr.lc - 1) & 0x1ff;
	    instructions[i] = code;
            fwrite(&code, sizeof(code), 1, ostream);
            break;
        case STI:
            code |= instr.sr1 << 9;
            sym = getSymbol(&prog->symbolTable, instr.pcoffset9->name);
            if (!sym) {
                fprintf(stderr, "unbound symbol '%s'\n", instr.pcoffset9->name);
                exit(1);
            }
            code |= (sym->value - instr.lc - 1) & 0x1ff;
	    instructions[i] = code;
            fwrite(&code, sizeof(code), 1, ostream);
            break;
        case STR:
            code |= (instr.sr1 << 9) | (instr.baser << 6) | instr.offset6;
	    instructions[i] = code;
            fwrite(&code, sizeof(code), 1, ostream);
            break;
        case TRAP:
            code |= instr.trapvect8;
	    instructions[i] = code;
            fwrite(&code, sizeof(code), 1, ostream);
            break;
        case GETC:
            fwrite(&instr.op->opcode, sizeof(instr.op->opcode), 1, ostream);
	    instructions[i] = instr.op->opcode;
	    break;
        case OUT:
            fwrite(&instr.op->opcode, sizeof(instr.op->opcode), 1, ostream);
	    instructions[i] = instr.op->opcode;
	    break;
        case PUTS:
            fwrite(&instr.op->opcode, sizeof(instr.op->opcode), 1, ostream);
	    instructions[i] = instr.op->opcode;
	    break;
        case IN:
            fwrite(&instr.op->opcode, sizeof(instr.op->opcode), 1, ostream);
	    instructions[i] = instr.op->opcode;
	    break;
        case PUTSP:
            fwrite(&instr.op->opcode, sizeof(instr.op->opcode), 1, ostream);
	    instructions[i] = instr.op->opcode;
	    break;
        case HALT:
            fwrite(&instr.op->opcode, sizeof(instr.op->opcode), 1, ostream);
	    instructions[i] = instr.op->opcode;
            break;
        case FILL:
        case BLKW:
        case STRINGZ:
            fwrite(&instr.val, sizeof(instr.val), 1, ostream);
	    instructions[i] = instr.val;
            break;
        case END:
            break;
        default:
            fprintf(stderr, "illegal opcode exception \\x%x\n",
                    instr.op->opcode);
            exit(1);
        }
    }
}
