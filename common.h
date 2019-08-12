#ifndef COMMON_H
#define COMMON_H

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ALLOC(p, n)                                                            \
    {                                                                          \
        *p = calloc((n), sizeof(**p));                                         \
        if (*p == NULL) {                                                      \
            fprintf(stderr, "out of memory");                                  \
            exit(1);                                                           \
        }                                                                      \
    }

#endif
