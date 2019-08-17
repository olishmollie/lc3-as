#include "parse.h"

#define MAX_STR_LEN 48

void initProgram(Program *prog) {
    initTable(&prog->symbolTable);
    prog->orig = 0;
    prog->lc = 0;
}

void deleteProgram(Program *prog) {
    deleteTable(&prog->symbolTable);
    free(prog);
}

Instr *nextInstr(Program *prog) {
    if (prog->lc == MAX_NUM_INSTR) {
        fprintf(stderr, "fatal: maximum instruction count reached\n");
        exit(1);
    }
    Instr *instr = &prog->instructions[prog->lc];
    instr->lc = prog->lc++;

    instr->label = NULL;
    instr->op = NULL;
    instr->pcoffset9 = NULL;
    instr->pcoffset11 = NULL;

    instr->dr = 0xffff;
    instr->sr1 = 0xffff;
    instr->sr2 = 0xffff;
    instr->imm5 = 0xffff;
    instr->offset6 = 0xffff;
    instr->baser = 0xffff;
    instr->trapvect8 = 0xffff;

    return instr;
}

Instr *curInstr(Program *prog) {
    return &prog->instructions[prog->lc];
}

typedef struct Parser {
    uint16_t line;
    FILE *istream;
    char cur;
} Parser;

void initParser(Parser *parser, FILE *istream) {
    parser->line = 1;
    parser->istream = istream;
    parser->cur = toupper(getc(istream));
}

void deleteParser(Parser *parser) {
    fclose(parser->istream);
}

uint16_t eof(Parser *parser) {
    return parser->cur == EOF;
}

char advance(Parser *parser, int ci) {
    parser->cur = ci ? toupper(getc(parser->istream)) : getc(parser->istream);
    if (parser->cur == '\n') {
        parser->line++;
    }
    return parser->cur;
}

void skipComments(Parser *parser) {
    while (!eof(parser)) {
        if (parser->cur == ';') {
            while (parser->cur != '\n') {
                advance(parser, 1);
            }
        } else if (isspace(parser->cur)) {
            advance(parser, 1);
        } else {
            break;
        }
    }
}

void skipSpaces(Parser *parser) {
    while (parser->cur == ' ' || parser->cur == '\t') {
        advance(parser, 1);
    }
}

void expect(Parser *parser, char c) {
    skipSpaces(parser);
    if (parser->cur != c) {
        fprintf(stderr, "expected '%c', got '%c'; line %d\n", c, parser->cur,
                parser->line);
        exit(1);
    }
    advance(parser, 1);
}

uint16_t isdelim(char c) {
    return c == ';' || c == ',' || isspace(c);
}

uint16_t ishex(char c) {
    return (c >= 'A' && c <= 'F') || isdigit(c);
}

uint16_t hexval(char c) {
    uint16_t val;
    if (isdigit(c)) {
	return c - '0';
    }
    switch (c) {
    case 'A':
	val = 10;
	break;
    case 'B':
	val = 11;
	break;
    case 'C':
	val = 12;
	break;
    case 'D':
	val = 13;
	break;
    case 'E':
	val = 14;
	break;
    case 'F':
	val = 15;
    }
    return val;
}

uint16_t isdirective(Operation *op) {
    return op->opcode >= 0xfffb && op->opcode <= 0xffff;
}

uint16_t isoperation(Operation *op) {
    return op->opcode >= 0 && op->opcode <= 0xe;
}

Symbol *parseLabel(Parser *parser, Program *prog, Instr *instr) {
    skipSpaces(parser);
    char buf[MAX_SYM_LEN], *bp;
    uint16_t opidx;
    Symbol *symbol;

    bp = buf;
    while (!isdelim(parser->cur) && bp - buf < MAX_SYM_LEN - 1) {
        *bp++ = parser->cur;
        advance(parser, 1);
    }
    *bp = '\0';

    if (bp - buf == 0) {
        fprintf(stderr, "expected symbol; line %d\n", parser->line);
        exit(1);
    }

    opidx = findOperation(buf);
    if (opidx != 0xffff) {
        instr->op = &operations[opidx];
        return NULL;
    }

    symbol = newSymbol(&prog->symbolTable, buf, instr->lc);
    instr->label = symbol;

    return symbol;
}

uint16_t parseOp(Parser *parser) {
    skipSpaces(parser);
    char buf[MAX_SYM_LEN], *bp;

    bp = buf;
    while (!isdelim(parser->cur)) {
        *bp++ = parser->cur;
        advance(parser, 1);
    }
    *bp = '\0';

    if (bp - buf == 0) {
        fprintf(stderr, "expected operation; line %d\n", parser->line);
        exit(1);
    }

    uint16_t opidx = findOperation(buf);
    if (opidx == 0xffff) {
        fprintf(stderr, "unknown operation '%s'; line %d\n", buf, parser->line);
        exit(1);
    }

    return opidx;
}

uint16_t parseRegister(Parser *parser) {
    skipSpaces(parser);
    char buf[MAX_STR_LEN], *bp;
    uint16_t nargs, reg;

    bp = buf;
    while (!eof(parser) && !isdelim(parser->cur)) {
        *bp++ = parser->cur;
        advance(parser, 1);
    }
    *bp = '\0';

    nargs = sscanf(buf, "R%hu", &reg);
    if (nargs != 1) {
        fprintf(stderr, "invalid register specification: '%s'; line %d\n", buf,
                parser->line);
        exit(1);
    }

    return reg;
}

uint16_t parseLiteral(Parser *parser) {
    skipSpaces(parser);
    uint16_t val, neg = 0;

    val = 0;
    switch (parser->cur) {
    case '#':
        advance(parser, 1);
        if (parser->cur == '-') {
            neg = 1;
            advance(parser, 1);
        }
        while (!eof(parser) && isdigit(parser->cur)) {
            val *= 10;
            val += parser->cur - '0';
            advance(parser, 1);
        }
        if (neg) {
            val *= -1;
        }
        break;
    case 'X':
        advance(parser, 1);
        while (!eof(parser) && ishex(parser->cur)) {
            val <<= 4;
            val += hexval(parser->cur);
            advance(parser, 1);
        }
        break;
    default:
        fprintf(stderr, "invalid literal prefix '%c'; line %d\n", parser->cur,
                parser->line);
        exit(1);
    }

    if (!isdelim(parser->cur)) {
        fprintf(stderr, "invalid literal syntax '%c'; line %d\n", parser->cur,
                parser->line);
        exit(1);
    }

    return val;
}

uint16_t parseString(Parser *parser, Program *prog, Instr *instr) {
    skipSpaces(parser);
    if (parser->cur != '"') {
        fprintf(stderr, "expected string literal argument; line %d\n",
                parser->line);
        exit(1);
    }
    advance(parser, 0);

    uint16_t len;
    Operation *stringz = instr->op;

    len = 0;
    while (!eof(parser) && parser->cur != '"' && len < MAX_STR_LEN) {
        instr->val = (uint16_t)parser->cur;
        advance(parser, 0);
        instr = nextInstr(prog);
        instr->op = stringz;
    }
    instr->val = 0;

    if (parser->cur != '"') {
        fprintf(stderr, "unterminated string literal; line %d\n", parser->line);
        exit(1);
    }

    advance(parser, 1);

    return len;
}

Symbol *parseSymbol(Parser *parser, Program *prog) {
    skipSpaces(parser);
    char buf[MAX_SYM_LEN], *bp;

    bp = buf;
    while (!isdelim(parser->cur) && bp - buf < MAX_SYM_LEN) {
        *bp++ = parser->cur;
        advance(parser, 1);
    }
    *bp = '\0';

    if (bp - buf == 0) {
        fprintf(stderr, "expected symbol; line %d\n", parser->line);
        exit(1);
    }

    if (findOperation(buf) != 0xffff) {
        fprintf(stderr, "unexpected operation '%s'; line %d\n", buf,
                parser->line);
        exit(1);
    }

    return newSymbol(&prog->symbolTable, buf, -1);
}

void parseOperands(Parser *parser, Program *prog, Instr *instr) {
    skipSpaces(parser);

    switch (instr->op->opcode) {
    case ADD:
    case AND:
        instr->dr = parseRegister(parser);
        expect(parser, ',');
        instr->sr1 = parseRegister(parser);
        expect(parser, ',');
        skipSpaces(parser);
        if (parser->cur == 'R') {
            instr->sr2 = parseRegister(parser);
        } else {
            instr->imm5 = parseLiteral(parser);
        }
        break;
    case BR:
        instr->pcoffset9 = parseSymbol(parser, prog);
        break;
    case JMP:
	if (instr->op->ret) {
	    instr->baser = 0x7;
	} else {
	    instr->baser = parseRegister(parser);
	}
        break;
    case JSR:
        skipSpaces(parser);
        if (instr->op->reg) {
            instr->baser = parseRegister(parser);
        } else {
            instr->pcoffset11 = parseSymbol(parser, prog);
        }
        break;
    case LD:
    case LDI:
    case LEA:
        instr->dr = parseRegister(parser);
        expect(parser, ',');
        instr->pcoffset9 = parseSymbol(parser, prog);
        break;
    case LDR:
        instr->dr = parseRegister(parser);
        expect(parser, ',');
        instr->baser = parseRegister(parser);
        expect(parser, ',');
        instr->offset6 = parseLiteral(parser);
        break;
    case NOT:
        instr->dr = parseRegister(parser);
        expect(parser, ',');
        instr->sr1 = parseRegister(parser);
        break;
    case RTI:
        break;
    case ST:
    case STI:
        instr->sr1 = parseRegister(parser);
        expect(parser, ',');
        instr->pcoffset9 = parseSymbol(parser, prog);
        break;
    case STR:
        instr->sr1 = parseRegister(parser);
        expect(parser, ',');
        instr->baser = parseRegister(parser);
        expect(parser, ',');
        instr->offset6 = parseLiteral(parser);
        break;
    case TRAP:
        instr->trapvect8 = parseLiteral(parser);
        break;
    case GETC:
    case OUT:
    case PUTS:
    case IN:
    case HALT:
        break;
    case FILL:
        instr->val = parseLiteral(parser);
        break;
    case BLKW: {
        uint16_t val = parseLiteral(parser) - 1;
        Operation *blkw = instr->op;
        while (val--) {
            instr->val = 0;
            instr = nextInstr(prog);
            instr->op = blkw;
        }
        break;
    }
    case STRINGZ:
        parseString(parser, prog, instr);
        break;
    case END:
        break;
    case ORIG:
        fprintf(stderr, "unexpected .ORIG directive; line %d\n", parser->line);
        exit(1);
    default:
        fprintf(stderr, "illegal opcode exception \\x%x; line %d\n",
                instr->op->opcode, parser->line);
    }
}

void parseOrigin(Parser *parser, Program *prog) {
    skipSpaces(parser);
    uint16_t opidx;

    opidx = parseOp(parser);
    if (operations[opidx].opcode != 0xffff) {
        fprintf(stderr,
                "all lc-3 programs must begin with .ORIG directive; line %d\n",
                parser->line);
        exit(1);
    }
    prog->orig = parseLiteral(parser);
    skipComments(parser);
}

void parseInstr(Parser *parser, Program *prog) {
    Instr *instr = nextInstr(prog);

    if (parseLabel(parser, prog, instr)) {
        uint16_t opidx = parseOp(parser);
        instr->op = &operations[opidx];
    }

    parseOperands(parser, prog, instr);

    skipComments(parser);
}

void parseProgram(Parser *parser, Program *prog) {
    skipComments(parser);
    parseOrigin(parser, prog);

    while (!eof(parser) && prog->lc < MAX_NUM_INSTR) {
        parseInstr(parser, prog);
    }

    /* TODO - enforce last instruction be .END directive */
}

int fwriteSymbol(Symbol *symbol, FILE *sf) {
    if (symbol->next) {
        if (!fwriteSymbol(symbol->next, sf)) {
            return 0;
        }
    }
    if (fprintf(sf, "{\n\tname: %s\n\tvalue: \\x%x\n}\n", symbol->name,
                symbol->value) > 0) {
        return 1;
    }
    return 0;
}

void makeSymbolFile(char *fname, Program *prog) {
    /* TODO - munge strings to make proper symbol file name */
    FILE *sf = fopen("table.txt", "w");
    if (!sf) {
        fprintf(stderr, "unable to open symbol file\n");
        return;
    }

    fprintf(sf, "Symbol Table:\n");

    int i;
    for (i = 0; i < prog->symbolTable.capacity; i++) {
        Symbol *symbol = prog->symbolTable.buckets[i];
        if (symbol) {
            fwriteSymbol(symbol, sf);
        }
    }

    fclose(sf);
}

/* 
 * PrintProgram prints a JSON like representation of instructions 
 * to stdout; it is useful for debugging.
*/
void printProgram(Program *prog) {
    for (int i = 0; i < prog->lc; i++) {
        Instr instr = prog->instructions[i];
        printf("{\n");
        printf("\tlabel: %s\n", instr.label ? instr.label->name : "");
        printf("\tlc: \\x%x\n", instr.lc);
        printf("\tname: %s\n", instr.op ? instr.op->name : "");
        printf("\topcode: \\x%x\n", instr.op ? instr.op->opcode : 0xffff);
	printf("\tnzp: \\x%x\n", instr.op ? instr.op->nzp : 0xffff);
        printf("\tdr: \\x%x\n", instr.dr);
        printf("\tsr1: \\x%x\n", instr.sr1);
        printf("\tsr2: \\x%x\n", instr.sr2);
        printf("\timm5: \\x%x\n", instr.imm5);
        printf("\tval: \\x%x\n", instr.val);
        printf("\tpcoffset9: %s\n",
               instr.pcoffset9 ? instr.pcoffset9->name : "");
        printf("}\n");
    }
}

Program *parse(FILE *istream) {
    Parser parser;
    Program *prog;

    ALLOC(&prog, 1);

    initParser(&parser, istream);
    initProgram(prog);

    parseProgram(&parser, prog);

    deleteParser(&parser);

    return prog;
}
