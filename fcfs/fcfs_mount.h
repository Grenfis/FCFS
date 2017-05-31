#ifndef FCFS_MOUNT_H
#define FCFS_MOUNT_H

#include "fcfs.h"
#include <fcfs_structs.h>

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

#endif
