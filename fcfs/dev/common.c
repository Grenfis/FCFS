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
dev_upd_bitmap(fcfs_args_t *args, fcfs_block_list_t *bl, int cid) {
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
dev_free_cluster(fcfs_args_t *args) {
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
dev_free_fid(fcfs_args_t *args) {
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
dev_file_alloc(fcfs_args_t *args, int fid) {
    DEBUG();
    memset(&args->fs_table->entrys[fid], 0, sizeof(fcfs_table_entry_t));

    int cid = dev_free_cluster(args);
    if(cid <= 0 ) {
        ERROR("not enouth clusters");
        return -1;
    }

    memset(&args->fs_table->entrys[fid], 0, sizeof(fcfs_table_entry_t));
    args->fs_table->entrys[fid].link_count = 1;
    args->fs_table->entrys[fid].clusters[0] = cid;

    fcfs_block_list_t *bl = dev_read_ctable(args, cid);
    int blk_cnt = 0;
    int *blks = dev_get_blocks(bl, 0, &blk_cnt);
    if(blk_cnt == 0) {
        ERROR("cluster is not free");
        free(blks);
        free(bl);
        return -1;
    }
    bl->entrys[blks[0] - 1].file_id = fid;
    bl->entrys[blks[0] - 1].num     = 0;
    dev_write_block(args, cid, 0, (char*)bl, sizeof(fcfs_block_list_t));

    if(dev_upd_bitmap(args, bl, cid))
        dev_write_bitmap(args);
    dev_write_table(args);

    free(bl);
    free(blks);
    return 0;
}

int
dev_file_size(fcfs_args_t *args, int fid) {
    DEBUG();

    int lblk_sz = args->fs_head->phy_block_size + args->fs_head->block_size;
    fcfs_file_header_t fh;
    char buf[lblk_sz];
    int res = dev_read_by_id(args, fid, 0, buf, lblk_sz);
    if(res < 0)
        return -1;
    memcpy(&fh, buf, sizeof(fcfs_file_header_t));
    return fh.file_size;
}

int
dev_set_file_size(fcfs_args_t *args, int fid, int size) {
    DEBUG();

    int lblk_sz = args->fs_head->phy_block_size + args->fs_head->block_size;
    fcfs_file_header_t fh;
    fh.file_size = size;

    char buf[lblk_sz];
    int res = dev_read_by_id(args, fid, 0, buf, lblk_sz);
    if(res < 0)
        return -1;
    //memcpy(&fh, buf, sizeof(fcfs_file_header_t));
    memcpy(buf, &fh, sizeof(fcfs_file_header_t));

    res = dev_write_by_id(args, fid, 0, buf, lblk_sz);
    
    return res;
}

dev_blk_info_t *
dev_free_blocks(fcfs_args_t *args, int count, int *size) {
    dev_blk_info_t *res = calloc(1, sizeof(dev_blk_info_t) * count);

    size_t i = 0;
    for(; i < count;) {
        int cid = dev_free_cluster(args);
        fcfs_block_list_t *bl = dev_read_ctable(args, cid);
        int b_len = 0;
        int *blist = dev_get_blocks(bl, 0, &b_len);
        for(size_t j = 0; j < b_len && i < count; ++j) {
            res[i].cid = cid;
            res[i].bid = blist[j];
            res[i].num = 0;
            i++;
        }
        free(blist);
        free(bl);
    }

    *size = i;
    return res;
}

int
dev_del_block(fcfs_args_t *args, int fid, int cid, int bid) {
    fcfs_table_entry_t *tentry = &args->fs_table->entrys[fid];
    fcfs_block_list_t *bl = dev_read_ctable(args, cid);

    int b_len = 0;
    int *blist = dev_get_blocks(bl, fid, &b_len);

    bl->entrys[bid - 1].file_id = 0;
    bl->entrys[bid - 1].num = 0;

    if(!dev_upd_bitmap(args, bl, cid))
        dev_write_bitmap(args);
    dev_write_block(args, cid, 0, (char*)bl, sizeof(fcfs_block_list_t));

    if(b_len == 1) {
        for(size_t i = 0; i < FCFS_MAX_CLASTER_COUNT_PER_FILE - 1; ++i) {
            if(tentry->clusters[i] == cid)
                tentry->clusters[i] = 0;
        }
        dev_write_table(args);
    }

    free(blist);
    free(bl);

    return 0;
}
