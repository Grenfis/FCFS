#ifndef FCFS_STRUCTS_H
#define FCFS_STRUCTS_H

#include <time.h>

#define FCFS_LABEL_LENGTH               (unsigned char) 255
#define FCFS_MAX_CLASTER_COUNT_PER_FILE (unsigned char) 8
#define FCFS_BLOKS_PER_CLUSTER          (unsigned short)8
#define FCFS_TABLE_COUNT                (unsigned short)65535
#define FCFS_BLOCK_SIZE                 (unsigned char)2
#define FCFS_MAX_FILE_NAME_LENGTH       (unsigned char) 32
#define FCFS_DEFAULT_DRIVE_NAME         "FCFS_Volume"

#define FCFS_MIN_PASSWORD_LEN           (unsigned char)6

typedef struct fcfs_head{
    unsigned        fsid; //magic
    char            label[FCFS_LABEL_LENGTH]; //fs label

    time_t          create_date;
    time_t          access_date; //last access date

    unsigned        table_len; //count of entyers
    unsigned short  table_count; //blocks count
    unsigned        bitmap_count; //blocks count
    unsigned short  cluster_size;
    unsigned long   cluster_count;
    unsigned short  block_size;
    unsigned long   block_count;
    unsigned short  phy_block_size;
    unsigned long   phy_block_count;
    unsigned        dta_beg; //number of first data block

    unsigned short  file_count;
} fcfs_head_t;

typedef struct fcfs_table_entry {
    unsigned char   link_count; //hard link count
    unsigned short  clusters[FCFS_MAX_CLASTER_COUNT_PER_FILE];
} fcfs_table_entry_t;

typedef struct fcfs_table {
    struct          fcfs_table_entry entrys[FCFS_TABLE_COUNT];
} fcfs_table_t;
//---------------------------------------CLUSTER-----------------------------------
typedef struct fcfs_block_list_entry {
    unsigned        file_id;
    unsigned        num;
} fcfs_block_list_entry_t;

typedef struct fcfs_block_list {
    struct          fcfs_block_list_entry entrys[FCFS_BLOKS_PER_CLUSTER - 1];
} fcfs_block_list_t;
//-----------------------------------DATA--------------------------------
typedef struct fcfs_file_header{
    unsigned        file_size;
} fcfs_file_header_t;

typedef struct fcfs_dir_entry{
    unsigned short  file_id;
    unsigned        mode; //access attrs
    time_t          create_date;
    time_t          access_date;
    time_t          change_date;
    char            name[FCFS_MAX_FILE_NAME_LENGTH];
} fcfs_dir_entry_t;

#endif
