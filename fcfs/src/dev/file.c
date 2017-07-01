#include "../dev.h"
#include "../cache.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <debug.h>

int
dev_rm_file(fcfs_args_t *args, int fid, int pfid)
{
    DEBUG();
    if(fid == 0)
        return -1;

    fcfs_table_entry_t *tentry = &args->fs_table->entrs[fid];
    int clust_cnt = dev_tbl_clrs_cnt(args, fid);
    tentry->lnk_cnt--;
    if(tentry->lnk_cnt != 0)
        return -1;

    for(size_t i = 0; i < clust_cnt; ++i)
    {
        int cid = dev_tbl_clrs_get(args, fid, i);
        if(cid == 0)
            break;

        fcfs_block_list_t *bl = dev_read_ctable(args, cid);
        int blist_cnt = 0;
        int *blist = dev_get_blocks(bl, fid, &blist_cnt);

        for(size_t j = 0; j < blist_cnt; ++j)
        {
            bl->entrs[blist[j] - 1].fid = 0;
            bl->entrs[blist[j] - 1].num = 0;
        }

        if(blist_cnt != 0)
        {
            //dev_write_block(args, cid, 0, (char*)bl, sizeof(fcfs_block_list_t));
            dev_write_ctable(args, cid, bl);
            if(dev_upd_bitmap(args, bl, cid))
                dev_write_bitmap(args);
        }
    }

    dev_rm_from_dir(args, pfid, fid);
    dev_write_table(args);
    return 0;
}

int
dev_create_file(fcfs_args_t *args, int pfid, int fid, const char *name, mode_t mode)
{
    if(fid <= 0)
    {
        return -ENOENT;
    }

    int res = 0;
    int dirs_len = 0;
    fcfs_dir_entry_t *dirs = dev_read_dir(args, pfid, &dirs_len);
    fcfs_dir_entry_t *tmp = NULL;

    int new_pos = -1;
    dirs_len += 1;
    tmp = calloc(1, sizeof(fcfs_dir_entry_t) * dirs_len);
    memcpy(tmp, dirs, sizeof(fcfs_dir_entry_t) * (dirs_len - 1));
    if(dirs != NULL)
        free(dirs);
    new_pos = dirs_len - 1;

    dev_file_alloc(args, fid);
    tmp[new_pos].fid     = fid; //set free file id
    tmp[new_pos].mode    = mode;
    tmp[new_pos].ctime   = time(NULL);
    tmp[new_pos].atime   = time(NULL);
    tmp[new_pos].mtime   = time(NULL);
    strcpy(tmp[new_pos].name, name);

    res = dev_write_dir(args, pfid, tmp, dirs_len);

    return res;
}

int
dev_init_file(fcfs_args_t *args, int fid)
{
    DEBUG();

    fcfs_file_header_t fh;
    memset(&fh, 0, sizeof(fcfs_file_header_t));

    int cid = dev_tbl_clrs_get(args, fid, 0);
    fcfs_block_list_t *bl = dev_read_ctable(args, cid);

    int b_len = 0;
    int *blks = dev_get_blocks(bl, fid, &b_len);

    int res = dev_write_block(args, cid, blks[0], (char*)&fh, sizeof(fcfs_file_header_t), NEED);

    if(bl != NULL)
        free(bl);
    if(blks != NULL)
        free(blks);
    return res;
}

dev_blk_info_t *
dev_get_file_seq(fcfs_args_t *args, int fid, int *size)
{
    DEBUG();
    //check cache hint
    int t_len = 0;
    dev_blk_info_t *t = cache_seq_get(fid, &t_len);
    if(t != NULL) {
        *size = t_len;
        return t;
    }

    int clust_cnt = dev_tbl_clrs_cnt(args, fid);
    dev_blk_info_t *first, *last, *pre_last;
    first = pre_last = last = calloc(1, sizeof(dev_blk_info_t));
    int k = 0;
    for(size_t i = 0; i < clust_cnt; ++i)
    {
        int cid = dev_tbl_clrs_get(args, fid, i);
        if(fid != 0 && cid == 0)
            break;

        fcfs_block_list_t *bl = dev_read_ctable(args, cid);
        int b_len = 0;
        int *blist = dev_get_blocks(bl, fid, &b_len);
        for(size_t j = 0; j < b_len; ++j)
        {
            last->cid = cid;
            last->bid = blist[j];
            last->num = bl->entrs[blist[j] - 1].num;
            last->next = calloc(1, sizeof(dev_blk_info_t));
            pre_last = last;
            last = last->next;
            k++;
        }

        if(bl != NULL)
            free(bl);
        if(blist != NULL)
            free(blist);

        if(fid == 0)
            break;
    }

    if(last != NULL)
        free(last);
    pre_last->next = NULL;

    unsigned char flag = 1;
    dev_blk_info_t fi;
    while(flag)
    {
        flag = 0;
        dev_blk_info_t *cur, *nxt;
        cur = first;
        for(size_t i = 1; i < k; ++i)
        {
            nxt = cur->next;
            if(cur->num > nxt->num)
            {
                flag = 1;
                fi.cid = cur->cid;
                fi.bid = cur->bid;
                fi.num = cur->num;
                cur->cid = nxt->cid;
                cur->bid = nxt->bid;
                cur->num = nxt->num;
                nxt->cid = fi.cid;
                nxt->bid = fi.bid;
                nxt->num = fi.num;
            }
        }
    }
    //add to cache
    cache_seq_add(fid, first);

    *size = k;
    return first;
}

int
dev_file_size(fcfs_args_t *args, int fid)
{
    DEBUG();

    int lblk_sz = args->fs_head->phy_blk_sz * args->fs_head->blk_sz;
    fcfs_file_header_t fh;
    char buf[lblk_sz];

    int seq_sz = 0;
    dev_blk_info_t *inf = dev_get_file_seq(args, fid, &seq_sz);

    int res = dev_read_by_id(args, fid, 0, buf, lblk_sz, inf, seq_sz);
    if(res < 0)
        return -1;
    memcpy(&fh, buf, sizeof(fcfs_file_header_t));
    return fh.f_sz;
}

int
dev_file_alloc(fcfs_args_t *args, int fid)
{
    DEBUG();
    memset(&args->fs_table->entrs[fid], 0, sizeof(fcfs_table_entry_t));

    int cid = dev_free_cluster(args);
    if(cid <= 0 )
    {
        ERROR("not enouth clrs");
        return -1;
    }

    memset(&args->fs_table->entrs[fid], 0, sizeof(fcfs_table_entry_t));
    args->fs_table->entrs[fid].lnk_cnt = 1;
    args->fs_table->entrs[fid].clrs[0] = cid;

    fcfs_block_list_t *bl = dev_read_ctable(args, cid);
    int blk_cnt = 0;
    int *blks   = dev_get_blocks(bl, 0, &blk_cnt);
    if(blk_cnt == 0)
    {
        ERROR("cluster is not free");
        if(blks != NULL)
            free(blks);
        if(bl != NULL)
            free(bl);
        return -1;
    }
    bl->entrs[blks[0] - 1].fid = fid;
    bl->entrs[blks[0] - 1].num = 0;
    //dev_write_block(args, cid, 0, (char*)bl, sizeof(fcfs_block_list_t));
    dev_write_ctable(args, cid, bl);

    if(dev_upd_bitmap(args, bl, cid))
        dev_write_bitmap(args);
    dev_write_table(args);

    if(bl != NULL)
        free(bl);
    if(blks != NULL)
        free(blks);
    return 0;
}

int
dev_set_file_size(fcfs_args_t *args, int fid, int size)
{
    DEBUG();

    int lblk_sz = args->fs_head->phy_blk_sz * args->fs_head->blk_sz;
    fcfs_file_header_t fh;
    fh.f_sz = size;

    int seq_sz = 0;
    dev_blk_info_t *inf = dev_get_file_seq(args, fid, &seq_sz);

    char buf[lblk_sz];
    int res = dev_read_by_id(args, fid, 0, buf, lblk_sz, inf, seq_sz);
    if(res < 0)
        return -1;
    memcpy(buf, &fh, sizeof(fcfs_file_header_t));

    res = dev_write_by_id(args, fid, 0, buf, lblk_sz, inf, seq_sz);
    return res;
}

void
dev_file_mem_clrs(fcfs_args_t *args, int fid, dev_blk_info_t *inf, int seq_sz, int clust_cnt)
{
    for(size_t i = 0; i < seq_sz; ++i)
    {
        char f = 0;
        for(size_t j = 0; j < clust_cnt; ++j)
        {
            int t = dev_tbl_clrs_get(args, fid, j);
            if(t == inf->cid)
            {
                f = 1;
                break;
            }
        }
        if(f != 1)
        {
            dev_tbl_clrs_add(args, fid, inf->cid);
            clust_cnt++;
        }
        inf = inf->next;
    }
}

int
dev_file_reserve(fcfs_args_t *args, int fid, dev_blk_info_t *inf, int seq_sz, int last_num)
{
    if(fid == 0)
    {
        ERROR("root can not be resize");
        return -1;
    }
    int clust_cnt = dev_tbl_clrs_cnt(args, fid);
    dev_blk_info_t *first = inf;
    if(clust_cnt < 0)
        return -1;
    for(size_t i = 0; i < seq_sz; ++i)
    {
        last_num++;
        fcfs_block_list_t *bl = dev_read_ctable(args, inf->cid);
        bl->entrs[inf->bid - 1].fid = fid;
        bl->entrs[inf->bid - 1].num = last_num;
        //dev_write_block(args, inf->cid, 0, (char*)bl, sizeof(fcfs_block_list_t));
        dev_write_ctable(args, inf->cid, bl);
        inf->num = last_num;

        if(dev_upd_bitmap(args, bl, inf->cid))
            dev_write_bitmap(args);
        if(bl != NULL)
            free(bl);
        inf = inf->next;
    }
    dev_file_mem_clrs(args, fid, first, seq_sz, clust_cnt);
    dev_write_table(args);
    return 0;
}
