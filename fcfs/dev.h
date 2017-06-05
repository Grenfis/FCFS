#ifndef DEV_H
#define DEV_H

#include "fcfs.h"
#include <fcfs_structs.h>

int
dev_mount(fcfs_args_t *args);

//id - cluster id
fcfs_block_list_t *
dev_read_ctable(fcfs_args_t *args, int id);

//cid - cluster id
//bid - id of block of file in cluster
char *
dev_read_block(fcfs_args_t *args, int cid, int bid);

fcfs_dir_entry_t *
dev_read_dir(fcfs_args_t *args, int fid, int *ret_sz);

int
dev_write_dir(fcfs_args_t *args, int fid, fcfs_dir_entry_t *ent, int len);

int
dev_free_fid(fcfs_args_t *args);

int
dev_write_head(fcfs_args_t *args);

int
dev_write_bitmap(fcfs_args_t *args);

int
dev_write_table(fcfs_args_t *args);

int
dev_file_alloc(fcfs_args_t *args, int fid);

int
dev_init_dir(fcfs_args_t *args, int fid);

int
dev_file_size(fcfs_args_t *args, int fid);

int
dev_rm_file(fcfs_args_t *args, int fid);

int
dev_rm_from_dir(fcfs_args_t *args, int fid, int del_id);

char
dev_upd_bitmap(fcfs_args_t *args, fcfs_block_list_t *bl, int cid);

int *
dev_get_blocks(fcfs_block_list_t *blist, int fid, int *ret_sz);

int
dev_free_cluster(fcfs_args_t *args);

int
dev_write_block(fcfs_args_t *args, int cid, int bid, char *data, int len);

#endif
