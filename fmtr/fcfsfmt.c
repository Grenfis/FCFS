#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include <errno.h>

#include <fcfs_structs.h>
#include <scsi.h>

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
    fs_head->phy_blk_sz = info->sec_sz;
    fs_head->phy_blk_cnt= info->sec_cnt;
    fs_head->fsid           = 1337; //magic!
    time(&fs_head->ctime);
    time(&fs_head->atime);
    fs_head->blk_sz     = FCFS_BLOCK_SIZE;
    fs_head->clu_sz   = FCFS_BLOKS_PER_CLUSTER;
    fs_head->tbl_cnt      = FCFS_TABLE_LEN;

    fs_head->blk_cnt    = (unsigned long)(fs_head->phy_blk_cnt / FCFS_BLOCK_SIZE);
    fs_head->tbl_len    = (unsigned short)ceil(sizeof(struct fcfs_table) /
    (fs_head->blk_sz * fs_head->phy_blk_sz)) + 1;
    fs_head->clu_cnt  = (unsigned long)floor((fs_head->blk_cnt
        - fs_head->tbl_len - 1)
        / fs_head->clu_sz); //needed for first counting

    fs_head->bmp_len   = (unsigned int)ceil((fs_head->clu_cnt / 8) / (double)(fs_head->blk_sz * fs_head->phy_blk_sz));
    fs_head->clu_cnt  = (unsigned long)floor((fs_head->blk_cnt
        - fs_head->tbl_len
        - fs_head->bmp_len - 1)
        / fs_head->clu_sz);

    memcpy(fs_head->label, info->product,
        info->prod_len);
    fs_head->dta_beg = 2 + fs_head->bmp_len + fs_head->tbl_len; //head + table + bitmap + 1
    print_fcfs_head(fs_head);
    /////////////////////prepare device///////////////////////
    FILE *device = fopen(argv[1], "wb"); //open device for writing
    if(device == NULL) {
        printf("[ERROR] when openning device! Code: %d Text: %s\n", errno, strerror(errno));
        exit(-1);
    }
    /////////////////////prepare fs header////////////////////
    int dta_blk = fs_head->blk_sz * fs_head->phy_blk_sz;
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
    int btm_sz = fs_head->bmp_len * dta_blk;
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
    int tbl_sz = fs_head->tbl_len * dta_blk;

    struct fcfs_table *fs_table = calloc(1, tbl_sz);
    struct fcfs_table_entry *tbl_entry = &fs_table->entrs[0]; //create root directory
    tbl_entry->lnk_cnt = 1;
    tbl_entry->clrs[0] = 0;

    res = fwrite(fs_table, 1, tbl_sz, device);
    if(res != tbl_sz) {
        printf("[ERROR] table write\n");
        exit(-1);
    }
    free(fs_table);
    printf("%d bytes writed.\n", res);
    ////////////////////prepare first block for root/////////
    printf("Root block table writing... ");
    char first_table[dta_blk];
    fcfs_block_list_t *bl = calloc(1, sizeof(fcfs_block_list_t));
    for(size_t i = 0; i < FCFS_BLOKS_PER_CLUSTER - 1; ++i) {
        bl->entrs[i].num = i;
    }
    memcpy(first_table, bl, sizeof(fcfs_block_list_t));

    fseek(device, fs_head->dta_beg * dta_blk, SEEK_SET);
    res = fwrite(first_table, 1, dta_blk, device);
    if(res != dta_blk) {
        printf("[ERROR] root block table write\n");
        exit(-1);
    }
    free(bl);
    printf("%d bytes writed.\n", res);
    /////////////////////root cluster////////////////////////
    printf("Cleaning root cluster... ");
    char clu[dta_blk * (FCFS_BLOKS_PER_CLUSTER - 1)];
    memset(clu, 0, dta_blk * (FCFS_BLOKS_PER_CLUSTER - 1));

    res = fwrite(clu, 1, dta_blk * (FCFS_BLOKS_PER_CLUSTER - 1), device);
    if(res != dta_blk * (FCFS_BLOKS_PER_CLUSTER - 1)) {
        printf("[ERROR] cleaning root cluster\n");
        exit(-1);
    }
    printf("%d bytes writed.\n", dta_blk * (FCFS_BLOKS_PER_CLUSTER - 1));
    ////////////////////dispose resources////////////////////
    fclose(device);
    free(fs_head);
    return 0;
}

void print_fcfs_head(struct fcfs_head *h){
    if(h == NULL)
        return;
    printf("======================================\n");
    printf("FS ID                   : %u\n", h->fsid);
    printf("Label                   : %s\n", h->label);
    printf("Create date             : %s", asctime(localtime(&h->ctime)));
    printf("Access date             : %s", asctime(localtime(&h->atime)));
    printf("Table entyers count     : %u\n", h->tbl_cnt);
    printf("Table size in blocks    : %u\n", h->tbl_len);
    printf("Bitmap size in blocks   : %u\n", h->bmp_len);
    printf("Cluster size            : %u\n", h->clu_sz);
    printf("Cluster count           : %lu\n", h->clu_cnt);
    printf("Block size              : %u\n", h->blk_sz);
    printf("Block count             : %lu\n", h->blk_cnt);
    printf("Physical block size     : %u\n", h->phy_blk_sz);
    printf("Physical block count    : %lu\n", h->phy_blk_cnt);
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
