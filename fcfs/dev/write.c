#include "../dev.h"

#include <debug.h>

int
dev_write_block(fcfs_args_t *args, int cid, int bid, const char *data, int len)
{
    DEBUG();
    int lblk_sz = args->fs_head->blk_sz * args->fs_head->phy_blk_sz;
    int dta_beg = args->fs_head->dta_beg * lblk_sz;
    int clu_sz = FCFS_BLOKS_PER_CLUSTER * lblk_sz;

    int res = fseek(args->dev, dta_beg + clu_sz * cid + lblk_sz * bid, SEEK_SET);
    if(res == 1L)
    {
        ERROR("seeking to dta_beg");
        return -1;
    }

    res = fwrite(data, 1, len, args->dev);
    if(res != len)
    {
        ERROR("writing data");
        return -1;
    }

    return 0;
}

int
dev_write_by_id(fcfs_args_t *args, int fid, int id, const char *buf, int lblk_sz, dev_blk_info_t *list, int seq_sz)
{
    //int seq_sz = 0;
    //dev_blk_info_t *list = dev_get_file_seq(args, fid, &seq_sz);
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
        //dev_destr_blk_info(list);
        return -1;
    }

    dev_write_block(args, cid, bid, buf, lblk_sz);
    return 0;
}
