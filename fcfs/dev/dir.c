#include "../dev.h"
#include "../utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <scsi.h>

#include <debug.h>

fcfs_dir_entry_t *
dev_read_dir(fcfs_args_t *args, int fid, int *ret_sz) {
    DEBUG();
    int lblk_sz = args->fs_head->phy_block_size * args->fs_head->block_size;
    fcfs_dir_entry_t   *dir_list = NULL;
    char *dir_buf = NULL;
    int   dir_buf_len = 0;

    int seq_sz = 0;
    dev_blk_info_t *inf = dev_get_file_seq(args, fid, &seq_sz);
    dir_buf = calloc(1, lblk_sz * seq_sz);

    for(size_t i = 0; i < seq_sz; ++i) {
        char *buf = dev_read_block(args, inf[i].cid, inf[i].bid);
        memcpy(dir_buf + dir_buf_len, buf, lblk_sz);
        dir_buf_len += lblk_sz;
    }

    DEBUG();
    fcfs_file_header_t fh;
    memcpy(&fh, dir_buf, sizeof(fcfs_file_header_t));
    *ret_sz = fh.file_size / sizeof(fcfs_dir_entry_t);
    if(fh.file_size == 0)
        return NULL;
    int dir_entr_len = fh.file_size;

    dir_list = calloc(1, dir_entr_len);

    memcpy(dir_list, dir_buf + sizeof(fcfs_file_header_t), dir_entr_len);
    free(dir_buf);
    return dir_list;
}

int
dev_write_dir(fcfs_args_t *args, int fid, fcfs_dir_entry_t *ent, int len) {
    DEBUG();
    int lblk_sz = args->fs_head->block_size * args->fs_head->phy_block_size;
    int dir_buf_len = len * sizeof(fcfs_dir_entry_t);
    int dir_blk_cnt = to_block_count(dir_buf_len + sizeof(fcfs_file_header_t), lblk_sz);
    int dir_buf_off = 0;

    char *dir_buf = calloc(1, dir_blk_cnt * lblk_sz);
    fcfs_file_header_t fh;
    fh.file_size = len * sizeof(fcfs_dir_entry_t);

    memcpy(dir_buf, &fh, sizeof(fcfs_file_header_t));
    memcpy(dir_buf + sizeof(fcfs_file_header_t), ent, dir_buf_len);

    int seq_sz = 0;
    dev_blk_info_t *inf = dev_get_file_seq(args, fid, &seq_sz);

    if(seq_sz < dir_blk_cnt){
        int new_seq_sz = 0;
        dev_blk_info_t *blks = dev_free_blocks(args, dir_blk_cnt - seq_sz, &new_seq_sz);
        dev_blk_info_t *tmp = calloc(1, (new_seq_sz + seq_sz) * sizeof(dev_blk_info_t));
        memcpy(tmp, inf, sizeof(dev_blk_info_t) * seq_sz);
        memcpy(tmp + seq_sz, blks, sizeof(dev_blk_info_t) * new_seq_sz);
        free(inf);
        free(blks);
        inf = tmp;
        seq_sz += new_seq_sz;
    }else if(seq_sz > dir_blk_cnt) {
        int for_del = seq_sz - dir_blk_cnt;
        for(size_t i = 0; i < for_del; i++) {
            dev_del_block(args, fid, inf[seq_sz - i - 1].cid, inf[seq_sz - i - 1].bid);
        }
    }

    fcfs_table_entry_t *tentry = &args->fs_table->entrys[fid];
    int old_cid = -1;
    fcfs_block_list_t *bl = NULL;

    for(size_t i = 0; i < seq_sz; ++i) {
        if(inf[i].num == 0 && i != 0) {
            inf[i].num = inf[i-1].num + 1;
        }
        if(old_cid != inf[i].cid) {
            if(old_cid != -1) {
                dev_write_block(args, old_cid, 0, (char*)bl, sizeof(fcfs_block_list_t));
                if(!dev_upd_bitmap(args, bl, inf[i].cid))
                    dev_write_bitmap(args);
            }
            old_cid = inf[i].cid;
            bl = dev_read_ctable(args, inf[i].cid);
        }
        //mark new cluster in table
        for(size_t j = 0; j < FCFS_MAX_CLASTER_COUNT_PER_FILE - 1; ++j) {
            if(tentry->clusters[j] && inf[i].cid == tentry->clusters[j])
                break;
            else if(tentry->clusters[j] == 0) {
                tentry->clusters[j] = inf[i].cid;
                break;
            }

        }

        bl->entrys[inf[i].bid - 1].num = inf[i].num;
        bl->entrys[inf[i].bid - 1].file_id = fid;

        char *buf = calloc(1, lblk_sz);
        memcpy(buf, dir_buf + dir_buf_off, lblk_sz);
        dev_write_block(args, inf[i].cid, inf[i].bid, buf, lblk_sz);
        dir_buf_off += lblk_sz;
    }

    if(old_cid != -1) {
        dev_write_block(args, old_cid, 0, (char*)bl, sizeof(fcfs_block_list_t));
        if(!dev_upd_bitmap(args, bl, inf[seq_sz - 1].cid))
            dev_write_bitmap(args);
    }

    DEBUG();
    dev_write_table(args);

    free(dir_buf);
    free(inf);
    return 0;
}

int
dev_rm_from_dir(fcfs_args_t *args, int fid, int del_id) {
    int dirs_len = 0;
    fcfs_dir_entry_t *dirs = dev_read_dir(args, fid, &dirs_len);

    int index = -1;
    for(size_t i = 0; i < dirs_len; ++i) {
        if(dirs[i].file_id == del_id) {
            //memset(dirs[i].name, 0, FCFS_MAX_FILE_NAME_LENGTH);
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
