#ifndef UTILS_H
#define UTILS_H

char
getch();

char
pathcmp(const char *a, const char *b);

int
get_parrent_path(const char *path);

int
to_block_count(int data_len, int lblk_sz);

#endif
