#include "../dev.h"

#include <stdlib.h>
#include <string.h>

#include <debug.h>

char *
dev_read_block(fcfs_args_t *args, int cid, int bid)
{
    DEBUG();
    int lblk_sz = args->fs_head->phy_blk_sz * args->fs_head->blk_sz;
    int dta_beg = args->fs_head->dta_beg * lblk_sz;
    int clu_sz = FCFS_BLOKS_PER_CLUSTER * lblk_sz;

    int res = fseek(args->dev, dta_beg + clu_sz * cid + lblk_sz * bid, SEEK_SET);
    if(res == 1L)
    {
        ERROR("seeking for block");
        return NULL;
    }

    char *block = calloc(1, lblk_sz);
    res = fread(block, 1, lblk_sz, args->dev);
    if(res != lblk_sz)
    {
        ERROR("reading block");
        return NULL;
    }
    return block;
}

int *
dev_get_blocks(fcfs_block_list_t *blist, int fid, int *ret_sz)
{
    DEBUG();
    *ret_sz = 0;
    int *res = NULL;
    for(size_t i = 0; i < FCFS_BLOKS_PER_CLUSTER - 1; ++i)
    {
        if(fid == blist->entrs[i].fid)
        {
            *ret_sz += 1;
        }
    }
    res = calloc(1, *ret_sz * sizeof(int));
    int k = 0;
    for(size_t i = 0; i < FCFS_BLOKS_PER_CLUSTER - 1; ++i)
    {
        if(fid == blist->entrs[i].fid)
        {
            res[k] = i + 1;
            k += 1;
         }
    }
    return res;
}

int
dev_read_by_id(fcfs_args_t *args, int fid, int id, char *buf, int lblk_sz, dev_blk_info_t *list, int seq_sz)
{
    DEBUG("fid = id - %d = %d", fid, id);
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
        //dev_destr_blk_info(list);
        return -1;
    }

    char *b = dev_read_block(args, cid, bid);
    memcpy(buf, b, sizeof(char) * lblk_sz);
    return 0;
}
