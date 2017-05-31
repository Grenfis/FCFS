#ifndef DEBUG_H
#define DEBUG_H

#ifdef DBG

#include <stdio.h>

#define DEBUG(__X, ...) fprintf(stdout, "[DEBUG] %s:%s:%lu > " __X "\n", __FILENAME__, __func__, (unsigned long)__LINE__, ##__VA_ARGS__)
#define ERROR(__X, ...) fprintf(stderr, "[ERROR] %s:%s:%lu > " __X "\n", __FILENAME__, __func__, (unsigned long)__LINE__, ##__VA_ARGS__)

#else

#define DEBUG(__X, ...)
#define ERROR(__X, ...)

#endif

#endif
