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

int
ops_read(  const char *path,
            char *buf, size_t size,
            off_t offset,
            struct fuse_file_info *fi) {
    DEBUG("");
    return 0;
}