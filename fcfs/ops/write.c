#include "common.h"

int
ops_write(const char *path, const char *data, size_t size, off_t offset, struct fuse_file_info *fi) {
    DEBUG("size %lu", size);
    DEBUG("offset %lu", offset);
    if(fi != NULL) {
        DEBUG("fid %lu", fi->fh);
        if(fi->fh == 0) {
            ops_open(path, fi);
        }
    }

    offset += sizeof(fcfs_file_header_t);
    fcfs_args_t *args = fcfs_get_args();

    int fid = fi->fh;
    int lblk_sz = args->fs_head->phy_blk_sz * args->fs_head->blk_sz;

    int blk_num = offset / (double)lblk_sz;
    int blk_off = offset % lblk_sz;

    int sz_cnt = to_blk_cnt(blk_off + size, lblk_sz);

    int seq_sz = 0;
    dev_blk_info_t *inf = dev_get_file_seq(args, fid, &seq_sz);

    if(seq_sz < (blk_num + sz_cnt)) {
        seq_sz = dev_extd_blk_list(args, &inf, seq_sz, (blk_num + sz_cnt) - seq_sz, fid);
    }

    char tmp_buf[lblk_sz * sz_cnt];
    for(size_t i = 0; i < sz_cnt; ++i) {
        dev_read_by_id(args, fid, blk_num + i, tmp_buf + lblk_sz * i, lblk_sz);
    }
    memcpy(tmp_buf + blk_off, data, size);
    for(size_t i = 0; i < sz_cnt; ++i) {
        dev_write_by_id(args, fid, blk_num + i, tmp_buf + lblk_sz * i, lblk_sz);
    }

    int sz = dev_file_size(args, fid);
    if(sz < (offset - sizeof(fcfs_file_header_t) + size)) {
        sz += (offset - sizeof(fcfs_file_header_t) + size) - sz;
    }
    dev_set_file_size(args, fid, sz);

    return size;
}
