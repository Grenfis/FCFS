#include "../dev.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <scsi.h>
#include <errno.h>

#include <debug.h>

int
dev_rm_file(fcfs_args_t *args, int fid, int pfid) {
    DEBUG();
    if(fid == 0)
        return -1;

    fcfs_table_entry_t *tentry = &args->fs_table->entrys[fid];
    tentry->link_count--;
    if(tentry->link_count != 0)
        return -1;

    for(size_t i = 0; i < FCFS_MAX_CLASTER_COUNT_PER_FILE - 1; ++i) {
        int cid = tentry->clusters[i];
        if(cid == 0)
            break;

        fcfs_block_list_t *bl = dev_read_ctable(args, cid);
        int blist_cnt = 0;
        int *blist = dev_get_blocks(bl, fid, &blist_cnt);

        for(size_t j = 0; j < blist_cnt; ++j) {
            bl->entrys[blist[j] - 1].file_id = 0;
        }

        if(blist_cnt != 0) {
            dev_write_block(args, cid, 0, (char*)bl, sizeof(fcfs_block_list_t));
            if(!dev_upd_bitmap(args, bl, cid))
                dev_write_bitmap(args);
        }
    }

    dev_rm_from_dir(args, pfid, fid);
    dev_write_table(args);
    return 0;
}

int
dev_create_file(fcfs_args_t *args, int pfid, int fid, const char *name, mode_t mode) {
    if(fid <= 0) {
        return -ENOENT;
    }

    int dirs_len = 0;
    fcfs_dir_entry_t *dirs = dev_read_dir(args, pfid, &dirs_len);
    fcfs_dir_entry_t *tmp = NULL;

    int new_pos = -1;
    for(size_t i = 0; i < dirs_len; ++i) {
        if(strlen(dirs[i].name) == 0) {
            new_pos = i;
            break;
        }
    }

    if(new_pos != -1) {
        tmp = dirs;
    }else{
        dirs_len += 1;
        tmp = calloc(1, sizeof(fcfs_dir_entry_t) * dirs_len);
        memcpy(tmp, dirs, sizeof(fcfs_dir_entry_t) * (dirs_len - 1));
        free(dirs);
        new_pos = dirs_len - 1;
    }

    dev_file_alloc(args, fid);
    tmp[new_pos].file_id       = fid; //set free file id
    tmp[new_pos].mode          = mode;
    tmp[new_pos].create_date   = time(NULL);
    tmp[new_pos].access_date   = time(NULL);
    tmp[new_pos].change_date   = time(NULL);

    strcpy(tmp[new_pos].name, name);

    dev_write_dir(args, pfid, tmp, dirs_len);

    return 0;
}

int
dev_init_file(fcfs_args_t *args, int fid) {
    DEBUG();
    fcfs_table_entry_t *tentry = &args->fs_table->entrys[fid];

    fcfs_file_header_t fh;
    memset(&fh, 0, sizeof(fcfs_file_header_t));

    int cid = tentry->clusters[0];
    fcfs_block_list_t *bl = dev_read_ctable(args, cid);

    int b_len = 0;
    int *blks = dev_get_blocks(bl, fid, &b_len);

    int res = dev_write_block(args, cid, blks[0], (char*)&fh, sizeof(fcfs_file_header_t));

    free(bl);
    free(blks);
    return res;
}

dev_blk_info_t *
dev_get_file_seq(fcfs_args_t *args, int fid, int *size) {
    DEBUG();
    fcfs_table_entry_t *tentry = &args->fs_table->entrys[fid];

    dev_blk_info_t tmp[(FCFS_MAX_CLASTER_COUNT_PER_FILE - 1) * FCFS_BLOKS_PER_CLUSTER];
    int k = 0;
    for(size_t i = 0; i < FCFS_MAX_CLASTER_COUNT_PER_FILE - 1; ++i) {
        int cid = tentry->clusters[i];
        if(fid != 0 && cid == 0)
            break;

        fcfs_block_list_t *bl = dev_read_ctable(args, cid);
        int b_len = 0;
        int *blist = dev_get_blocks(bl, fid, &b_len);
        for(size_t j = 0; j < b_len; ++j) {
            tmp[k].cid = cid;
            tmp[k].bid = blist[j];
            tmp[k].num = bl->entrys[blist[j] - 1].num;
            k++;
        }

        free(bl);
        free(blist);

        if(fid == 0)
            break;
    }

    unsigned char flag = 1;
    dev_blk_info_t fi;
    while(flag) {
        flag = 0;
        for(size_t i = 1; i < k; ++i) {
            if(tmp[i - 1].num > tmp[i].num) {
                flag = 1;
                memcpy(&fi, &tmp[i - 1], sizeof(dev_blk_info_t));
                memcpy(&tmp[i - 1], &tmp[i], sizeof(dev_blk_info_t));
                memcpy(&tmp[i], &fi, sizeof(dev_blk_info_t));
            }
        }
    }

    dev_blk_info_t *inf = calloc(1, sizeof(dev_blk_info_t) * k);
    memcpy(inf, &tmp, sizeof(dev_blk_info_t) * k);

    *size = k;
    return inf;
}
