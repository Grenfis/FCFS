#include "../dev.h"

#include <stdlib.h>
#include <string.h>
#include <gcrypt.h>

#include <debug.h>

int
dev_write_bitmap(fcfs_args_t *args)
{
    DEBUG("");
    int lblk_sz = args->fs_head->phy_blk_sz * args->fs_head->blk_sz;
    int btm_blk_len = args->fs_head->bmp_len * lblk_sz;
    char *blk_buf = calloc(1, btm_blk_len);
    memcpy(blk_buf, args->fs_bitmap, args->fs_head->clu_cnt / 8.0);

    int res = fseek(args->dev, lblk_sz, SEEK_SET);
    if(res == 1L)
    {
        ERROR("seeking device");
        return -1;
    }

    res = fwrite(blk_buf, 1, btm_blk_len, args->dev);
    if(res != btm_blk_len)
    {
        ERROR("writing bitmap");
    }

    if(blk_buf != NULL)
        free(blk_buf);
    return 0;
}

int
dev_write_table(fcfs_args_t *args)
{
    DEBUG("");
    int lblk_sz = args->fs_head->phy_blk_sz * args->fs_head->blk_sz;
    int btm_blk_len = args->fs_head->bmp_len * lblk_sz;
    int tbl_blk_len = args->fs_head->tbl_len  * lblk_sz;

    char *blk_buf = calloc(1, tbl_blk_len);
    DEBUG("fcfs_table len %lu", sizeof(fcfs_table_t));
    memcpy(blk_buf, args->fs_table, sizeof(fcfs_table_t));

    int res = fseek(args->dev, lblk_sz + btm_blk_len, SEEK_SET);
    if(res == 1L)
    {
        ERROR("seeking device");
        return -1;
    }

    gcry_error_t err = gcry_cipher_encrypt(args->ciph, blk_buf, tbl_blk_len, blk_buf, tbl_blk_len);
    if(err)
        die("write fs table, %s", gcry_strerror(err));

    res = fwrite(blk_buf, 1, tbl_blk_len, args->dev);
    if(res != tbl_blk_len)
    {
        ERROR("writing table");
        return -1;
    }

    if(blk_buf != NULL)
        free(blk_buf);
    return 0;
}
