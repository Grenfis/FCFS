#include "../dev.h"
#include "cache.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <scsi.h>
#include <debug.h>

static dev_clrs_cache_t clrs_cache = {
    .fid = -1,
};

unsigned int
dev_tbl_calim_sec_addr(fcfs_args_t *args, int fid) {
    DEBUG("");
    int lblk_sz = args->fs_head->phy_blk_sz * args->fs_head->blk_sz;

    int cid = dev_full_free_cluster(args);
    dev_clust_claim(args, cid);

    clrs_cache.s1_cl = calloc(1, ((FCFS_BLOKS_PER_CLUSTER - 1) * lblk_sz) - sizeof(unsigned));
    clrs_cache.s1_cnt = 0;
    return cid;
}

unsigned int
dev_tbl_load_fir(fcfs_args_t *args, int fid) {
    unsigned int count = 0;
    fcfs_table_entry_t *tentry = &args->fs_table->entrs[fid];
    for(size_t i = 0; i < FCFS_CLUSTER_PER_FILE - 3; ++i) {
        int cid = tentry->clrs[i];
        if(fid != 0 && cid == 0)
            break;
        clrs_cache.s0_cl[count] = cid;
        count++;
        if(fid == 0)
            break;
    }
    return count;
}

unsigned int
dev_tbl_load_sec(fcfs_args_t *args, int cid) {
    DEBUG("cid = %d", cid);
    unsigned int count   = 0;
    int lblk_sz = args->fs_head->phy_blk_sz * args->fs_head->blk_sz;

    char buf[(FCFS_BLOKS_PER_CLUSTER - 1) * lblk_sz];
    for(size_t i = 1; i < FCFS_BLOKS_PER_CLUSTER - 1; ++i) {
        char *b = dev_read_block(args, cid, i);
        memcpy(buf + (i - 1) * lblk_sz, b, lblk_sz);
        free(b);
    }
    unsigned int off = sizeof(unsigned int);
    memcpy(&count, buf, off);

    clrs_cache.s1_cl = calloc(1, ((FCFS_BLOKS_PER_CLUSTER - 1) * lblk_sz) - off);
    memcpy(clrs_cache.s1_cl, buf + off, ((FCFS_BLOKS_PER_CLUSTER - 1) * lblk_sz) - off);
    clrs_cache.s1_cnt = count;

    return count;
}

int
dev_tbl_save_sec(fcfs_args_t *args, int cid) {
    DEBUG("cid = %d", cid);
    int lblk_sz = args->fs_head->phy_blk_sz * args->fs_head->blk_sz;
    int off = sizeof(unsigned int);

    char buf[(FCFS_BLOKS_PER_CLUSTER - 1) * lblk_sz];
    /*for(size_t i = 1; i < FCFS_BLOKS_PER_CLUSTER - 1; ++i) {
        char *b = dev_read_block(args, cid, i);
        memcpy(buf + (i - 1) * lblk_sz, b, lblk_sz);
        free(b);
    }*/
    memcpy(buf, &clrs_cache.s1_cnt, off);
    memcpy(buf + off, clrs_cache.s1_cl, ((FCFS_BLOKS_PER_CLUSTER - 1) * lblk_sz) - off);

    for(size_t i = 1; i < FCFS_BLOKS_PER_CLUSTER - 1; ++i) {
        dev_write_block(args, cid, i, buf + (i - 1) * lblk_sz, lblk_sz);
    }

    return 0;
}

int
dev_tbl_clrs_cnt(fcfs_args_t *args, int fid) {
    DEBUG("fid = %d", fid);

    if(clrs_cache.fid != fid) {
        clrs_cache.fid = fid;
        clrs_cache.s0_cnt = 0;
        clrs_cache.s1_cnt = 0;
        clrs_cache.s2_cnt = 0;

        if(clrs_cache.s1_cnt != 0)
            free(clrs_cache.s1_cl);
        if(clrs_cache.s2_cnt != 0)
            free(clrs_cache.s2_cl);
    }

    unsigned int count = 0;
    int cid = 0;
    fcfs_table_entry_t *tentry = &args->fs_table->entrs[fid];
    //first stage addresses
    if(clrs_cache.s0_cnt == 0) {
        clrs_cache.s0_cnt = dev_tbl_load_fir(args, fid);
    }
    count += clrs_cache.s0_cnt;
    //check for second stage
    cid = tentry->clrs[FCFS_CLUSTER_PER_FILE - 3];
    if(cid != 0) {
        DEBUG("second");
        if(clrs_cache.s1_cnt == 0)
            clrs_cache.s1_cnt = dev_tbl_load_sec(args, cid);
    }
    count += clrs_cache.s1_cnt;
    //check for third stage
    cid = tentry->clrs[FCFS_CLUSTER_PER_FILE - 2];
    if(cid != 0) {
        DEBUG("third");
        /*if(clrs_cache.s2_cnt == 0)
            //count += dev_tbl_chk_sec(args, cid);
        else
            count += clrs_cache.s2_cnt;*/
    }
    //check for fourth stage
    cid = tentry->clrs[FCFS_CLUSTER_PER_FILE - 1];
    if(cid != 0) {
        DEBUG("fourth");
        /*if(clrs_cache.s3_cnt == 0)
            count += dev_tbl_chk_sec(args, cid);
        else
            count += clrs_cache.s3_cnt;*/
    }

    return count;
}

int
dev_tbl_clrs_get(fcfs_args_t *args, int fid, int id) {
    DEBUG("fid - id = %d - %d", fid, id);
    int lblk_sz = args->fs_head->phy_blk_sz * args->fs_head->blk_sz;
    int uint_len = sizeof(unsigned int);
    int s1_len = ((lblk_sz - uint_len)
                    / (double)uint_len)
                    * (FCFS_BLOKS_PER_CLUSTER - 1);

    if(id < (FCFS_CLUSTER_PER_FILE - 3)) {
        if(id < clrs_cache.s0_cnt) {
            return clrs_cache.s0_cl[id];
        }else{
            ERROR("");
            return -1;
        }
    }else if(id >= (FCFS_CLUSTER_PER_FILE - 3) && id < s1_len) {
        if(clrs_cache.s1_cnt != 0) {
            return clrs_cache.s1_cl[id - (FCFS_CLUSTER_PER_FILE - 3)];
        }else{
            ERROR();
            return -1;
        }
    }/*else if(id == (FCFS_CLUSTER_PER_FILE - 2)) {

    }else if(id == (FCFS_CLUSTER_PER_FILE - 1)) {
    }*/else{
        ERROR("impossible id");
        return -1;
    }
}

int
dev_tbl_clrs_add(fcfs_args_t *args, int fid, int id) {
    int count = dev_tbl_clrs_cnt(args, fid);
    int lblk_sz = args->fs_head->phy_blk_sz * args->fs_head->blk_sz;
    fcfs_table_entry_t *tentry = &args->fs_table->entrs[fid];
    int uint_len = sizeof(unsigned int);
    int s1_len = ((lblk_sz - uint_len)
                    / (double)uint_len)
                    * (FCFS_BLOKS_PER_CLUSTER - 1);

    if(count < (FCFS_CLUSTER_PER_FILE - 3)) {
        clrs_cache.s0_cl[clrs_cache.s0_cnt] = id;
        tentry->clrs[clrs_cache.s0_cnt] = id;
        clrs_cache.s0_cnt++;
        dev_write_table(args);
    }else if(count >= (FCFS_CLUSTER_PER_FILE - 3) && count < s1_len) {
        if(clrs_cache.s1_cnt == 0) {
            tentry->clrs[(FCFS_CLUSTER_PER_FILE - 3)] = dev_tbl_calim_sec_addr(args, fid);
        }
        clrs_cache.s1_cl[clrs_cache.s1_cnt] = id;
        clrs_cache.s1_cnt++;
        dev_tbl_save_sec(args, args->fs_table->entrs[fid].clrs[FCFS_CLUSTER_PER_FILE - 3]);
        dev_write_table(args);
    }/*else if(id == (FCFS_CLUSTER_PER_FILE - 2)) {

    }else if(id == (FCFS_CLUSTER_PER_FILE - 1)) {

    }*/else{
        ERROR("impossible id");
        return -1;
    }
    return 0;
}

int
dev_tbl_clrs_set(fcfs_args_t *args, int fid, int id, int val) {
    DEBUG("fid - id - val = %d - %d - %d", fid, id, val);
    int lblk_sz = args->fs_head->phy_blk_sz * args->fs_head->blk_sz;
    int uint_len = sizeof(unsigned int);
    int s1_len = ((lblk_sz - uint_len)
                    / (double)uint_len)
                    * (FCFS_BLOKS_PER_CLUSTER - 1);

    if(id < (FCFS_CLUSTER_PER_FILE - 3)) {
        if(id < clrs_cache.s0_cnt) {
            //return clrs_cache.s0_cl[id];
            clrs_cache.s0_cl[id] = val;
        }else{
            ERROR("");
            return -1;
        }
    }else if(id >= (FCFS_CLUSTER_PER_FILE - 3) && id < s1_len) {
        if(clrs_cache.s1_cnt != 0) {
            //return clrs_cache.s1_cl[id];
            clrs_cache.s1_cl[id] = val;
        }else{
            ERROR();
            return -1;
        }
    }/*else if(id == (FCFS_CLUSTER_PER_FILE - 2)) {

    }else if(id == (FCFS_CLUSTER_PER_FILE - 1)) {

    }*/else{
        ERROR("impossible id");
        return -1;
    }
    return 0;
}
