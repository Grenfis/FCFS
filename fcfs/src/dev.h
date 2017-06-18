#ifndef DEV_H
#define DEV_H

#include "fcfs.h"
#include <fcfs_structs.h>
#include <sys/stat.h>
#include <sys/types.h>

enum {
    NONEED = 0,
    NEED = 1
};

typedef struct dev_blk_info {
    unsigned cid;
    unsigned char bid;
    unsigned num;
    struct dev_blk_info *next;
} dev_blk_info_t;

void
dev_destr_blk_info(dev_blk_info_t *i);

int
dev_mount(fcfs_args_t *args);

//id - cluster id
fcfs_block_list_t *
dev_read_ctable(fcfs_args_t *args, int id);

int
dev_write_ctable(fcfs_args_t *args, int id, fcfs_block_list_t *bl);

//cid - cluster id
//bid - id of block of file in cluster
char *
dev_read_block(fcfs_args_t *args, int cid, int bid, unsigned char need_crypt);

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
dev_init_file(fcfs_args_t *args, int fid);

int
dev_file_size(fcfs_args_t *args, int fid);

int
dev_rm_file(fcfs_args_t *args, int fid, int pfid);

int
dev_rm_from_dir(fcfs_args_t *args, int fid, int del_id);

char
dev_upd_bitmap(fcfs_args_t *args, fcfs_block_list_t *bl, int cid);

int *
dev_get_blocks(fcfs_block_list_t *blist, int fid, int *ret_sz);

int
dev_free_cluster(fcfs_args_t *args);

int
dev_write_block(fcfs_args_t *args, int cid, int bid, char *data, int len, unsigned char need_crypt);

int
dev_create_file(fcfs_args_t *args, int pfid, int fid, const char *name, mode_t mode);

dev_blk_info_t *
dev_get_file_seq(fcfs_args_t *args, int fid, int *size);

dev_blk_info_t *
dev_free_blocks(fcfs_args_t *args, int count, int *size);

int
dev_del_block(fcfs_args_t *agrs, int fid, int cid, int bid);

int
dev_read_by_id(fcfs_args_t *args, int fid, int id, char *buf, int lblk_sz,
    dev_blk_info_t *inf, int seq_sz);

int
dev_write_by_id(fcfs_args_t *args, int fid, int id, char *buf,
    int lblk_sz, dev_blk_info_t *inf, int seq_sz);

int
dev_file_reserve(fcfs_args_t *args, int fid, dev_blk_info_t *inf, int seq_sz, int last_num);

int
dev_set_file_size(fcfs_args_t *args, int fid, int size);

int
dev_extd_blk_list(fcfs_args_t *args, dev_blk_info_t **inf, int seq_sz,int count, int fid);

int
dev_free_cluster_from(fcfs_args_t *args, int cid);

int
dev_clust_claim(fcfs_args_t *args, int cid);

int
dev_full_free_cluster(fcfs_args_t *args);

//fs table
int
dev_tbl_clrs_cnt(fcfs_args_t *args, int fid);

int
dev_tbl_clrs_get(fcfs_args_t *args, int fid, int id);

int
dev_tbl_clrs_add(fcfs_args_t *args, int fid, int id);

int
dev_tbl_clrs_set(fcfs_args_t *args, int fid, int id, int val);

#endif