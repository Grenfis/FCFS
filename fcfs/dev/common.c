#include "../dev.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <scsi.h>

#include <debug.h>

static unsigned char mask[] = {
    0b10000000,
    0b01000000,
    0b00100000,
    0b00010000,
    0b00001000,
    0b00000100,
    0b00000010,
    0b00000001
};
static int mask_off[] = {7,6,5,4,3,2,1,0};

char
update_bitmap(fcfs_args_t *args, fcfs_block_list_t *bl, int cid) {
    DEBUG();
    char has_free = 0;
    char being_full = 0;

    for(size_t i = 0; i < FCFS_BLOKS_PER_CLUSTER - 1; ++i) {
        if(bl->entrys[i].file_id == 0) {
            has_free = 1;
            break;
        }
    }

    int byte = cid / 8.0;
    int num  = cid % 8;

    unsigned char cs = args->fs_bitmap[byte];
    cs &= mask[num];
    cs >>= mask_off[num];
    if(cs == 1)
        being_full = 1;

    if(has_free == 0 && !being_full) {
        args->fs_bitmap[byte] |= mask[num];
        return 0;
    }else if(has_free == 1 && being_full){
        args->fs_bitmap[byte] &= ~(mask[num]);
        return 0;
    }

    return 1;
}

int
fcfs_get_free_cluster(fcfs_args_t *args) {
    DEBUG();
    unsigned char *bitmap = args->fs_bitmap;
    int btm_len = args->fs_head->cluster_count;
    for(size_t i = 0; i < btm_len; ++i) {
        unsigned char cs = bitmap[i];
        if(cs == 255)
            continue;
        int j = 0;
        for(; j < 8; ++j) {
            if(i == 0 && j == 0)
                continue;
            unsigned char tmp = cs;
            tmp &= mask[j];
            tmp >>= mask_off[j];
            if(tmp == 0) {
                return i * 8 + j;
            }
        }
    }
    return -1;
}

int
fcfs_get_free_fid(fcfs_args_t *args) {
    DEBUG();
    fcfs_table_t *table = args->fs_table;
    int tbl_len = args->fs_head->table_len;
    for(size_t i = 1; i < tbl_len; ++i) {
        if(table->entrys[i].link_count == 0)
            return i;
    }
    return -1;
}

int
fcfs_alloc(fcfs_args_t *args, int fid) {
    DEBUG();
    memset(&args->fs_table->entrys[fid], 0, sizeof(fcfs_table_entry_t));

    int cid = fcfs_get_free_cluster(args);
    if(cid <= 0 ) {
        ERROR("not enouth clusters");
        return -1;
    }

    memset(&args->fs_table->entrys[fid], 0, sizeof(fcfs_table_entry_t));
    args->fs_table->entrys[fid].link_count = 1;
    args->fs_table->entrys[fid].clusters[0] = cid;

    fcfs_block_list_t *bl = fcfs_get_claster_table(args, cid);
    int blk_cnt = 0;
    int *blks = get_blocks(bl, 0, &blk_cnt);
    if(blk_cnt == 0) {
        ERROR("cluster is not free");
        free(blks);
        free(bl);
        return -1;
    }
    bl->entrys[blks[0] - 1].file_id = fid;
    fcfs_write_block(args, cid, 0, (char*)bl, sizeof(fcfs_block_list_t));

    if(update_bitmap(args, bl, cid))
        fcfs_write_bitmap(args);
    fcfs_write_table(args);

    free(bl);
    free(blks);
    return 0;
}

int
fcfs_get_file_size(fcfs_args_t *args, int fid) {
    DEBUG();
    int block_cnt = 0;
    int lblk_sz = args->fs_head->block_size * args->fs_head->phy_block_size;
    fcfs_table_entry_t *tentry = &args->fs_table->entrys[fid];
    for(size_t i = 0; i < FCFS_MAX_CLASTER_COUNT_PER_FILE; ++i) {
        int cid = tentry->clusters[i];
        if(fid != 0 && cid == 0)
            break;
        fcfs_block_list_t *ctable = fcfs_get_claster_table(args, cid);
        int blist_len = 0;
        int *blist = get_blocks(ctable, fid, &blist_len);
        block_cnt += blist_len;

        free(ctable);
        free(blist);
        if(fid == 0)
            break;
    }
    return block_cnt * lblk_sz;
}
