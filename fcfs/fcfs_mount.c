#include "fcfs_mount.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <scsi.h>

#include <debug.h>

static fcfs_head_t *
read_head(FILE *dev, int l_blk_sz) {
    DEBUG("");
    char *head_buf = calloc(1, l_blk_sz);
    fcfs_head_t *head = calloc(1, sizeof(fcfs_head_t));
    int res = fread(head_buf, 1, l_blk_sz, dev);
    if(res != l_blk_sz) {
        ERROR("can not read fs head from dev");
        exit(-1);
    }
    memcpy(head, head_buf, sizeof(fcfs_head_t));
    free(head_buf);
    return head;
}

static unsigned char *
read_bitmap(FILE *dev, int bytes) {
    DEBUG("");
    unsigned char *bitmap = calloc(1, bytes);
    int res = fread(bitmap, 1, bytes, dev);
    if(res != bytes) {
        ERROR("can not read bitmap from dev");
        exit(-1);
    }
    return bitmap;
}

static fcfs_table_t *
read_table(FILE *dev, int bytes) {
    DEBUG("");
    char *table_buf = calloc(1, bytes);
    fcfs_table_t *table = calloc(1, sizeof(fcfs_table_t));
    int res = fread(table_buf, 1, bytes, dev);
    if(res != bytes) {
        ERROR("can not read table from dev");
        exit(-1);
    }
    memcpy(table, table_buf, sizeof(fcfs_table_t));
    free(table_buf);
    return table;
}

int
fcfs_mount(fcfs_args_t *args) {
    DEBUG("");

    dsc_info_t *d_info = calloc(1, sizeof(dsc_info_t));
    int res = get_disc_info(args->p_dev, d_info);
    if(res != 0) {
        ERROR("could not get info about drive");
        return -1;
    }

    int l_blk_sz = d_info->sec_sz * FCFS_BLOCK_SIZE;
    free(d_info);

    res = fseek(args->dev, 0, SEEK_SET);
    if(res == 1L) {
        ERROR("seeking to beginning device");
        return -1;
    }

    fcfs_head_t *f_head = read_head(args->dev, l_blk_sz);
    args->fs_head   = f_head;
    args->fs_bitmap = read_bitmap(args->dev,    f_head->phy_block_size *
                                                f_head->block_size *
                                                f_head->bitmap_count);

    args->fs_table  = read_table(args->dev,     f_head->phy_block_size *
                                                f_head->block_size *
                                                f_head->table_count);

    DEBUG("fsid                 %u",    f_head->fsid);
    DEBUG("label                %s",    f_head->label);
    DEBUG("phy blk sz           %u",    f_head->phy_block_size);
    DEBUG("phy blk cnt          %lu",   f_head->phy_block_count);
    DEBUG("log blk sz           %u",    f_head->block_size);
    DEBUG("log blk cnt          %lu",   f_head->block_count);
    DEBUG("bitmap len           %u",    f_head->bitmap_count);
    DEBUG("table count          %u",    f_head->table_count);

    return 0;
}

fcfs_block_list_t *
fcfs_get_claster_table(fcfs_args_t *args, int id) {
    fcfs_block_list_t *table = calloc(1, sizeof(fcfs_block_list_t));
    int lblk_sz = args->fs_head->phy_block_size * args->fs_head->block_size;
    int dta_beg = args->fs_head->dta_beg * lblk_sz;
    int clu_sz = FCFS_BLOKS_PER_CLUSTER * lblk_sz;

    int res = fseek(args->dev, dta_beg + clu_sz * id, SEEK_SET);
    if(res == 1L) {
        ERROR("seeking to cluster");
        return NULL;
    }

    char *buf = calloc(1, lblk_sz);
    res = fread(buf, 1, lblk_sz, args->dev);
    if(res != lblk_sz) {
        ERROR("reading cluster table");
        return NULL;
    }

    memcpy(table, buf, sizeof(fcfs_block_list_t));
    free(buf);
    return table;
}

char *
fcfs_read_block(fcfs_args_t *args, int cid, int bid) {
    int lblk_sz = args->fs_head->phy_block_size * args->fs_head->block_size;
    int dta_beg = args->fs_head->dta_beg * lblk_sz;
    dta_beg += cid * FCFS_BLOKS_PER_CLUSTER * lblk_sz;
    dta_beg += bid * lblk_sz + lblk_sz;

    int res = fseek(args->dev, dta_beg, SEEK_SET);
    if(res == 1L) {
        ERROR("seeking for block");
        return NULL;
    }

    char *block = calloc(1, lblk_sz);
    res = fread(block, 1, lblk_sz, args->dev);
    if(res != lblk_sz) {
        ERROR("reading block");
        return NULL;
    }

    return block;
}

static int *
get_blocks(fcfs_block_list_t *blist, int fid, int *ret_sz) {
    *ret_sz = 0;
    int *res = NULL;
    for(size_t i = 0; i < FCFS_BLOKS_PER_CLUSTER - 1; ++i) {
        if(fid == blist->entrys[i].file_id) {
            *ret_sz += 1;
        }
    }
    res = calloc(1, *ret_sz);
    for(size_t i = 0, k = 0; i < FCFS_BLOKS_PER_CLUSTER - 1; ++i) {
        if(fid == blist->entrys[i].file_id) {
            res[k] = i;
         }
    }
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

    int dir_entr_cnt = 0;
    memcpy(&dir_entr_cnt, dir_buf, sizeof(unsigned));
    *ret_sz = dir_entr_cnt;
    if(dir_entr_cnt == 0)
        return NULL;

    dir_list = calloc(1, sizeof(fcfs_dir_entry_t) * dir_entr_cnt);

    dir_buf_len -= sizeof(unsigned);
    dir_buf     += sizeof(unsigned);

    memcpy(dir_list, dir_buf, sizeof(fcfs_dir_entry_t) * dir_entr_cnt);
    free(dir_buf);
    return dir_list;
}
