#include "asm.h"
#include "parse.h"

void assemble(Program *prog, FILE *ostream) {
    Symbol *sym;
    uint16_t i, code;
    Instr instr;

    /* write origin to file */
    fwrite(&prog->orig, sizeof(prog->orig), 1, ostream);

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
            fwrite(&code, sizeof(code), 1, ostream);
            printf("ADD code = \\x%x\n", code);
            break;
        case AND:
            code |= (instr.dr << 9) | (instr.sr1 << 6);
            if (instr.sr2 == 0xffff) {
                code |= (1 << 5) | (instr.imm5 & 0x1f);
            } else {
                code |= instr.sr2;
            }
            printf("AND code = \\x%x\n", code);
            fwrite(&code, sizeof(code), 1, ostream);
            break;
        case BR:
            code |= (instr.op->nzp << 9);
            sym = getSymbol(&prog->symbolTable, instr.pcoffset9->name);
            if (!sym) {
                fprintf(stderr, "unbound symbol '%s'\n", instr.pcoffset9->name);
                exit(1);
            }
            code |= (sym->value - instr.lc - 1) & 0x1ff;
            printf("BR code = \\x%x\n", code);
            fwrite(&code, sizeof(code), 1, ostream);
            break;
        case JMP:
            code |= instr.baser << 6;
            printf("JMP code = \\x%x\n", code);
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
            printf("JSR code = \\x%x\n", code);
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
            printf("LD code = \\x%x\n", code);
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
            printf("LDI code = \\x%x\n", code);
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
            printf("LEA code = \\x%x\n", code);
            fwrite(&code, sizeof(code), 1, ostream);
            break;
        case LDR:
            code |= (instr.dr << 9) | (instr.baser << 6) | instr.offset6;
            printf("LDR code = \\x%x\n", code);
            fwrite(&code, sizeof(code), 1, ostream);
            break;
        case NOT:
            code |= (instr.dr << 9) | (instr.sr1 << 6) | 0x3f;
            printf("NOT code = \\x%x\n", code);
            fwrite(&code, sizeof(code), 1, ostream);
            break;
        case RTI:
            code = instr.op->opcode;
            printf("RTI code = \\x%x\n", code);
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
            printf("ST code = \\x%x\n", code);
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
            printf("STI code = \\x%x\n", code);
            fwrite(&code, sizeof(code), 1, ostream);
            break;
        case STR:
            code |= (instr.sr1 << 9) | (instr.baser << 6) | instr.offset6;
	    printf("STR code = \\x%x\n", code);
            fwrite(&code, sizeof(code), 1, ostream);
            break;
        case TRAP:
            code |= instr.trapvect8;
	    printf("TRAP code = \\x%x\n", code);
            fwrite(&code, sizeof(code), 1, ostream);
            break;
        case GETC:
	    printf("GETC code = \\x%x\n", instr.op->opcode);
            fwrite(&instr.op->opcode, sizeof(instr.op->opcode), 1, ostream);
	    break;
        case OUT:
	    printf("OUT code = \\x%x\n", instr.op->opcode);
            fwrite(&instr.op->opcode, sizeof(instr.op->opcode), 1, ostream);
	    break;
        case PUTS:
	    printf("PUTS code = \\x%x\n", instr.op->opcode);
            fwrite(&instr.op->opcode, sizeof(instr.op->opcode), 1, ostream);
	    break;
        case IN:
	    printf("IN code = \\x%x\n", instr.op->opcode);
            fwrite(&instr.op->opcode, sizeof(instr.op->opcode), 1, ostream);
	    break;
        case PUTSP:
	    printf("PUTSP code = \\x%x\n", instr.op->opcode);
            fwrite(&instr.op->opcode, sizeof(instr.op->opcode), 1, ostream);
	    break;
        case HALT:
	    printf("HALT code = \\x%x\n", instr.op->opcode);
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
    }
}
