#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>

#include <debug.h>

char
getch() {
    struct termios oldt, newt;
    char ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

char
pathcmp(const char *a, const char *b) {
    if (a == NULL || b == NULL)
        return 0;
    DEBUG("a & b is not NULL");

    int la = strlen(a);
    int lb = strlen(b);
    if(la == 0 || lb == 0 || la > lb)
        return 0;
    DEBUG("strlen a: %d b: %d", la, lb);

    for(size_t i = 0; i < la; ++i) {
        if(a[i] != b[i])
            return 0;
    }

    return 1;
}
