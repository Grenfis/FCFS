#ifndef FCFS_H
#define FCFS_H

#include <fcfs_structs.h>
#include <stdio.h>

typedef struct fcfs_args {
    char            *p_dev;      //device
    char            *passwd;     //password
    FILE            *dev;
    fcfs_head_t     *fs_head;
    unsigned char   *fs_bitmap;
    fcfs_table_t    *fs_table;
} fcfs_args_t;

#endif
