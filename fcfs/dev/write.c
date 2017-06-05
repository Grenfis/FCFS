#include "../dev.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <scsi.h>

#include <debug.h>

int
dev_write_block(fcfs_args_t *args, int cid, int bid, char *data, int len) {
    DEBUG();
    int lblk_sz = args->fs_head->block_size * args->fs_head->phy_block_size;
    int dta_beg = args->fs_head->dta_beg * lblk_sz;
    int clu_sz = FCFS_BLOKS_PER_CLUSTER * lblk_sz;

    int res = fseek(args->dev, dta_beg + clu_sz * cid + lblk_sz * bid, SEEK_SET);
    if(res == 1L) {
        ERROR("seeking to dta_beg");
        return -1;
    }

    res = fwrite(data, 1, len, args->dev);
    if(res != len) {
        ERROR("writing data");
        return -1;
    }

    return 0;
}
