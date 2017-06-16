#ifndef FCFS_H
#define FCFS_H

#include <fcfs_structs.h>
#include <stdio.h>
#include <gcrypt.h>

typedef struct fcfs_args {
    char            *p_dev;      //device
    gcry_cipher_hd_t ciph;
    FILE            *dev;
    fcfs_head_t     *fs_head;
    unsigned char   *fs_bitmap;
    fcfs_table_t    *fs_table;
} fcfs_args_t;

#endif
