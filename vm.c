#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "error.h"
#include "instr.h"

/* Data and Code sections are 256 KiB, addressable by a 16 bit int */
#define SECTION_SIZE 65536

/* Code section */
uint32_t code[SECTION_SIZE];

/* Data section */
int32_t data[SECTION_SIZE];

/*
 * Stack pointer
 * The stack grows up from the bottom of the data section.
 */
int32_t *sp = &data[SECTION_SIZE - 1];

/* Push a value onto the stack */
void push(uint32_t);

/* Pop a value from the stack */
void pop(void);

/* Load an object file into memory from input file descriptor. */
void load(int);

/* Interpret the object file */
void run(void);

/* Sign extend n */
uint32_t sext(uint16_t n);

int main(int argc, char **argv)
{
    int ifd;

    if (argc == 1)
    {
        panic("wrong number of arguments");
    }

    ifd = open(argv[1], O_RDONLY);
    if (ifd == -1)
    {
        panic("unable to open %s for reading", argv[1]);
    }

    load(ifd);
    run();
    close(ifd);

    return 0;
}

/* Read from input file descriptor in big endian order, panicking on fail */
int read_from(int ifd, uint32_t *val, size_t nbytes)
{
    int bytes_read = 0;

    if ((bytes_read = read(ifd, val, nbytes)) == -1)
    {
        panic("read_from: unable to read from input file descriptor");
    }

    *val = ntohl(*val); /* Ensure machine endianness */

    return bytes_read;
}

void load(int ifd)
{
    int offset;
#ifdef XTRA
    unsigned i;
    uint32_t ndecl, decl;

    /* Read first line of object file, number of bytes in data */
    read_from(ifd, &ndecl, INSTR_WIDTH);

    /* Load static data */
    offset = 0;
    for (i = 0; i < ndecl; ++i)
    {
        read_from(ifd, &decl, INSTR_WIDTH);
        data[offset++] = decl;
    }
#endif

    /* Load code */
    offset = 0;
    while (read_from(ifd, &code[offset++], INSTR_WIDTH))
    {
    }
}

void run(void)
{
    /* Program counter */
    uint32_t pc = 0;

    while (1)
    {
        /* Fetch next instruction and increment pc */
        uint32_t instr = code[pc++];

        /* Opcode is stored in bits 20-16, so right shift should get it */
        switch (instr >> 16)
        {
        case HALT:
            return;
        case PUSH:
            push(sext(instr & 0xffff));
            break;
        case RVALUE:
            push(data[sext(instr & 0xffff)]);
            break;
        case LVALUE:
            push(sext(instr & 0xffff));
            break;
        case POP:
            pop();
            break;
        case STO:
        {
            int32_t rvalue = *sp;
            pop();
            data[*sp] = rvalue;
            pop();
        }
        break;
        case COPY:
            push(*sp);
            break;
        case ADD:
        {
            int32_t b = *sp;
            pop();
            *sp += b;
        }
        break;
        case SUB:
        {
            int32_t b = *sp;
            pop();
            *sp -= b;
        }
        break;
        case MPY:
        {
            int32_t b = *sp;
            pop();
            *sp *= b;
        }
        break;
        case DIV:
        {
            int32_t b = *sp;
            if (b == 0)
            {
                panic("divizion by zero");
            }
            pop();
            *sp /= b;
        }
        break;
        case MOD:
        {
            int32_t b = *sp;
            pop();
            *sp %= b;
        }
        break;
        case NEG:
        {
            int32_t b = *sp;
            pop();
            push(~b + 0x1);
        }
        break;
        case NOT:
            *sp = ~*sp;
            break;
        case AND:
        {
            int32_t b = *sp;
            pop();
            *sp &= b;
        }
        break;
        case OR:
        {
            int32_t b = *sp;
            pop();
            *sp |= b;
        }
        break;
        case EQ:
        {
            int32_t b = *sp;
            pop();
            *sp = *sp == b;
        }
        break;
        case NE:
        {
            int32_t b = *sp;
            pop();
            *sp = *sp != b;
        }
        break;
        case GT:
        {
            int32_t b = *sp;
            pop();
            *sp = *sp > b;
        }
        break;
        case GE:
        {
            int32_t b = *sp;
            pop();
            *sp = *sp >= b;
        }
        case LT:
        {
            int32_t b = *sp;
            pop();
            *sp = *sp < b;
        }
        break;
        case LE:
        {
            uint32_t b = *sp;
            pop();
            *sp = *sp <= b;
        }
        break;
        case LABEL:
            break;
        case GOTO:
            pc = sext(instr & 0xffff);
            break;
        case GOFALSE:
            pc = *sp ? pc : sext(instr & 0xffff);
            pop();
            break;
        case GOTRUE:
            pc = !*sp ? pc : sext(instr & 0xffff);
            pop();
            break;
        case PRINT:
            printf("%d\n", (int)*sp);
            pop();
            break;
        case READ:
        {
            int32_t n;
            /* TODO: Handle overflow */
            printf("Enter a decimal number: ");
            scanf("%d", &n);
            push(n);
        }
        break;
        case GOSUB:
            push(pc);
            pc = sext(instr & 0xffff);
            break;
        case RET:
            pc = *sp;
            pop();
            break;
        case PUTS:
            printf("%s", (char *)&data[*sp]);
            pop();
            break;
        case DREF:
        {
            uint32_t p = *sp;
            pop();
            push(data[p]);
        }
        break;
        default:
            panic("unknown opcode 0x%x", instr >> 16);
        }
    }
}

void push(uint32_t v)
{
    if (sp < &data[SECTION_SIZE / 2])
    {
        panic("stack overflow");
    }
    *(--sp) = v;
}

void pop(void)
{
    if (sp >= &data[SECTION_SIZE])
    {
        panic("stack underflow");
    }
    ++sp;
}

uint32_t sext(uint16_t n)
{
    if (n & 0x8000)
    {
        return n | 0xffff0000;
    }
    return n;
}
