#include "common.h"
#include "operation.h"
#include "symbol.h"

#define MAX_NUM_INSTR 0xcfff
#define MAX_SYM_LEN 11
#define MAX_STR_LEN 48

typedef struct Instr {
    uint16_t lc;
    Operation *op;

    Symbol *label;
    Symbol *pcoffset9;

    uint16_t dr;
    uint16_t sr1;
    uint16_t sr2;
    uint16_t imm5;
    uint16_t val;
    uint16_t str[MAX_STR_LEN];

} Instr;

typedef struct Program {
    uint16_t lc;
    uint16_t size;
    Instr instructions[MAX_NUM_INSTR];

    Table symbolTable;
} Program;

void initProgram(Program *prog) {
    initTable(&prog->symbolTable);
    prog->lc = 0;
    prog->size = 0;
}

void deleteProgram(Program *prog) {
    deleteTable(&prog->symbolTable);
}

Instr *nextInstr(Program *prog) {
    if (prog->size == MAX_NUM_INSTR) {
        fprintf(stderr, "fatal: maximum instruction count reached\n");
        exit(1);
    }
    Instr *instr = &prog->instructions[prog->size++];
    instr->lc = prog->lc++;
    instr->label = NULL;
    instr->pcoffset9 = NULL;
    return instr;
}

Instr *curInstr(Program *prog) {
    return &prog->instructions[prog->lc];
}

typedef struct Parser {
    uint16_t line;
    FILE *stream;
    char cur;
    char peek;
} Parser;

void initParser(Parser *parser, FILE *stream) {
    parser->line = 1;
    parser->stream = stream;
    parser->cur = getc(stream);
    parser->peek = getc(stream);
}

void deleteParser(Parser *parser) {
    fclose(parser->stream);
}

uint16_t eof(Parser *parser) {
    return parser->peek == EOF;
}

char advance(Parser *parser) {
    parser->cur = parser->peek;
    if (parser->cur == '\n') {
        parser->line++;
    }
    parser->peek = getc(parser->stream);
    return parser->cur;
}

void skipComments(Parser *parser) {
    while (!eof(parser)) {
        if (parser->cur == ';') {
            while (parser->cur != '\n') {
                advance(parser);
            }
        } else if (isspace(parser->cur)) {
            advance(parser);
        } else {
            break;
        }
    }
}

void skipSpaces(Parser *parser) {
    while (parser->cur == ' ' || parser->cur == '\t') {
        advance(parser);
    }
}

void expect(Parser *parser, char c) {
    skipSpaces(parser);
    if (parser->cur != c) {
        fprintf(stderr, "expected '%c', got '%c'; line %d\n", c, parser->cur,
                parser->line);
        exit(1);
    }
    advance(parser);
}

uint16_t isdelim(char c) {
    return c == ';' || c == ',' || isspace(c);
}

uint16_t ishex(char c) {
    return (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F') || isdigit(c);
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
        advance(parser);
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
        advance(parser);
    }
    *bp = '\0';

    uint16_t opidx = findOperation(buf);
    if (opidx == 0xffff) {
        fprintf(stderr, "unknown operation '%s'; line %d\n", buf, parser->line);
        exit(1);
    }

    return opidx;
}

uint16_t parseRegister(Parser *parser) {
    skipSpaces(parser);
    uint16_t reg;

    if (parser->cur != 'R' && parser->cur != 'r') {
        fprintf(stderr, "expected register argument; line %d\n", parser->line);
    }

    advance(parser);
    if (!isdigit(parser->cur)) {
        fprintf(stderr, "invalid register literal; line %d\n", parser->line);
        exit(1);
    }

    reg = parser->cur - '0';
    advance(parser);

    return reg;
}

uint16_t parseLiteral(Parser *parser) {
    skipSpaces(parser);
    uint16_t val;

    val = 0;
    switch (parser->cur) {
    case '#':
        advance(parser);
        while (!eof(parser) && isdigit(parser->cur)) {
            val += parser->cur - '0';
            val *= 10;
            advance(parser);
        }
        break;
    case 'x':
        advance(parser);
        while (!eof(parser) && ishex(parser->cur)) {
            val <<= 4;
            val += parser->cur - '0';
            advance(parser);
        }
        break;
    default:
        fprintf(stderr, "invalid literal prefix '%c'; line %d\n", parser->cur,
                parser->line);
        exit(1);
    }

    if (!isdelim(parser->cur)) {
        fprintf(stderr, "invalid literal syntax; line %d\n", parser->line);
        exit(1);
    }

    return val;
}

uint16_t parseString(Parser *parser, uint16_t *str) {
    skipSpaces(parser);
    if (parser->cur != '"') {
        fprintf(stderr, "expected string literal argument; line %d\n",
                parser->line);
        exit(1);
    }
    advance(parser);

    uint16_t len = 0;
    while (!eof(parser) && parser->cur != '"' && len < MAX_STR_LEN) {
        str[len++] = (uint16_t)parser->cur;
        advance(parser);
    }
    str[len] = 0;

    if (parser->cur != '"') {
        fprintf(stderr, "unterminated string literal; line %d\n", parser->line);
        exit(1);
    }

    advance(parser);

    return len + 1;
}

Symbol *parseSymbol(Parser *parser, Program *prog) {
    skipSpaces(parser);
    char buf[MAX_SYM_LEN], *bp;

    bp = buf;
    while (!isdelim(parser->cur) && bp - buf < MAX_SYM_LEN) {
        *bp++ = parser->cur;
        advance(parser);
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

void parseDirective(Parser *parser, Program *prog, Instr *instr) {
    switch (instr->op->opcode) {
    case 0xfffe: /* .FILL */
        instr->val = parseLiteral(parser);
        break;
    case 0xfffd: /* .BLKW */
        prog->lc += parseLiteral(parser);
        break;
    case 0xfffc: /* .STRINGZ */
        prog->lc += parseString(parser, instr->str);
        break;
    case 0xfffb: /* .END */
        break;
    case 0xffff: /* .ORIG */
        fprintf(stderr, "unexpected .ORIG directive; line %d\n", parser->line);
        exit(1);
    }
}

void parseOperands(Parser *parser, Program *prog, Instr *instr) {
    skipSpaces(parser);

    switch (instr->op->opcode) {
    case ADD:
    case AND:
        instr->dr = parseRegister(parser);
        expect(parser, ',');
        instr->sr1 = parseRegister(parser);
        skipSpaces(parser);
        if (toupper(parser->cur) == 'R') {
            instr->sr2 = parseRegister(parser);
        } else {
            instr->imm5 = parseLiteral(parser);
        }
        break;
    case LEA:
        instr->dr = parseRegister(parser);
        expect(parser, ',');
        instr->pcoffset9 = parseSymbol(parser, prog);
        break;
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
    prog->lc = parseLiteral(parser);
    skipComments(parser);
}

void parseInstr(Parser *parser, Program *prog) {
    Instr *instr = nextInstr(prog);

    if (parseLabel(parser, prog, instr)) {
        uint16_t opidx = parseOp(parser);
        instr->op = &operations[opidx];
    }

    if (isdirective(instr->op)) {
        parseDirective(parser, prog, instr);
    } else if (isoperation(instr->op)) {
        parseOperands(parser, prog, instr);
    }

    skipComments(parser);
}

void parse(Parser *parser, Program *prog) {
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

void makeSymbolFile(Program *prog) {
    FILE *sf = fopen("symbol-table.txt", "w");
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

void printProgram(Program *prog) {
    for (int i = 0; i < prog->size; i++) {
        Instr instr = prog->instructions[i];
        printf("{\n");
        printf("\tlabel: %s\n", instr.label ? instr.label->name : "");
        printf("\tlc: \\x%x\n", instr.lc);
        printf("\tname: %s\n", instr.op->name);
        printf("\topcode: \\x%x\n", instr.op->opcode);
        printf("\tdr: \\x%x\n", instr.dr);
        printf("\tsr1: \\x%x\n", instr.sr1);
        printf("\tsr2: \\x%x\n", instr.sr2);
        printf("\timm5: \\x%x\n", instr.imm5);
        printf("\tpcoffset9: %s\n",
               instr.pcoffset9 ? instr.pcoffset9->name : "");
        printf("}\n");
    }
}

int main(void) {
    Parser parser;
    Program prog;
    FILE *stream;

    char *fn = "hello.asm";

    stream = fopen(fn, "r");
    if (!stream) {
        fprintf(stderr, "unable to open assembly file '%s'\n", fn);
        exit(1);
    }

    initParser(&parser, stream);
    initProgram(&prog);

    parse(&parser, &prog);
    printProgram(&prog);
    makeSymbolFile(&prog);

    deleteProgram(&prog);
    deleteParser(&parser);

    return 0;
}
