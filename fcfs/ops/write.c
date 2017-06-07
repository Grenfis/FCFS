#include "common.h"

int
ops_write(const char *path, const char *data, size_t size, off_t offset, struct fuse_file_info *fi) {
    DEBUG("size %lu", size);
    DEBUG("offset %lu", offset);
    if(fi != NULL) {
        DEBUG("fid %lu", fi->fh);
    }

    offset += sizeof(fcfs_file_header_t);
    fcfs_args_t *args = fcfs_get_args();

    int fid = fi->fh;
    int lblk_sz = args->fs_head->phy_block_size * args->fs_head->block_size;

    int blk_num = offset / (double)lblk_sz;
    int blk_off = offset % lblk_sz;
    const int old_num = blk_num;

    int sz_cnt = to_block_count(blk_off + size, lblk_sz);

    int seq_sz = 0;
    dev_blk_info_t *inf = dev_get_file_seq(args, fid, &seq_sz);

    if(seq_sz < (blk_num + sz_cnt)) {
        int new_seq_sz = 0;
        dev_blk_info_t *blks = dev_free_blocks(args, (blk_num + sz_cnt) - seq_sz, &new_seq_sz);
        dev_file_reserve(args, fid, blks, new_seq_sz, inf[seq_sz - 1].num);
        dev_blk_info_t *tmp = calloc(1, (new_seq_sz + seq_sz) * sizeof(dev_blk_info_t));
        memcpy(tmp, inf, sizeof(dev_blk_info_t) * seq_sz);
        memcpy(tmp + seq_sz, blks, sizeof(dev_blk_info_t) * new_seq_sz);
        free(inf);
        free(blks);
        inf = tmp;
        seq_sz += new_seq_sz;
    }

    char tmp_buf[lblk_sz * sz_cnt];
    for(size_t i = 0; i < sz_cnt; ++i) {
        char buf[lblk_sz];
        dev_read_by_id(args, fid, blk_num, buf, lblk_sz);
        memcpy(tmp_buf + lblk_sz * i, buf, lblk_sz);
        blk_num++;
    }
    blk_num = old_num;
    memcpy(tmp_buf + blk_off, data, size);
    for(size_t i = 0; i < sz_cnt; ++i) {
        char buf[lblk_sz];
        memcpy(buf, tmp_buf + lblk_sz * i, lblk_sz);
        dev_write_by_id(args, fid, blk_num, buf, lblk_sz);
        blk_num++;
    }

    int sz = dev_file_size(args, fid);
    if(sz < (offset - sizeof(fcfs_file_header_t) + size)) {
        sz += (offset - sizeof(fcfs_file_header_t) + size) - sz;
    }
    dev_set_file_size(args, fid, sz);

    return size;
}
