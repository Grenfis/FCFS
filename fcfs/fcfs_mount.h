#ifndef FCFS_MOUNT_H
#define FCFS_MOUNT_H

#include "fcfs.h"
#include <fcfs_structs.h>
#include <math.h>

int
fcfs_mount(fcfs_args_t *args);

//id - cluster id
fcfs_block_list_t *
fcfs_get_claster_table(fcfs_args_t *args, int id);

//cid - cluster id
//bid - id of block of file in cluster
char *
fcfs_read_block(fcfs_args_t *args, int cid, int bid);

fcfs_dir_entry_t *
fcfs_read_directory(fcfs_args_t *args, int fid, int *ret_sz);

int
fcfs_write_directory(fcfs_args_t *args, int fid, fcfs_dir_entry_t *ent, int len);

int
fcfs_get_free_fid(fcfs_args_t *args);

int
fcfs_write_head(fcfs_args_t *args);

int
fcfs_write_bitmap(fcfs_args_t *args);

int
fcfs_write_table(fcfs_args_t *args);

int
fcfs_alloc(fcfs_args_t *args, int fid);

int
fcfs_init_dir(fcfs_args_t *args, int fid);

int
fcfs_get_file_size(fcfs_args_t *args, int fid);

static inline int
to_block_count(int data_len, int lblk_sz) {
    return ceil(data_len / (double)lblk_sz);
}

#endif
