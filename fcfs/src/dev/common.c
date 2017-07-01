#include "../dev.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <scsi.h>

#include <debug.h>

static int mask_off[] = {7,6,5,4,3,2,1,0};
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

void
dev_destr_blk_info(dev_blk_info_t *i)
{
    while(i != NULL)
    {
        dev_blk_info_t *t = i;
        i = i->next;
        free(t);
    }
}

int
dev_full_free_cluster(fcfs_args_t *args)
{
    DEBUG();
    int start   = 0;
    int cid     = 0;
    int b_len   = 0;
    do
    {
        cid = dev_free_cluster_from(args, start);
        fcfs_block_list_t *bl = dev_read_ctable(args, cid);
        b_len = 0;
        int *blist = dev_get_blocks(bl, 0, &b_len);
        if(blist != NULL)
            free(blist);
        if(bl != NULL)
            free(bl);
        start = cid;
    }
    while(b_len != (FCFS_BLOKS_PER_CLUSTER - 1));
    return cid;
}

char
dev_upd_bitmap(fcfs_args_t *args, fcfs_block_list_t *bl, int cid)
{
    DEBUG();
    char has_free = 0;
    char being_full = 0;

    for(size_t i = 0; i < FCFS_BLOKS_PER_CLUSTER - 1; ++i)
    {
        if(bl->entrs[i].fid==0)
        {
            has_free = 1;
            break;
        }
    }

    int byte = cid / 8.0;
    int num  = cid % 8;

    unsigned char cs = args->fs_bitmap[byte];
    cs &= mask[num];
    cs >>= mask_off[num];
    if(cs==1)
        being_full = 1;

    if(has_free == 0 && !being_full)
    {
        args->fs_bitmap[byte] |= mask[num];
        return 1;
    }
    else if(has_free == 1 && being_full)
    {
        args->fs_bitmap[byte] &= ~(mask[num]);
        return 1;
    }

    return 0;
}

int
dev_free_cluster_from(fcfs_args_t *args, int cid)
{
    DEBUG();
    unsigned char *bitmap = args->fs_bitmap;
    unsigned long btm_len = args->fs_head->clu_cnt;

    int sg = cid / 8.0;
    int sc = (cid % 8) + 1;

    for(int i = sg; i < btm_len; ++i)
    {
        unsigned char cs = bitmap[i];
        if(cs == 255)
            continue;
        int j = sc;
        sc = 0;
        for(; j < 8; ++j)
        {
            unsigned char tmp = cs;
            tmp &= mask[j];
            tmp >>= mask_off[j];
            if(tmp == 0)
            {
                return i * 8 + j;
            }
        }
    }
    return -1;
}

int
dev_free_cluster(fcfs_args_t *args)
{
    return dev_free_cluster_from(args, 0);
}

int
dev_free_fid(fcfs_args_t *args)
{
    DEBUG();
    fcfs_table_t *table = args->fs_table;
    int tbl_len = args->fs_head->tbl_cnt;
    for(int i = 1; i < tbl_len; ++i)
    {
        if(table->entrs[i].lnk_cnt == 0)
            return i;
    }
    return -1;
}

dev_blk_info_t *
dev_free_blocks(fcfs_args_t *args, int count, int *size)
{
    size_t i = 0;
    int old_cid = 0;
    dev_blk_info_t *first, *last;
    first = last = calloc(1, sizeof(dev_blk_info_t));

    for(; i < count;)
    {
        int cid = dev_free_cluster_from(args, old_cid);
        fcfs_block_list_t *bl = dev_read_ctable(args, cid);
        int b_len = 0;
        int *blist = dev_get_blocks(bl, 0, &b_len);
        for(size_t j = 0; j < b_len && i < count; ++j)
        {
            last->cid = cid;
            old_cid = cid;
            last->bid = blist[j];
            last->num = 0;
            if(i != count - 1)
                last->next = calloc(1, sizeof(dev_blk_info_t));
            last = last->next;
            i++;
        }
        if(blist != NULL)
            free(blist);
        if(bl != NULL)
            free(bl);
    }
    //free(last);
    *size = i;
    return first;
}

int
dev_del_block(fcfs_args_t *args, int fid, int cid, int bid)
{
    int res = 0;
    int clust_cnt = dev_tbl_clrs_cnt(args, fid);
    fcfs_block_list_t *bl = dev_read_ctable(args, cid);

    int b_len  = 0;
    int *blist = dev_get_blocks(bl, fid, &b_len);

    bl->entrs[bid - 1].fid = 0;
    bl->entrs[bid - 1].num = 0;

    if(dev_upd_bitmap(args, bl, cid))
        dev_write_bitmap(args);
    //dev_write_block(args, cid, 0, (char*)bl, sizeof(fcfs_block_list_t));
    res = dev_write_ctable(args, cid, bl);

    if(b_len == 1)
    {
        for(size_t i = 0; i < clust_cnt; ++i)
        {
            if(dev_tbl_clrs_get(args, fid, i) == cid)
                dev_tbl_clrs_set(args, fid, i, 0);
        }
        dev_write_table(args);
    }

    if(blist != NULL)
        free(blist);
    if(bl != NULL)
        free(bl);
    return res;
}

int
dev_extd_blk_list(fcfs_args_t *args, dev_blk_info_t **inf, int seq_sz,int count, int fid)
{
    int new_seq_sz = 0;
    dev_blk_info_t *blks = dev_free_blocks(args, count, &new_seq_sz);
    int res = dev_file_reserve(args, fid, blks, new_seq_sz, seq_sz-1);
    if(res < 0)
        return -1;
    dev_blk_info_t *last = *inf;
    while(last->next != NULL)
        last = last->next;
    last->next = blks;
    return seq_sz += new_seq_sz;
}

int
dev_clust_claim(fcfs_args_t *args, int cid)
{
    DEBUG();
    fcfs_block_list_t *bl = dev_read_ctable(args, cid);

    for(size_t i = 0; i < FCFS_BLOKS_PER_CLUSTER - 1; ++i)
    {
        bl->entrs[i].fid = -1;
        bl->entrs[i].num = i;
    }

    //dev_write_block(args, cid, 0, (char*)bl, sizeof(fcfs_block_list_t));
    int res = dev_write_ctable(args, cid, bl);
    if(dev_upd_bitmap(args, bl, cid))
        dev_write_bitmap(args);
    if(bl != NULL)
        free(bl);
    return res;
}
