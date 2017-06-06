#include "../dev.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <scsi.h>
#include <errno.h>

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

int
dev_create_file(fcfs_args_t *args, int pfid, int fid, const char *name, mode_t mode) {
    if(fid <= 0) {
        return -ENOENT;
    }

    int dirs_len = 0;
    fcfs_dir_entry_t *dirs = dev_read_dir(args, pfid, &dirs_len);
    fcfs_dir_entry_t *tmp = NULL;

    int new_pos = -1;
    for(size_t i = 0; i < dirs_len; ++i) {
        if(strlen(dirs[i].name) == 0) {
            new_pos = i;
            break;
        }
    }

    if(new_pos != -1) {
        tmp = dirs;
    }else{
        dirs_len += 1;
        tmp = calloc(1, sizeof(fcfs_dir_entry_t) * dirs_len);
        memcpy(tmp, dirs, sizeof(fcfs_dir_entry_t) * (dirs_len - 1));
        free(dirs);
        new_pos = dirs_len - 1;
    }

    dev_file_alloc(args, fid);
    tmp[new_pos].file_id       = fid; //set free file id
    tmp[new_pos].mode          = mode;
    tmp[new_pos].create_date   = time(NULL);
    tmp[new_pos].access_date   = time(NULL);
    tmp[new_pos].change_date   = time(NULL);

    strcpy(tmp[new_pos].name, name);

    dev_write_dir(args, pfid, tmp, dirs_len);

    return 0;
}
