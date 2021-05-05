#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void panic(char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    printf("fatal: ");
    vprintf(fmt, ap);
    printf("\n");
    va_end(ap);
    exit(1);
}
