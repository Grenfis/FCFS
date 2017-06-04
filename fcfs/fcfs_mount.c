#include "fcfs_mount.h"

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

static char
update_bitmap(fcfs_args_t *args, fcfs_block_list_t *bl, int cid) {
    DEBUG();
    char has_free = 0;
    for(size_t i = 0; i < FCFS_BLOKS_PER_CLUSTER - 1; ++i) {
        if(bl->entrys[i].file_id == 0) {
            has_free = 1;
            break;
        }
    }
    if(has_free == 0) {
        int byte = cid / 8.0;
        int num  = cid % 8;
        args->fs_bitmap[byte] |= mask[num];
        return 1;
    }
    return 0;
}

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
fcfs_mount(fcfs_args_t *args) {
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

int
fcfs_write_header(fcfs_args_t *args) {
    DEBUG("");
    int lblk_sz = args->fs_head->phy_block_size * args->fs_head->block_size;
    char *blk_buf = calloc(1, lblk_sz);
    memcpy(blk_buf, args->fs_head, sizeof(fcfs_head_t));

    int res = fseek(args->dev, 0, SEEK_SET);
    if(res == 1L) {
        ERROR("seeking device");
        return -1;
    }

    res = fwrite(blk_buf, 1, lblk_sz, args->dev);
    if(res != lblk_sz) {
        ERROR("writing head");
    }

    free(blk_buf);
    return 0;
}

int
fcfs_write_bitmap(fcfs_args_t *args) {
    DEBUG("");
    int lblk_sz = args->fs_head->phy_block_size * args->fs_head->block_size;
    int btm_blk_len = args->fs_head->bitmap_count * lblk_sz;
    char *blk_buf = calloc(1, btm_blk_len);
    memcpy(blk_buf, args->fs_bitmap, args->fs_head->cluster_count / 8.0);

    int res = fseek(args->dev, lblk_sz, SEEK_SET);
    if(res == 1L) {
        ERROR("seeking device");
        return -1;
    }

    res = fwrite(blk_buf, 1, btm_blk_len, args->dev);
    if(res != btm_blk_len) {
        ERROR("writing bitmap");
    }

    free(blk_buf);
    return 0;
}

int
fcfs_write_table(fcfs_args_t *args) {
    DEBUG("");
    int lblk_sz = args->fs_head->phy_block_size * args->fs_head->block_size;
    int btm_blk_len = args->fs_head->bitmap_count * lblk_sz;
    int tbl_blk_len = args->fs_head->table_count  * lblk_sz;

    char *blk_buf = calloc(1, tbl_blk_len);
    DEBUG("fcfs_table len %lu", sizeof(fcfs_table_t));
    memcpy(blk_buf, args->fs_table, sizeof(fcfs_table_t));

    int res = fseek(args->dev, lblk_sz + btm_blk_len, SEEK_SET);
    if(res == 1L) {
        ERROR("seeking device");
        return -1;
    }

    res = fwrite(blk_buf, 1, tbl_blk_len, args->dev);
    if(res != tbl_blk_len) {
        ERROR("writing table");
        return -1;
    }

    free(blk_buf);
    return 0;
}

fcfs_block_list_t *
fcfs_get_claster_table(fcfs_args_t *args, int id) {
    DEBUG();
    fcfs_block_list_t *table = calloc(1, sizeof(fcfs_block_list_t));
    int lblk_sz = args->fs_head->phy_block_size * args->fs_head->block_size;
    int dta_beg = args->fs_head->dta_beg * lblk_sz;
    int clu_sz = FCFS_BLOKS_PER_CLUSTER * lblk_sz;

    int res = fseek(args->dev, dta_beg + clu_sz * id, SEEK_SET);
    if(res == 1L) {
        ERROR("seeking to cluster");
        return NULL;
    }

    char *buf = calloc(1, lblk_sz);
    res = fread(buf, 1, lblk_sz, args->dev);
    if(res != lblk_sz) {
        ERROR("reading cluster table");
        return NULL;
    }

    memcpy(table, buf, sizeof(fcfs_block_list_t));
    free(buf);
    return table;
}

char *
fcfs_read_block(fcfs_args_t *args, int cid, int bid) {
    DEBUG();
    int lblk_sz = args->fs_head->phy_block_size * args->fs_head->block_size;
    int dta_beg = args->fs_head->dta_beg * lblk_sz;
    dta_beg += cid * FCFS_BLOKS_PER_CLUSTER * lblk_sz;
    dta_beg += bid * lblk_sz;

    int res = fseek(args->dev, dta_beg, SEEK_SET);
    if(res == 1L) {
        ERROR("seeking for block");
        return NULL;
    }

    char *block = calloc(1, lblk_sz);
    res = fread(block, 1, lblk_sz, args->dev);
    if(res != lblk_sz) {
        ERROR("reading block");
        return NULL;
    }

    return block;
}

static int *
get_blocks(fcfs_block_list_t *blist, int fid, int *ret_sz) {
    DEBUG();
    *ret_sz = 0;
    int *res = NULL;
    for(size_t i = 0; i < FCFS_BLOKS_PER_CLUSTER - 1; ++i) {
        if(fid == blist->entrys[i].file_id) {
            *ret_sz += 1;
        }
    }
    res = calloc(1, *ret_sz * sizeof(int));
    int k = 0;
    for(size_t i = 0; i < FCFS_BLOKS_PER_CLUSTER - 1; ++i) {
        if(fid == blist->entrys[i].file_id) {
            res[k] = i + 1;
            k += 1;
         }
    }
    return res;
}

fcfs_dir_entry_t *
fcfs_read_directory(fcfs_args_t *args, int fid, int *ret_sz) {
    DEBUG();
    int lblk_sz = args->fs_head->phy_block_size * args->fs_head->block_size;
    fcfs_table_entry_t *tentry = &args->fs_table->entrys[fid];
    fcfs_dir_entry_t   *dir_list = NULL;
    char *dir_buf = NULL;
    int   dir_buf_len = 0;
    //read all blocks from all clusters to shared buffer
    for(size_t i = 0; i < FCFS_MAX_CLASTER_COUNT_PER_FILE - 1; ++i) {
        DEBUG();
        int                 cid         = tentry->clusters[i];
        if(fid != 0 && cid == 0)
            break;
        fcfs_block_list_t   *ctable     = fcfs_get_claster_table(args, cid); //get cluster table
        int                 blist_len   = 0;
        int                 *blist      = get_blocks(ctable, fid, &blist_len); //get blocks id of file
        char                *blk_buf    = calloc(1, blist_len * lblk_sz);
        //read all blocks from cluster
        for(size_t j = 0; j < blist_len; ++j) {
            char *buf = fcfs_read_block(args, cid, blist[j]);
            memcpy(blk_buf + lblk_sz * j, buf, lblk_sz);
            free(buf);
        }
        //copy from local buffer to shared buffer
        if(dir_buf_len != 0) {
            char *tmp = calloc(1, dir_buf_len + lblk_sz * blist_len);
            memcpy(tmp, dir_buf, dir_buf_len);
            memcpy(tmp + dir_buf_len, blk_buf, lblk_sz * blist_len);
            free(dir_buf);
            free(blk_buf);
            dir_buf = tmp;
            dir_buf_len = dir_buf_len + lblk_sz * blist_len;
        }else{
            dir_buf = blk_buf;
            dir_buf_len = lblk_sz * blist_len;
        }
        free(ctable);
        free(blist);

        if(fid == 0)
            break;
    }
    DEBUG();
    int dir_entr_cnt = 0;
    memcpy(&dir_entr_cnt, dir_buf, sizeof(unsigned));
    *ret_sz = dir_entr_cnt;
    if(dir_entr_cnt == 0)
        return NULL;

    dir_list = calloc(1, sizeof(fcfs_dir_entry_t) * dir_entr_cnt);
    dir_buf_len -= sizeof(unsigned);

    memcpy(dir_list, dir_buf + sizeof(unsigned), sizeof(fcfs_dir_entry_t) * dir_entr_cnt);
    free(dir_buf);
    return dir_list;
}

int
fcfs_write_block(fcfs_args_t *args, int cid, int bid, char *data, int len) {
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
fcfs_write_directory(fcfs_args_t *args, int fid, fcfs_dir_entry_t *ent, int len) {
    DEBUG();
    int lblk_sz = args->fs_head->block_size * args->fs_head->phy_block_size;
    int blk_cnt = to_block_count(sizeof(unsigned) + len * sizeof(fcfs_dir_entry_t), lblk_sz);

    char *blk_buf = calloc(1, blk_cnt * lblk_sz);
    int cur_off_buf = 0;
    memcpy(blk_buf, &len, sizeof(unsigned));
    memcpy(blk_buf + sizeof(unsigned), ent, sizeof(fcfs_dir_entry_t) * len);
    DEBUG("lbk cnt  %d", blk_cnt);
    DEBUG("data len %lu", sizeof(fcfs_dir_entry_t) * len);

    size_t i = 0;
    for(;i < FCFS_MAX_CLASTER_COUNT_PER_FILE - 1; ++i) {
        int cid = args->fs_table->entrys[fid].clusters[i];
        if(fid != 0 && cid == 0)
            break;
        fcfs_block_list_t *bl = fcfs_get_claster_table(args, cid);
        int cblk_cnt = 0;
        int *cblks = get_blocks(bl, fid, &cblk_cnt);

        for(size_t j = 0; (j < cblk_cnt) && (cur_off_buf < lblk_sz * blk_cnt) ; ++j) {
            int bid = cblks[j];
            fcfs_write_block(args, cid, bid, blk_buf + cur_off_buf, lblk_sz);
            cur_off_buf += lblk_sz;
        }

        free(bl);
        free(cblks);
        if(fid == 0)
            break;
    }
    DEBUG();
    while(cur_off_buf < blk_cnt * lblk_sz && i != FCFS_MAX_CLASTER_COUNT_PER_FILE - 1) {
        int cl_free = fcfs_get_free_cluster(args);
        args->fs_table->entrys[fid].clusters[i] = cl_free;
        if(cl_free <= 0) {
            free(ent);
            free(blk_buf);
            return -1;
        }
        fcfs_block_list_t *bl = fcfs_get_claster_table(args, cl_free);
        int cblk_cnt = 0;
        int *cblks = get_blocks(bl, 0, &cblk_cnt);
        for(size_t j = 0; j < cblk_cnt; ++j) {
            bl->entrys[cblks[j] - 1].file_id = fid;
            int bid = cblks[j];
            fcfs_write_block(args, cl_free, bid, blk_buf + cur_off_buf, lblk_sz);
            cur_off_buf += lblk_sz;
        }
        i += 1;
        fcfs_write_block(args, cl_free, 0, (char*)bl, sizeof(fcfs_block_list_t)); //save cluster table
        if(update_bitmap(args, bl, cl_free)) {
            fcfs_write_bitmap(args);
        }
        free(cblks);
        free(bl);
    }
    if(i == FCFS_MAX_CLASTER_COUNT_PER_FILE - 1){
        free(ent);
        free(blk_buf);
        return -1;
    }
    DEBUG();
    fcfs_write_table(args);
    free(ent);
    free(blk_buf);
    return 0;
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
    int cid = fcfs_get_free_cluster(args);
    if(cid <= 0 ) {
        DEBUG("not enouth clusters");
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

int
fcfs_init_dir(fcfs_args_t *args, int fid) {
    DEBUG();
    fcfs_table_entry_t *tentry = &args->fs_table->entrys[fid];

    fcfs_dir_header_t dh;
    memset(&dh, 0, sizeof(fcfs_dir_header_t));

    int cid = tentry->clusters[0];
    fcfs_block_list_t *bl = fcfs_get_claster_table(args, cid);

    int b_len = 0;
    int *blks = get_blocks(bl, fid, &b_len);

    int res = fcfs_write_block(args, cid, blks[0], (char*)&dh, sizeof(fcfs_dir_header_t));

    free(bl);
    free(blks);
    return res;
}
