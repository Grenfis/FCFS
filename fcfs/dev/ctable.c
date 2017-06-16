#include "../dev.h"
#include "cache.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <scsi.h>
#include <utils.h>

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

    char *b = dev_read_block(args, id, 0, NONEED);
    memcpy(table, b, sizeof(fcfs_block_list_t));
    if(table->hashsum != get_hashsum(args, table)) {
        memset(table, 0, sizeof(fcfs_block_list_t));
    }
    free(b);
    return table;
}

int
dev_write_ctable(fcfs_args_t *args, int id, fcfs_block_list_t *bl)
{
    bl->hashsum = get_hashsum(args, bl);
    dev_write_block(args, id, 0, (char*)bl, sizeof(fcfs_block_list_t), NONEED);
    return 0;
}
