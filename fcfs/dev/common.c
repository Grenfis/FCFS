#include "../dev.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <scsi.h>

#include <debug.h>

static unsigned char mask[] = {
    0b10000000,
    0b01000000,
    0b00100000,
    0b00010000,
    0b00001000,
    0b00000100,
    0b00000010,
    0b00000001
};
static int mask_off[] = {7,6,5,4,3,2,1,0};

void
dev_destr_blk_info(dev_blk_info_t *i) {
    while(i != NULL) {
        dev_blk_info_t *t = i;
        i = i->next;
        free(t);
    }
}

int
dev_full_free_cluster(fcfs_args_t *args) {
    DEBUG();
    int start = 0;
    int cid = 0;
    int b_len = 0;
    do {
        cid = dev_free_cluster_from(args, start);
        fcfs_block_list_t *bl = dev_read_ctable(args, cid);
        b_len = 0;
        int *blist = dev_get_blocks(bl, 0, &b_len);
        free(blist);
        free(bl);
        start = cid;
    }while(b_len != (FCFS_BLOKS_PER_CLUSTER - 1));
    return cid;
}

char
dev_upd_bitmap(fcfs_args_t *args, fcfs_block_list_t *bl, int cid) {
    DEBUG();
    char has_free = 0;
    char being_full = 0;

    for(size_t i = 0; i < FCFS_BLOKS_PER_CLUSTER - 1; ++i) {
        if(bl->entrs[i].fid == 0) {
            has_free = 1;
            break;
        }
    }

    int byte = cid / 8.0;
    int num  = cid % 8;

    unsigned char cs = args->fs_bitmap[byte];
    cs &= mask[num];
    cs >>= mask_off[num];
    if(cs == 1)
        being_full = 1;

    if(has_free == 0 && !being_full) {
        args->fs_bitmap[byte] |= mask[num];
        return 0;
    }else if(has_free == 1 && being_full){
        args->fs_bitmap[byte] &= ~(mask[num]);
        return 0;
    }

    return 1;
}

int
dev_free_cluster_from(fcfs_args_t *args, int cid) {
    DEBUG();
    unsigned char *bitmap = args->fs_bitmap;
    unsigned long btm_len = args->fs_head->clu_cnt;

    int sg = cid / 8.0;
    int sc = (cid % 8) + 1;

    for(int i = sg; i < btm_len; ++i) {
        unsigned char cs = bitmap[i];
        if(cs == 255)
            continue;
        int j = sc;
        sc = 0;
        for(; j < 8; ++j) {
            /*if(i == 0 && j == 0)
                continue;*/
            unsigned char tmp = cs;
            tmp &= mask[j];
            tmp >>= mask_off[j];
            if(tmp == 0) {
                return i * 8 + j;
            }
        }
    }
    return -1;
}

int
dev_free_cluster(fcfs_args_t *args) {
    return dev_free_cluster_from(args, 0);
}

int
dev_free_fid(fcfs_args_t *args) {
    DEBUG();
    fcfs_table_t *table = args->fs_table;
    int tbl_len = args->fs_head->tbl_cnt;
    for(int i = 1; i < tbl_len; ++i) {
        if(table->entrs[i].lnk_cnt == 0)
            return i;
    }
    return -1;
}

int
dev_file_alloc(fcfs_args_t *args, int fid) {
    DEBUG();
    memset(&args->fs_table->entrs[fid], 0, sizeof(fcfs_table_entry_t));

    int cid = dev_free_cluster(args);
    if(cid <= 0 ) {
        ERROR("not enouth clrs");
        return -1;
    }

    memset(&args->fs_table->entrs[fid], 0, sizeof(fcfs_table_entry_t));
    args->fs_table->entrs[fid].lnk_cnt = 1;
    args->fs_table->entrs[fid].clrs[0] = cid;

    fcfs_block_list_t *bl = dev_read_ctable(args, cid);
    int blk_cnt = 0;
    int *blks = dev_get_blocks(bl, 0, &blk_cnt);
    if(blk_cnt == 0) {
        ERROR("cluster is not free");
        free(blks);
        free(bl);
        return -1;
    }
    bl->entrs[blks[0] - 1].fid = fid;
    bl->entrs[blks[0] - 1].num     = 0;
    dev_write_block(args, cid, 0, (char*)bl, sizeof(fcfs_block_list_t));

    if(dev_upd_bitmap(args, bl, cid))
        dev_write_bitmap(args);
    dev_write_table(args);

    free(bl);
    free(blks);
    return 0;
}

int
dev_file_size(fcfs_args_t *args, int fid) {
    DEBUG();

    int lblk_sz = args->fs_head->phy_blk_sz * args->fs_head->blk_sz;
    fcfs_file_header_t fh;
    char buf[lblk_sz];
    int res = dev_read_by_id(args, fid, 0, buf, lblk_sz);
    if(res < 0)
        return -1;
    memcpy(&fh, buf, sizeof(fcfs_file_header_t));
    return fh.f_sz;
}

int
dev_set_file_size(fcfs_args_t *args, int fid, int size) {
    DEBUG();

    int lblk_sz = args->fs_head->phy_blk_sz * args->fs_head->blk_sz;
    fcfs_file_header_t fh;
    fh.f_sz = size;

    char buf[lblk_sz];
    int res = dev_read_by_id(args, fid, 0, buf, lblk_sz);
    if(res < 0)
        return -1;
    //memcpy(&fh, buf, sizeof(fcfs_file_header_t));
    memcpy(buf, &fh, sizeof(fcfs_file_header_t));

    res = dev_write_by_id(args, fid, 0, buf, lblk_sz);

    return res;
}

dev_blk_info_t *
dev_free_blocks(fcfs_args_t *args, int count, int *size) {
    //dev_blk_info_t *res = calloc(1, sizeof(dev_blk_info_t) * count);

    size_t i = 0;
    int old_cid = 0;
    dev_blk_info_t *first, *last;
    first = last = calloc(1, sizeof(dev_blk_info_t));

    for(; i < count;) {
        int cid = dev_free_cluster_from(args, old_cid);
        fcfs_block_list_t *bl = dev_read_ctable(args, cid);
        int b_len = 0;
        int *blist = dev_get_blocks(bl, 0, &b_len);
        for(size_t j = 0; j < b_len && i < count; ++j) {
            last->cid = cid;
            old_cid = cid;
            last->bid = blist[j];
            last->num = 0;
            last->next = calloc(1, sizeof(dev_blk_info_t));
            last = last->next;
            i++;
        }
        free(blist);
        free(bl);
    }

    *size = i;
    return first;
}

int
dev_del_block(fcfs_args_t *args, int fid, int cid, int bid) {
    //fcfs_table_entry_t *tentry = &args->fs_table->entrs[fid];
    int clust_cnt = dev_tbl_clrs_cnt(args, fid);
    fcfs_block_list_t *bl = dev_read_ctable(args, cid);

    int b_len = 0;
    int *blist = dev_get_blocks(bl, fid, &b_len);

    bl->entrs[bid - 1].fid = 0;
    bl->entrs[bid - 1].num = 0;

    if(!dev_upd_bitmap(args, bl, cid))
        dev_write_bitmap(args);
    dev_write_block(args, cid, 0, (char*)bl, sizeof(fcfs_block_list_t));

    if(b_len == 1) {
        for(size_t i = 0; i < clust_cnt; ++i) {
            //if(tentry->clrs[i] == cid)
            if(dev_tbl_clrs_get(args, fid, i) == cid)
                //tentry->clrs[i] = 0;
                dev_tbl_clrs_set(args, fid, i, 0);
        }
        dev_write_table(args);
    }

    free(blist);
    free(bl);

    return 0;
}

void
dev_file_mem_clrs(fcfs_args_t *args, int fid, dev_blk_info_t *inf, int seq_sz, int clust_cnt) {
    for(size_t i = 0; i < seq_sz; ++i) {
        char f = 0;
        for(size_t j = 0; j < clust_cnt; ++j) {
            int t = dev_tbl_clrs_get(args, fid, j);
            if(t == inf->cid) {
                f = 1;
                break;
            }else if(t < 0) {
                //return -1;
            }
        }
        if(f != 1) {
            dev_tbl_clrs_add(args, fid, inf->cid);
            clust_cnt++;
        }
        inf = inf->next;
    }
}

int
dev_file_reserve(fcfs_args_t *args, int fid, dev_blk_info_t *inf, int seq_sz, int last_num) {
    if(fid == 0) {
        ERROR("root can not be resize");
        return -1;
    }
    int clust_cnt = dev_tbl_clrs_cnt(args, fid);
    dev_blk_info_t *first = inf;
    if(clust_cnt < 0)
        return -1;
    for(size_t i = 0; i < seq_sz; ++i) {
        last_num++;
        fcfs_block_list_t *bl = dev_read_ctable(args, inf->cid);
        bl->entrs[inf->bid - 1].fid = fid;
        bl->entrs[inf->bid - 1].num = last_num;
        dev_write_block(args, inf->cid, 0, (char*)bl, sizeof(fcfs_block_list_t));
        inf->num = last_num;

        if(!dev_upd_bitmap(args, bl, inf->cid))
            dev_write_bitmap(args);
        free(bl);
        inf = inf->next;
    }
    dev_file_mem_clrs(args, fid, first, seq_sz, clust_cnt);
    dev_write_table(args);
    return 0;
}

int
dev_extd_blk_list(fcfs_args_t *args, dev_blk_info_t **inf, int seq_sz,int count, int fid) {
    int new_seq_sz = 0;
    dev_blk_info_t *blks = dev_free_blocks(args, count, &new_seq_sz);
    int res = dev_file_reserve(args, fid, blks, new_seq_sz, seq_sz-1);
    if(res < 0)
        return -1;
    /*dev_blk_info_t *tmp = calloc(1, (new_seq_sz + seq_sz) * sizeof(dev_blk_info_t));
    memcpy(tmp, *inf, sizeof(dev_blk_info_t) * seq_sz);
    memcpy(tmp + seq_sz, blks, sizeof(dev_blk_info_t) * new_seq_sz);
    free(*inf);
    free(blks);
    *inf = tmp;*/
    dev_blk_info_t *last = *inf;
    while(last->next != NULL)
        last = last->next;
    last->next = blks;
    return seq_sz += new_seq_sz;
}

int
dev_clust_claim(fcfs_args_t *args, int cid) {
    DEBUG();
    fcfs_block_list_t *bl = dev_read_ctable(args, cid);

    for(size_t i = 0; i < FCFS_BLOKS_PER_CLUSTER - 1; ++i) {
        bl->entrs[i].fid = -1;
        bl->entrs[i].num = i;
    }

    dev_write_block(args, cid, 0, (char*)bl, sizeof(fcfs_block_list_t));
    if(!dev_upd_bitmap(args, bl, cid))
        dev_write_bitmap(args);
    free(bl);
    return 0;
}
