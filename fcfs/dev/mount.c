#include "../dev.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <scsi.h>

#include <debug.h>

static fcfs_head_t *
read_head(FILE *dev, int l_blk_sz) {
    DEBUG("");
    char *head_buf = calloc(1, l_blk_sz);
    fcfs_head_t *head = calloc(1, sizeof(fcfs_head_t));
    int res = fread(head_buf, 1, l_blk_sz, dev);
    if(res != l_blk_sz) {
        ERROR("can not read fs head from dev");
        exit(-1);
    }
    memcpy(head, head_buf, sizeof(fcfs_head_t));
    free(head_buf);
    return head;
}

static unsigned char *
read_bitmap(FILE *dev, int bytes) {
    DEBUG("");
    unsigned char *bitmap = calloc(1, bytes);
    int res = fread(bitmap, 1, bytes, dev);
    if(res != bytes) {
        ERROR("can not read bitmap from dev");
        exit(-1);
    }
    return bitmap;
}

static fcfs_table_t *
read_table(FILE *dev, int bytes) {
    DEBUG("");
    char *table_buf = calloc(1, bytes);
    fcfs_table_t *table = calloc(1, sizeof(fcfs_table_t));
    int res = fread(table_buf, 1, bytes, dev);
    if(res != bytes) {
        ERROR("can not read table from dev");
        exit(-1);
    }
    memcpy(table, table_buf, sizeof(fcfs_table_t));
    free(table_buf);
    return table;
}

int
dev_mount(fcfs_args_t *args) {
    DEBUG("");

    dsc_info_t *d_info = calloc(1, sizeof(dsc_info_t));
    int res = get_disc_info(args->p_dev, d_info);
    if(res != 0) {
        ERROR("could not get info about drive");
        return -1;
    }

    int l_blk_sz = d_info->sec_sz * FCFS_BLOCK_SIZE;
    free(d_info);

    res = fseek(args->dev, 0, SEEK_SET);
    if(res == 1L) {
        ERROR("seeking to beginning device");
        return -1;
    }

    fcfs_head_t *f_head = read_head(args->dev, l_blk_sz);
    args->fs_head   = f_head;
    args->fs_bitmap = read_bitmap(args->dev,    f_head->phy_block_size *
                                                f_head->block_size *
                                                f_head->bitmap_count);

    args->fs_table  = read_table(args->dev,     f_head->phy_block_size *
                                                f_head->block_size *
                                                f_head->table_count);

    DEBUG("fsid                 %u",    f_head->fsid);
    DEBUG("label                %s",    f_head->label);
    DEBUG("phy blk sz           %u",    f_head->phy_block_size);
    DEBUG("phy blk cnt          %lu",   f_head->phy_block_count);
    DEBUG("log blk sz           %u",    f_head->block_size);
    DEBUG("log blk cnt          %lu",   f_head->block_count);
    DEBUG("bitmap len           %u",    f_head->bitmap_count);
    DEBUG("table count          %u",    f_head->table_count);

    return 0;
}
