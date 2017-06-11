#include "../dev.h"
#include "cache.h"
#include "../utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <scsi.h>

#include <debug.h>

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

    char *buf = calloc(1, lblk_sz);
    res = fread(buf, 1, lblk_sz, args->dev);
    if(res != lblk_sz)
    {
        ERROR("reading cluster table");
        return NULL;
    }

    memcpy(table, buf, sizeof(fcfs_block_list_t));
    free(buf);
    return table;
}

int
dev_write_ctable(fcfs_args_t *args, int id, fcfs_block_list_t *bl)
{

}
