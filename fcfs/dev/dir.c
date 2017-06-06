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
    fcfs_table_entry_t *tentry = &args->fs_table->entrys[fid];
    fcfs_dir_entry_t   *dir_list = NULL;
    char *dir_buf = NULL;
    int   dir_buf_len = 0;
    //read all blocks from all clusters to shared buffer
    for(size_t i = 0; i < FCFS_MAX_CLASTER_COUNT_PER_FILE - 1; ++i) {
        DEBUG();
        int                 cid         = tentry->clusters[i];
        if(fid != 0 && cid == 0)
            break;
        fcfs_block_list_t   *ctable     = dev_read_ctable(args, cid); //get cluster table
        int                 blist_len   = 0;
        int                 *blist      = dev_get_blocks(ctable, fid, &blist_len); //get blocks id of file
        char                *blk_buf    = calloc(1, blist_len * lblk_sz);
        //read all blocks from cluster
        for(size_t j = 0; j < blist_len; ++j) {
            char *buf = dev_read_block(args, cid, blist[j]);
            memcpy(blk_buf + lblk_sz * j, buf, lblk_sz);
            free(buf);
        }
        //copy from local buffer to shared buffer
        if(dir_buf_len != 0) {
            char *tmp = calloc(1, dir_buf_len + lblk_sz * blist_len);
            memcpy(tmp, dir_buf, dir_buf_len);
            memcpy(tmp + dir_buf_len, blk_buf, lblk_sz * blist_len);
            free(dir_buf);
            free(blk_buf);
            dir_buf = tmp;
            dir_buf_len = dir_buf_len + lblk_sz * blist_len;
        }else{
            dir_buf = blk_buf;
            dir_buf_len = lblk_sz * blist_len;
        }
        free(ctable);
        free(blist);

        if(fid == 0)
            break;
    }
    DEBUG();
    fcfs_file_header_t fh;
    memcpy(&fh, dir_buf, sizeof(fcfs_file_header_t));
    *ret_sz = fh.file_size; //for dir this means count of dir entrys
    if(fh.file_size == 0)
        return NULL;
    int dir_entr_cnt = fh.file_size;

    dir_list = calloc(1, sizeof(fcfs_dir_entry_t) * dir_entr_cnt);
    dir_buf_len -= sizeof(fcfs_file_header_t);

    memcpy(dir_list, dir_buf + sizeof(fcfs_file_header_t), sizeof(fcfs_dir_entry_t) * dir_entr_cnt);
    free(dir_buf);
    return dir_list;
}

int
dev_write_dir(fcfs_args_t *args, int fid, fcfs_dir_entry_t *ent, int len) {
    DEBUG();
    int lblk_sz = args->fs_head->block_size * args->fs_head->phy_block_size;
    int blk_cnt = to_block_count(sizeof(unsigned) + len * sizeof(fcfs_dir_entry_t), lblk_sz);

    char *blk_buf = calloc(1, blk_cnt * lblk_sz);
    int cur_off_buf = 0;
    fcfs_file_header_t fh;
    fh.file_size = len;
    memcpy(blk_buf, &fh, sizeof(fcfs_file_header_t));
    memcpy(blk_buf + sizeof(fcfs_file_header_t), ent, sizeof(fcfs_dir_entry_t) * len);
    DEBUG("lbk cnt  %d", blk_cnt);
    DEBUG("data len %lu", sizeof(fcfs_dir_entry_t) * len);

    size_t i = 0;
    for(;i < FCFS_MAX_CLASTER_COUNT_PER_FILE - 1; ++i) {
        int cid = args->fs_table->entrys[fid].clusters[i];
        if(fid != 0 && cid == 0)
            break;
        fcfs_block_list_t *bl = dev_read_ctable(args, cid);
        int cblk_cnt = 0;
        int *cblks = dev_get_blocks(bl, fid, &cblk_cnt);

        for(size_t j = 0; (j < cblk_cnt) && (cur_off_buf < lblk_sz * blk_cnt) ; ++j) {
            int bid = cblks[j];
            dev_write_block(args, cid, bid, blk_buf + cur_off_buf, lblk_sz);
            cur_off_buf += lblk_sz;
        }

        free(bl);
        free(cblks);
        if(fid == 0)
            break;
    }
    DEBUG();
    while(cur_off_buf < blk_cnt * lblk_sz && i != FCFS_MAX_CLASTER_COUNT_PER_FILE - 1) {
        int cl_free = dev_free_cluster(args);
        args->fs_table->entrys[fid].clusters[i] = cl_free;
        if(cl_free <= 0) {
            free(ent);
            free(blk_buf);
            return -1;
        }
        fcfs_block_list_t *bl = dev_read_ctable(args, cl_free);
        int cblk_cnt = 0;
        int *cblks = dev_get_blocks(bl, 0, &cblk_cnt);
        for(size_t j = 0; j < cblk_cnt; ++j) {
            bl->entrys[cblks[j] - 1].file_id = fid;
            int bid = cblks[j];
            dev_write_block(args, cl_free, bid, blk_buf + cur_off_buf, lblk_sz);
            cur_off_buf += lblk_sz;
        }
        i += 1;
        dev_write_block(args, cl_free, 0, (char*)bl, sizeof(fcfs_block_list_t)); //save cluster table
        if(dev_upd_bitmap(args, bl, cl_free)) {
            dev_write_bitmap(args);
        }
        free(cblks);
        free(bl);
    }
    if(i == FCFS_MAX_CLASTER_COUNT_PER_FILE - 1){
        free(ent);
        free(blk_buf);
        return -1;
    }
    DEBUG();
    dev_write_table(args);
    free(ent);
    free(blk_buf);
    return 0;
}

int
dev_rm_from_dir(fcfs_args_t *args, int fid, int del_id) {
    int dirs_len = 0;
    fcfs_dir_entry_t *dirs = dev_read_dir(args, fid, &dirs_len);

    for(size_t i = 0; i < dirs_len; ++i) {
        if(dirs[i].file_id == del_id) {
            memset(dirs[i].name, 0, FCFS_MAX_FILE_NAME_LENGTH);
        }
    }

    dev_write_dir(args, fid, dirs, dirs_len);

    return 0;
}
