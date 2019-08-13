#include "asm.h"
#include "parse.h"

void assemble(Program *prog, FILE *ostream) {
    Symbol *sym;
    uint16_t code = 0;
    uint16_t i;
    Instr instr;

    /* write origin to file */
    fwrite(&prog->orig, sizeof(prog->orig), 1, ostream);

    for (i = 0; i < prog->lc; i++) {
	instr = prog->instructions[i];

        switch (instr.op->opcode) {
        case ADD:
        case AND:
            code |=
                (instr.op->opcode << 12) | (instr.dr << 9) | (instr.sr1 << 6);
            if (instr.sr2 == 0xffff) {
                code |= instr.imm5 & 0x1f;
            } else {
                code |= instr.sr2;
            }
	    fwrite(&code, sizeof(code), 1, ostream);
            break;
        case BR:
            code |= (instr.op->nzp << 12) | (instr.op->nzp << 9);
            sym = getSymbol(&prog->symbolTable, instr.pcoffset9->name);
            if (!sym) {
                fprintf(stderr, "unbound symbol '%s'\n", instr.pcoffset9->name);
                exit(1);
            }
            code |= sym->value & 0x1ff;
	    fwrite(&code, sizeof(code), 1, ostream);
            break;
        case JMP:
            code |= (instr.op->opcode << 12) | (instr.baser << 6);
	    fwrite(&code, sizeof(code), 1, ostream);
            break;
        case JSR:
            code |= instr.op->opcode << 12;
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
	    fwrite(&code, sizeof(code), 1, ostream);
            break;
        case LD:
        case LDI:
        case LEA:
            code |= (instr.op->opcode << 12) | (instr.dr << 9);
            sym = getSymbol(&prog->symbolTable, instr.pcoffset9->name);
            if (!sym) {
                fprintf(stderr, "unbound symbol '%s'\n", instr.pcoffset9->name);
                exit(1);
            }
            code |= (sym->value - instr.lc - 1) & 0x1ff;
	    fwrite(&code, sizeof(code), 1, ostream);
            break;
        case LDR:
            code |= (instr.op->opcode << 12) | (instr.dr << 9) |
                    (instr.baser << 6) | instr.offset6;
	    fwrite(&code, sizeof(code), 1, ostream);
            break;
        case NOT:
            code |= (instr.op->opcode << 12) | (instr.dr << 9) |
                    (instr.sr1 << 6) | 0x3f;
	    fwrite(&code, sizeof(code), 1, ostream);
            break;
        case RTI:
            break;
        case ST:
        case STI:
            code |= (instr.op->opcode << 12) | (instr.sr1 << 9);
            sym = getSymbol(&prog->symbolTable, instr.pcoffset9->name);
            if (!sym) {
                fprintf(stderr, "unbound symbol '%s'\n", instr.pcoffset9->name);
                exit(1);
            }
            code |= (sym->value - instr.lc - 1) & 0x1ff;
	    fwrite(&code, sizeof(code), 1, ostream);
            break;
        case STR:
            code |= (instr.op->opcode << 12) | (instr.sr1 << 9) |
                    (instr.baser << 6) | instr.offset6;
	    fwrite(&code, sizeof(code), 1, ostream);
            break;
        case TRAP:
            code |= (instr.op->opcode << 12) | instr.trapvect8;
	    fwrite(&code, sizeof(code), 1, ostream);
            break;
        case GETC:
        case OUT:
        case PUTS:
        case IN:
        case PUTSP:
        case HALT:
	    fwrite(&instr.op->opcode, sizeof(instr.op->opcode), 1, ostream);
            break;
        case FILL:
        case BLKW:
        case STRINGZ:
	    fwrite(&instr.val, sizeof(instr.val), 1, ostream);
	    break;
	   break;
        case END:
            break;
        default:
            fprintf(stderr, "illegal opcode exception \\x%x\n",
                    instr.op->opcode);
            exit(1);
        }

        /* fwrite(&code, sizeof(code), 1, ostream); */
    }
}

