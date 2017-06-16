#include "../dev.h"

#include <stdlib.h>
#include <string.h>
#include <scsi.h>
#include <gcrypt.h>

#include <debug.h>

static fcfs_head_t *
read_head(fcfs_args_t *args, FILE *dev, int l_blk_sz)
{
    DEBUG("");
    char *head_buf = calloc(1, l_blk_sz);
    fcfs_head_t *head = calloc(1, sizeof(fcfs_head_t));
    int res = fread(head_buf, 1, l_blk_sz, dev);
    if(res != l_blk_sz)
    {
        ERROR("can not read fs head from dev");
        exit(-1);
    }

    gcry_error_t err = gcry_cipher_decrypt(args->ciph, head_buf, l_blk_sz, head_buf, l_blk_sz);
    if(err)
        die("read fs header, %s", gcry_strerror(err));

    memcpy(head, head_buf, sizeof(fcfs_head_t));
    if(head->hashsum != (sizeof(fcfs_head_t) ^ head->ctime))
    {
        ERROR("bad head hashsum");
        exit(-1);
    }
    if(head_buf != NULL)
        free(head_buf);
    return head;
}

static unsigned char *
read_bitmap(FILE *dev, int bytes)
{
    DEBUG("");
    unsigned char *bitmap = calloc(1, bytes);
    int res = fread(bitmap, 1, bytes, dev);
    if(res != bytes)
    {
        ERROR("can not read bitmap from dev");
        exit(-1);
    }
    return bitmap;
}

static fcfs_table_t *
read_table(fcfs_args_t *args, FILE *dev, int bytes, fcfs_head_t *head)
{
    DEBUG("");
    char *table_buf = calloc(1, bytes);
    fcfs_table_t *table = calloc(1, sizeof(fcfs_table_t));
    int res = fread(table_buf, 1, bytes, dev);
    if(res != bytes)
    {
        ERROR("can not read table from dev");
        exit(-1);
    }

    gcry_error_t err = gcry_cipher_decrypt(args->ciph, table_buf, bytes, table_buf, bytes);
    if(err)
        die("read fs table, %s", gcry_strerror(err));

    memcpy(table, table_buf, sizeof(fcfs_table_t));
    if(table->hashsum != (sizeof(fcfs_table_t) ^ head->ctime))
    {
        ERROR("bad table heashsum");
        exit(-1);
    }
    if(table_buf != NULL)
        free(table_buf);
    return table;
}

int
dev_mount(fcfs_args_t *args)
{
    DEBUG("");

    dsc_info_t *d_info = calloc(1, sizeof(dsc_info_t));
    int res = get_disc_info(args->p_dev, d_info);
    if(res != 0)
    {
        ERROR("could not get info about drive");
        return -1;
    }

    int l_blk_sz = d_info->sec_sz * FCFS_BLOCK_SIZE;
    if(d_info != NULL)
        free(d_info);

    res = fseek(args->dev, 0, SEEK_SET);
    if(res == 1L)
    {
        ERROR("seeking to beginning device");
        return -1;
    }

    fcfs_head_t *f_head = read_head(args, args->dev, l_blk_sz);
    args->fs_head   = f_head;
    args->fs_bitmap = read_bitmap(args->dev,    f_head->phy_blk_sz *
                                                f_head->blk_sz *
                                                f_head->bmp_len);

    args->fs_table  = read_table(args, args->dev,   f_head->phy_blk_sz *
                                                    f_head->blk_sz *
                                                    f_head->tbl_len,
                                                    f_head);

    DEBUG("fsid                 %u",    f_head->fsid);
    DEBUG("label                %s",    f_head->label);
    DEBUG("phy blk sz           %u",    f_head->phy_blk_sz);
    DEBUG("phy blk cnt          %lu",   f_head->phy_blk_cnt);
    DEBUG("log blk sz           %u",    f_head->blk_sz);
    DEBUG("log blk cnt          %lu",   f_head->blk_cnt);
    DEBUG("bitmap len           %u",    f_head->bmp_len);
    DEBUG("table count          %u",    f_head->tbl_len);

    return 0;
}
