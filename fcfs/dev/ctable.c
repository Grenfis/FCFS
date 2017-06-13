#include "../dev.h"
#include "cache.h"
#include "../utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <scsi.h>

#include <debug.h>

static unsigned long
get_hashsum(fcfs_args_t *args, fcfs_block_list_t *bl)
{
    unsigned int sum = 0;
    for(size_t i = 0; i < FCFS_BLOKS_PER_CLUSTER - 1; ++i)
    {
        sum += bl->entrs[i].fid;
    }
    return sum ^ args->fs_head->ctime;
}

fcfs_block_list_t *
dev_read_ctable(fcfs_args_t *args, int id)
{
    DEBUG();
    fcfs_block_list_t *table = calloc(1, sizeof(fcfs_block_list_t));
    int lblk_sz = args->fs_head->phy_blk_sz * args->fs_head->blk_sz;
    int dta_beg = args->fs_head->dta_beg * lblk_sz;
    int clu_sz = FCFS_BLOKS_PER_CLUSTER * lblk_sz;

    int res = fseek(args->dev, dta_beg + clu_sz * id, SEEK_SET);
    if(res == 1L)
    {
        ERROR("seeking to cluster");
        return NULL;
    }

    char buf[lblk_sz];
    res = fread(buf, 1, lblk_sz, args->dev);
    if(res != lblk_sz)
    {
        ERROR("reading cluster table");
        return NULL;
    }

    memcpy(table, buf, sizeof(fcfs_block_list_t));
    if(table->hashsum != get_hashsum(args, table)) {
        memset(table, 0, sizeof(fcfs_block_list_t));
    }
    return table;
}

int
dev_write_ctable(fcfs_args_t *args, int id, fcfs_block_list_t *bl)
{
    bl->hashsum = get_hashsum(args, bl);
    int lblk_sz = args->fs_head->phy_blk_sz * args->fs_head->blk_sz;
    int dta_beg = args->fs_head->dta_beg * lblk_sz;
    int clu_sz = FCFS_BLOKS_PER_CLUSTER * lblk_sz;

    int res = fseek(args->dev, dta_beg + clu_sz * id, SEEK_SET);
    if(res == 1L)
    {
        ERROR("seeking to cluster");
        return -1;
    }

    res = fwrite(bl, 1, sizeof(fcfs_block_list_t), args->dev);
    if(res != lblk_sz)
    {
        ERROR("writing cluster table");
        return -1;
    }
    return 0;
}
