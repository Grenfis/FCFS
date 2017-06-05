#include "../ops.h"
#include "../fcfs.h"
#include "../dev.h"
#include "../utils.h"
#include "common.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fuse3/fuse.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <debug.h>

void *
fcfs_init(struct fuse_conn_info *conn, struct fuse_config *cfg) {
    DEBUG("");
    fcfs_args_t *args = fcfs_get_args();
    args->dev = fopen(args->p_dev, "w+b");

    int res = fcfs_mount(args);
    if(res != 0) {
        ERROR("when mounting");
        exit(-1);
    }

    return (void*)args;
}

void
fcfs_destroy(void *a) {
    DEBUG("");
    fcfs_args_t *args = (fcfs_args_t*)a;
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
