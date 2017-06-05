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
fcfs_mkdir(const char *path, mode_t mode) {
    DEBUG("path = %s", path);
    DEBUG("mode = %d", mode);
    fcfs_args_t *args = fcfs_get_args();
    //get directory content
    int dirs_len = 0;
    int pfid = fcfs_get_pfid(path);
    fcfs_dir_entry_t *dirs = fcfs_read_directory(args, pfid, &dirs_len);
    fcfs_dir_entry_t *tmp = NULL;

    int new_pos = -1;
    for(size_t i = 0; i < dirs_len; ++i) {
        if(strlen(dirs[i].name) == 0) {
            new_pos = i;
            break;
        }
    }

    if(new_pos != -1) {
        tmp = dirs;
    }else{
        dirs_len += 1;
        tmp = calloc(1, sizeof(fcfs_dir_entry_t) * dirs_len);
        memcpy(tmp, dirs, sizeof(fcfs_dir_entry_t) * (dirs_len - 1));
        free(dirs);
        new_pos = dirs_len - 1;
    }

    int new_fid = fcfs_get_free_fid(args);
    if(new_fid <= 0) {
        return -ENOENT;
    }

    fcfs_alloc(args, new_fid);
    fcfs_init_dir(args, new_fid);
    tmp[new_pos].file_id       = new_fid; //set free file id
    tmp[new_pos].mode          = 0040000 | mode;
    tmp[new_pos].create_date   = time(NULL);
    tmp[new_pos].access_date   = time(NULL);
    tmp[new_pos].change_date   = time(NULL);

    int p_len = get_parrent_path(path);
    p_len += 1;
    strcpy(tmp[new_pos].name, path + p_len);

    fcfs_write_directory(args, pfid, tmp, dirs_len);

    return 0;
}
