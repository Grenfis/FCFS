#include "../dev.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <scsi.h>

#include <debug.h>

fcfs_block_list_t *
fcfs_get_claster_table(fcfs_args_t *args, int id) {
    DEBUG();
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
    DEBUG();
    int lblk_sz = args->fs_head->phy_block_size * args->fs_head->block_size;
    int dta_beg = args->fs_head->dta_beg * lblk_sz;
    dta_beg += cid * FCFS_BLOKS_PER_CLUSTER * lblk_sz;
    dta_beg += bid * lblk_sz;

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

int *
get_blocks(fcfs_block_list_t *blist, int fid, int *ret_sz) {
    DEBUG();
    *ret_sz = 0;
    int *res = NULL;
    for(size_t i = 0; i < FCFS_BLOKS_PER_CLUSTER - 1; ++i) {
        if(fid == blist->entrys[i].file_id) {
            *ret_sz += 1;
        }
    }
    res = calloc(1, *ret_sz * sizeof(int));
    int k = 0;
    for(size_t i = 0; i < FCFS_BLOKS_PER_CLUSTER - 1; ++i) {
        if(fid == blist->entrys[i].file_id) {
            res[k] = i + 1;
            k += 1;
         }
    }
    return res;
}
