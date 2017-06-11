#include "../dev.h"
#include "../utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <scsi.h>

#include <debug.h>

fcfs_dir_entry_t *
dev_read_dir(fcfs_args_t *args, int fid, int *ret_sz)
{
    DEBUG();
    //define vars
    int lblk_sz = args->fs_head->phy_blk_sz * args->fs_head->blk_sz;
    fcfs_dir_entry_t   *dir_list = NULL;
    char *dir_buf = NULL;
    int   dir_buf_len = 0;
    //get file blocks
    int seq_sz = 0;
    free(dev_get_file_seq(args, fid, &seq_sz));
    dir_buf = calloc(1, lblk_sz * seq_sz);
    //read blocks
    for(size_t i = 0; i < seq_sz; ++i)
    {
        dev_read_by_id(args, fid, i, dir_buf + dir_buf_len, lblk_sz);
        dir_buf_len += lblk_sz;
    }
    //get dir len
    DEBUG();
    fcfs_file_header_t fh;
    memcpy(&fh, dir_buf, sizeof(fcfs_file_header_t));
    *ret_sz = fh.f_sz / sizeof(fcfs_dir_entry_t);
    if(fh.f_sz == 0)
        return NULL;
    int dir_entr_len = fh.f_sz;
    //copy to new array
    dir_list = calloc(1, dir_entr_len);
    memcpy(dir_list, dir_buf + sizeof(fcfs_file_header_t), dir_entr_len);

    free(dir_buf);
    return dir_list;
}

int
dev_write_dir(fcfs_args_t *args, int fid, fcfs_dir_entry_t *ent, int len)
{
    DEBUG();
    //define vars
    int lblk_sz = args->fs_head->blk_sz * args->fs_head->phy_blk_sz;
    int dir_buf_len = len * sizeof(fcfs_dir_entry_t);
    int dir_blk_cnt = to_blk_cnt(dir_buf_len + sizeof(fcfs_file_header_t), lblk_sz);
    int dir_buf_off = 0;

    char *dir_buf = calloc(1, dir_blk_cnt * lblk_sz);
    fcfs_file_header_t fh;
    fh.f_sz = len * sizeof(fcfs_dir_entry_t);

    memcpy(dir_buf, &fh, sizeof(fcfs_file_header_t));
    memcpy(dir_buf + sizeof(fcfs_file_header_t), ent, dir_buf_len);

    int seq_sz = 0;
    dev_blk_info_t *inf = dev_get_file_seq(args, fid, &seq_sz);

    if(seq_sz < dir_blk_cnt)
    {
        seq_sz = dev_extd_blk_list(args, &inf, seq_sz, dir_blk_cnt - seq_sz, fid);
    }
    else if(seq_sz > dir_blk_cnt)
    {
        DEBUG("free blocks!");
    }

    for(size_t i = 0; i < seq_sz; ++i)
    {
        dev_write_by_id(args, fid, i, dir_buf + dir_buf_off, lblk_sz);
        dir_buf_off += lblk_sz;
    }

    free(dir_buf);
    dev_destr_blk_info(inf);
    return 0;
}

int
dev_rm_from_dir(fcfs_args_t *args, int fid, int del_id)
{
    int dirs_len = 0;
    fcfs_dir_entry_t *dirs = dev_read_dir(args, fid, &dirs_len);

    int index = -1;
    for(size_t i = 0; i < dirs_len; ++i)
    {
        if(dirs[i].fid == del_id)
        {
            memcpy(&dirs[i], &dirs[dirs_len - 1], sizeof(fcfs_dir_entry_t));
            index = i;
            break;
        }
    }

    if(index < 0)
        return -1;

    fcfs_dir_entry_t *tmp = calloc(1, (dirs_len - 1) * sizeof(fcfs_dir_entry_t));
    memcpy(tmp, dirs, (dirs_len - 1) * sizeof(fcfs_dir_entry_t));

    dev_write_dir(args, fid, tmp, dirs_len - 1);

    free(dirs);
    return 0;
}
