#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include <errno.h>

#include <fcfs_structs.h>
#include <scsi.h>

struct stat *file_info; //file info buffer

FILE *device;

void print_fcfs_head(struct fcfs_head *h);
void print_dsc_info(struct dsc_info *h);

int main(int argc, char *argv[]) {
    if( argc < 2 ) {
        printf("[ERROR] Please specify file path\n");
        exit(-1);
    }

    struct dsc_info *info = calloc(1, sizeof(struct dsc_info));
    get_disc_info(argv[1], info);
    print_dsc_info(info);

    struct fcfs_head *fs_head = malloc(sizeof(struct fcfs_head)); //fill fs header with data
    fs_head->phy_block_size = info->sec_sz;
    fs_head->phy_block_count= info->sec_cnt;
    fs_head->fsid           = 1337; //magic!
    time(&fs_head->create_date);
    time(&fs_head->access_date);
    fs_head->block_size     = FCFS_BLOCK_SIZE;
    fs_head->cluster_size   = FCFS_BLOKS_PER_CLUSTER;
    fs_head->table_len      = FCFS_TABLE_COUNT;
    fs_head->file_count     = 1; //for root dir

    fs_head->block_count    = (unsigned long)(fs_head->phy_block_count / FCFS_BLOCK_SIZE);
    fs_head->table_count    = (unsigned short)ceil(sizeof(struct fcfs_table) /
    (fs_head->block_size * fs_head->phy_block_size)) + 1;
    fs_head->cluster_count  = (unsigned long)floor((fs_head->block_count
        - fs_head->table_count - 1)
        / fs_head->cluster_size); //needed for first counting

    fs_head->bitmap_count   = (unsigned int)ceil((fs_head->cluster_count / 8) / (double)(fs_head->block_size * fs_head->phy_block_size));
    fs_head->cluster_count  = (unsigned long)floor((fs_head->block_count
        - fs_head->table_count
        - fs_head->bitmap_count - 1)
        / fs_head->cluster_size);

    memcpy(fs_head->label, info->product,
        info->prod_len);
    fs_head->dta_beg = 2 + fs_head->bitmap_count + fs_head->table_count; //head + table + bitmap + 1
    print_fcfs_head(fs_head);
    /////////////////////prepare device///////////////////////
    FILE *device = fopen(argv[1], "wb"); //open device for writing
    if(device == NULL) {
        printf("[ERROR] when openning device!\n");
        exit(-1);
    }
    /////////////////////prepare fs header////////////////////
    int dta_blk = fs_head->block_size * fs_head->phy_block_size;
    printf("Header writing... ");
    char *head_block = calloc(1, dta_blk);
    memcpy(head_block, fs_head, sizeof(struct fcfs_head));
    int res = fwrite(head_block, 1, dta_blk, device);
    if(res != dta_blk) {
        printf("[ERROR] header write\n");
        exit(-1);
    }
    free(head_block);
    printf("%d bytes writed.\n", res);
    ////////////////////prepare bitmap///////////////////////
    printf("Bitmap writing... ");
    int btm_sz = fs_head->bitmap_count * dta_blk;
    unsigned char *bitmap_buffer = calloc(1, btm_sz);
    res = fwrite(bitmap_buffer, 1, btm_sz, device);
    if(res != btm_sz) {
        printf("[ERROR] bitmap write\n");
        exit(-1);
    }
    free(bitmap_buffer);
    printf("%d bytes writed.\n", res);
    //////////////////prepare table/////////////////////////
    printf("Table writing... ");
    int tbl_sz = fs_head->table_count * dta_blk;

    struct fcfs_table *fs_table = calloc(1, tbl_sz);
    struct fcfs_table_entry *tbl_entry = &fs_table->entrys[0]; //create root directory
    tbl_entry->link_count = 1;
    tbl_entry->clusters[0] = 0;

    res = fwrite(fs_table, 1, tbl_sz, device);
    if(res != tbl_sz) {
        printf("[ERROR] table write\n");
        exit(-1);
    }
    free(fs_table);
    printf("%d bytes writed.\n", res);
    ////////////////////dispose resources////////////////////
    fclose(device);
    free(fs_head);
    free(file_info);
    return 0;
}

void print_fcfs_head(struct fcfs_head *h){
    if(h == NULL)
        return;
    printf("======================================\n");
    printf("FS ID                   : %u\n", h->fsid);
    printf("Label                   : %s\n", h->label);
    printf("Create date             : %s", asctime(localtime(&h->create_date)));
    printf("Access date             : %s", asctime(localtime(&h->access_date)));
    printf("Table entyers count     : %u\n", h->table_len);
    printf("Table size in blocks    : %u\n", h->table_count);
    printf("Bitmap size in blocks   : %u\n", h->bitmap_count);
    printf("Cluster size            : %u\n", h->cluster_size);
    printf("Cluster count           : %lu\n", h->cluster_count);
    printf("Block size              : %u\n", h->block_size);
    printf("Block count             : %lu\n", h->block_count);
    printf("Physical block size     : %u\n", h->phy_block_size);
    printf("Physical block count    : %u\n", h->phy_block_count);
    printf("File count              : %u\n", h->file_count);
    printf("First data block at     : %u\n", h->dta_beg);
    printf("======================================\n");
}

void print_dsc_info(struct dsc_info *h) {
    if(h == NULL)
        return;
    printf("======================================\n");
    //printf("S\\N len                     : %u\n", h->sn_len);
    printf("S\\N                     : ");
    for(size_t i = 0; i < h->sn_len; ++i) {
        putc(h->sn[i], stdout);
    }
    putc('\n', stdout);

    printf("Vendor                  : ");
    for(size_t i = 0; i < h->vendor_len; ++i) {
        putc(h->vendor[i], stdout);
    }
    putc('\n', stdout);

    printf("Product                 : ");
    for(size_t i = 0; i < h->prod_len; ++i) {
        putc(h->product[i], stdout);
    }
    putc('\n', stdout);

    printf("Version                 : ");
    for(size_t i = 0; i < h->ver_len; ++i) {
        putc(h->ver[i], stdout);
    }
    putc('\n', stdout);

    printf("Sector size             : %u\n", h->sec_sz);
    printf("Sectors count           : %u\n", h->sec_cnt);
}
