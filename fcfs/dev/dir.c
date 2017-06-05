#include "../dev.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <scsi.h>

#include <debug.h>

int
fcfs_init_dir(fcfs_args_t *args, int fid) {
    DEBUG();
    fcfs_table_entry_t *tentry = &args->fs_table->entrys[fid];

    fcfs_dir_header_t dh;
    memset(&dh, 0, sizeof(fcfs_dir_header_t));

    int cid = tentry->clusters[0];
    fcfs_block_list_t *bl = fcfs_get_claster_table(args, cid);

    int b_len = 0;
    int *blks = get_blocks(bl, fid, &b_len);

    int res = fcfs_write_block(args, cid, blks[0], (char*)&dh, sizeof(fcfs_dir_header_t));

    free(bl);
    free(blks);
    return res;
}

fcfs_dir_entry_t *
fcfs_read_directory(fcfs_args_t *args, int fid, int *ret_sz) {
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
        fcfs_block_list_t   *ctable     = fcfs_get_claster_table(args, cid); //get cluster table
        int                 blist_len   = 0;
        int                 *blist      = get_blocks(ctable, fid, &blist_len); //get blocks id of file
        char                *blk_buf    = calloc(1, blist_len * lblk_sz);
        //read all blocks from cluster
        for(size_t j = 0; j < blist_len; ++j) {
            char *buf = fcfs_read_block(args, cid, blist[j]);
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
    int dir_entr_cnt = 0;
    memcpy(&dir_entr_cnt, dir_buf, sizeof(unsigned));
    *ret_sz = dir_entr_cnt;
    if(dir_entr_cnt == 0)
        return NULL;

    dir_list = calloc(1, sizeof(fcfs_dir_entry_t) * dir_entr_cnt);
    dir_buf_len -= sizeof(unsigned);

    memcpy(dir_list, dir_buf + sizeof(unsigned), sizeof(fcfs_dir_entry_t) * dir_entr_cnt);
    free(dir_buf);
    return dir_list;
}

int
fcfs_write_directory(fcfs_args_t *args, int fid, fcfs_dir_entry_t *ent, int len) {
    DEBUG();
    int lblk_sz = args->fs_head->block_size * args->fs_head->phy_block_size;
    int blk_cnt = to_block_count(sizeof(unsigned) + len * sizeof(fcfs_dir_entry_t), lblk_sz);

    char *blk_buf = calloc(1, blk_cnt * lblk_sz);
    int cur_off_buf = 0;
    memcpy(blk_buf, &len, sizeof(unsigned));
    memcpy(blk_buf + sizeof(unsigned), ent, sizeof(fcfs_dir_entry_t) * len);
    DEBUG("lbk cnt  %d", blk_cnt);
    DEBUG("data len %lu", sizeof(fcfs_dir_entry_t) * len);

    size_t i = 0;
    for(;i < FCFS_MAX_CLASTER_COUNT_PER_FILE - 1; ++i) {
        int cid = args->fs_table->entrys[fid].clusters[i];
        if(fid != 0 && cid == 0)
            break;
        fcfs_block_list_t *bl = fcfs_get_claster_table(args, cid);
        int cblk_cnt = 0;
        int *cblks = get_blocks(bl, fid, &cblk_cnt);

        for(size_t j = 0; (j < cblk_cnt) && (cur_off_buf < lblk_sz * blk_cnt) ; ++j) {
            int bid = cblks[j];
            fcfs_write_block(args, cid, bid, blk_buf + cur_off_buf, lblk_sz);
            cur_off_buf += lblk_sz;
        }

        free(bl);
        free(cblks);
        if(fid == 0)
            break;
    }
    DEBUG();
    while(cur_off_buf < blk_cnt * lblk_sz && i != FCFS_MAX_CLASTER_COUNT_PER_FILE - 1) {
        int cl_free = fcfs_get_free_cluster(args);
        args->fs_table->entrys[fid].clusters[i] = cl_free;
        if(cl_free <= 0) {
            free(ent);
            free(blk_buf);
            return -1;
        }
        fcfs_block_list_t *bl = fcfs_get_claster_table(args, cl_free);
        int cblk_cnt = 0;
        int *cblks = get_blocks(bl, 0, &cblk_cnt);
        for(size_t j = 0; j < cblk_cnt; ++j) {
            bl->entrys[cblks[j] - 1].file_id = fid;
            int bid = cblks[j];
            fcfs_write_block(args, cl_free, bid, blk_buf + cur_off_buf, lblk_sz);
            cur_off_buf += lblk_sz;
        }
        i += 1;
        fcfs_write_block(args, cl_free, 0, (char*)bl, sizeof(fcfs_block_list_t)); //save cluster table
        if(update_bitmap(args, bl, cl_free)) {
            fcfs_write_bitmap(args);
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
    fcfs_write_table(args);
    free(ent);
    free(blk_buf);
    return 0;
}

int
fcfs_remove_from_dir(fcfs_args_t *args, int fid, int del_id) {
    int dirs_len = 0;
    fcfs_dir_entry_t *dirs = fcfs_read_directory(args, fid, &dirs_len);

    for(size_t i = 0; i < dirs_len; ++i) {
        if(dirs[i].file_id == del_id) {
            memset(dirs[i].name, 0, FCFS_MAX_FILE_NAME_LENGTH);
        }
    }

    fcfs_write_directory(args, fid, dirs, dirs_len);

    return 0;
}
