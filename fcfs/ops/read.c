#include "common.h"

int
ops_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    DEBUG("path %s", path);
    DEBUG("size %lu", size);
    DEBUG("offset %lu", offset);
    if(fi != NULL) {
        DEBUG("fid %lu", fi->fh);
    }

    fcfs_args_t *args = fcfs_get_args();

    int fid = fi->fh;
    int fsize = dev_file_size(args, fid);
    if(fsize < (offset + size)) {
        size += fsize - (offset + size); //always be < 0
        if(size <= 0)
            return size;
    }

    int lblk_sz = args->fs_head->phy_block_size * args->fs_head->block_size;
    offset += sizeof(fcfs_file_header_t);

    int blk_num = offset / (double)lblk_sz;
    int blk_off = offset % lblk_sz;
    int sz_cnt = to_block_count(blk_off + size, lblk_sz);

    for(size_t i = 0; i < sz_cnt; ++i) {
        char tmp_buf[lblk_sz];
        dev_read_by_id(args, fid, blk_num, tmp_buf, lblk_sz);
        memcpy(buf + i * lblk_sz, tmp_buf + blk_off, lblk_sz - blk_off);
        blk_off = 0;
    }

    return size;
}
