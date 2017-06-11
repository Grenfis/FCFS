#include "../dev.h"

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
        }

        if(blist_cnt != 0)
        {
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
dev_create_file(fcfs_args_t *args, int pfid, int fid, const char *name, mode_t mode)
{
    if(fid <= 0)
    {
        return -ENOENT;
    }

    int dirs_len = 0;
    fcfs_dir_entry_t *dirs = dev_read_dir(args, pfid, &dirs_len);
    fcfs_dir_entry_t *tmp = NULL;

    int new_pos = -1;
    dirs_len += 1;
    tmp = calloc(1, sizeof(fcfs_dir_entry_t) * dirs_len);
    memcpy(tmp, dirs, sizeof(fcfs_dir_entry_t) * (dirs_len - 1));
    free(dirs);
    new_pos = dirs_len - 1;

    dev_file_alloc(args, fid);
    tmp[new_pos].fid     = fid; //set free file id
    tmp[new_pos].mode    = mode;
    tmp[new_pos].ctime   = time(NULL);
    tmp[new_pos].atime   = time(NULL);
    tmp[new_pos].mtime   = time(NULL);
    strcpy(tmp[new_pos].name, name);

    dev_write_dir(args, pfid, tmp, dirs_len);

    return 0;
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

    int res = dev_write_block(args, cid, blks[0], (char*)&fh, sizeof(fcfs_file_header_t));

    free(bl);
    free(blks);
    return res;
}

dev_blk_info_t *
dev_get_file_seq(fcfs_args_t *args, int fid, int *size)
{
    DEBUG();
    int clust_cnt = dev_tbl_clrs_cnt(args, fid);
    dev_blk_info_t *first, *last;
    first = last = calloc(1, sizeof(dev_blk_info_t));
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
            last = last->next;
            k++;
        }

        free(bl);
        free(blist);

        if(fid == 0)
            break;
    }

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

    *size = k;
    return first;
}

int
dev_read_by_id(fcfs_args_t *args, int fid, int id, char *buf, int lblk_sz)
{
    DEBUG("fid = id - %d = %d", fid, id);
    int seq_sz = 0;
    dev_blk_info_t *list = dev_get_file_seq(args, fid, &seq_sz);
    dev_blk_info_t *inf = list;

    if(seq_sz <= 0)
        return -1;

    int cid = -1;
    int bid = -1;
    for(size_t i = 0; i < seq_sz; ++i)
    {
        if(inf->num == id)
        {
            cid = inf->cid;
            bid = inf->bid;
            break;
        }
        inf = inf->next;
    }
    DEBUG("cid = bid - %d = %d", cid, bid);
    if(cid < 0 || bid < 0)
    {
        dev_destr_blk_info(list);
        return -1;
    }

    char *b = dev_read_block(args, cid, bid);
    memcpy(buf, b, sizeof(char) * lblk_sz);

    return 0;
}

int
dev_write_by_id(fcfs_args_t *args, int fid, int id, const char *buf, int lblk_sz)
{
    int seq_sz = 0;
    dev_blk_info_t *list = dev_get_file_seq(args, fid, &seq_sz);
    dev_blk_info_t *inf = list;

    if(seq_sz <= 0)
        return -1;

    int cid = -1;
    int bid = -1;
    for(size_t i = 0; i < seq_sz; ++i)
    {
        if(inf->num == id)
        {
            cid = inf->cid;
            bid = inf->bid;
            break;
        }
        inf = inf->next;
    }

    if(cid < 0 || bid < 0)
    {
        dev_destr_blk_info(list);
        return -1;
    }

    dev_write_block(args, cid, bid, buf, lblk_sz);
    return 0;
}
