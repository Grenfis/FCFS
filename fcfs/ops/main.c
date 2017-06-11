#include "common.h"
#include "cache.h"

void *
ops_init(struct fuse_conn_info *conn, struct fuse_config *cfg)
{
    DEBUG("");
    fcfs_args_t *args = fcfs_get_args();
    args->dev = fopen(args->p_dev, "w+b");

    int res = dev_mount(args);
    if(res != 0)
    {
        ERROR("when mounting");
        exit(-1);
    }

    cache_init();
    return (void*)args;
}

void
ops_destroy(void *a)
{
    DEBUG("");
    fcfs_args_t *args = (fcfs_args_t*)a;
    cache_destroy();
    free(args->p_dev);
    DEBUG("deleted p_dev");
    free(args->passwd);
    DEBUG("deleted passwd");
    free(args->fs_head);
    DEBUG("deleted fs_head");
    free(args->fs_bitmap);
    DEBUG("deleted fs_bitmap");
    free(args->fs_table);
    DEBUG("deleted fs_table");
    fclose(args->dev);
    DEBUG("deleted dev");
}
