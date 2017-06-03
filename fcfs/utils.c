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

    int bl = strlen(b);
    while(b[bl] != '/')
        bl--;

    char *tmp = calloc(1, bl);
    memcpy(tmp, b, bl);

    if(strcmp(a, tmp) != 0)
        return 0;

    DEBUG("ok %s %s", a, tmp);
    free(tmp);
    return 1;
}

int
get_parrent_path(const char *path) {
    int p_len = strlen(path);
    while(path[p_len] != '/')
        p_len--;
    return p_len;
}
