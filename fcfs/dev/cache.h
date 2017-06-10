#ifndef CACHE_H
#define CACHE_H

#include <time.h>

typedef struct {
    time_t  atime;
    int     fid;

    unsigned int s0_cl[FCFS_CLUSTER_PER_FILE - 3];
    unsigned int *s1_cl;
    unsigned int *s2_cl;
    //unsigned int ***s3_cl;

    unsigned int s0_cnt;
    unsigned int s1_cnt;
    unsigned int s2_cnt;
    //unsigned int s3_cnt;
} dev_clrs_cache_t;

#endif
