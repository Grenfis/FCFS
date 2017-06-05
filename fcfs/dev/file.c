#include "../dev.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <scsi.h>

#include <debug.h>

int
dev_rm_file(fcfs_args_t *args, int fid) {
    DEBUG();
    if(fid == 0)
        return -1;

    fcfs_table_entry_t *tentry = &args->fs_table->entrys[fid];
    if(tentry->link_count == 0)
        return -1;

    tentry->link_count = 0;
    for(size_t i = 0; i < FCFS_MAX_CLASTER_COUNT_PER_FILE - 1; ++i) {
        int cid = tentry->clusters[i];
        if(cid == 0)
            break;

        fcfs_block_list_t *bl = dev_read_ctable(args, cid);
        int blist_cnt = 0;
        int *blist = dev_get_blocks(bl, fid, &blist_cnt);

        for(size_t j = 0; j < blist_cnt; ++j) {
            bl->entrys[blist[j] - 1].file_id = 0;
        }

        if(blist_cnt != 0) {
            dev_write_block(args, cid, 0, (char*)bl, sizeof(fcfs_block_list_t));
            if(!dev_upd_bitmap(args, bl, cid))
                dev_write_bitmap(args);
        }
    }

    dev_write_table(args);
    return 0;
}
