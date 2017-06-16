#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>

#ifdef DBG

#define DEBUG(__X, ...) fprintf(stdout, "\x1b[34;1m[DEBUG] %s:%s:%lu > " __X "\x1b[0m\n", __FILENAME__, __func__, (unsigned long)__LINE__, ##__VA_ARGS__)
#define ERROR(__X, ...) fprintf(stderr, "\x1b[31;1m[ERROR] %s:%s:%lu > " __X "\x1b[0m\n", __FILENAME__, __func__, (unsigned long)__LINE__, ##__VA_ARGS__)

#else

#define DEBUG(__X, ...)
#define ERROR(__X, ...)

#endif

#define die(_X, ...) {printf("[DIE]" _X, ##__VA_ARGS__); exit(-1);}

#endif
