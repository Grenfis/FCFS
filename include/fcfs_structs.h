#ifndef FCFS_STRUCTS_H
#define FCFS_STRUCTS_H

#include <time.h>

#define FCFS_LABEL_LENGTH               (unsigned char)     255
#define FCFS_CLUSTER_PER_FILE           (unsigned char)     16
#define FCFS_BLOKS_PER_CLUSTER          (unsigned short)    8
#define FCFS_TABLE_LEN                  (unsigned short)    65535
#define FCFS_BLOCK_SIZE                 (unsigned char)     2
#define FCFS_MAX_FILE_NAME_LENGTH       (unsigned char)     32
#define FCFS_DEFAULT_DRIVE_NAME         "FCFS_Volume"

#define FCFS_MIN_PASSWORD_LEN           (unsigned char)6

typedef struct fcfs_head{
    unsigned        fsid; //magic
    char            label[FCFS_LABEL_LENGTH]; //fs label

    time_t          ctime;
    time_t          atime; //last access date

    unsigned        tbl_cnt; //count of entyers
    unsigned short  tbl_len; //blocks count
    unsigned        bmp_len; //blocks count
    unsigned short  clu_sz;
    unsigned long   clu_cnt;
    unsigned short  blk_sz;
    unsigned long   blk_cnt;
    unsigned short  phy_blk_sz;
    unsigned long   phy_blk_cnt;
    unsigned        dta_beg; //number of first data block
} fcfs_head_t;

typedef struct fcfs_table_entry {
    unsigned char   lnk_cnt; //hard link count
    unsigned short  clrs[FCFS_CLUSTER_PER_FILE];
} fcfs_table_entry_t;

typedef struct fcfs_table {
    struct          fcfs_table_entry entrs[FCFS_TABLE_LEN];
} fcfs_table_t;
//---------------------------------------CLUSTER-----------------------------------
typedef struct fcfs_block_list_entry {
    unsigned        fid;
    unsigned        num;
} fcfs_block_list_entry_t;

typedef struct fcfs_block_list {
    int             hashsum;
    struct          fcfs_block_list_entry entrs[FCFS_BLOKS_PER_CLUSTER - 1];
} fcfs_block_list_t;
//-----------------------------------DATA--------------------------------
typedef struct fcfs_file_header{
    unsigned        f_sz;
} fcfs_file_header_t;

typedef struct fcfs_dir_entry{
    unsigned short  fid;
    unsigned        mode; //access attrs
    time_t          ctime;
    time_t          atime;
    time_t          mtime;
    char            name[FCFS_MAX_FILE_NAME_LENGTH];
} fcfs_dir_entry_t;

#endif
