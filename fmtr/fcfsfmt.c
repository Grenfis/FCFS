#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <gcrypt.h>

#include <errno.h>

#include <fcfs_structs.h>
#include <scsi.h>
#include <debug.h>
#include <utils.h>

#define PASSWD_BUF_LEN 1024
#define MD_LEN 32

FILE *device;

gcry_cipher_hd_t    hd;
gcry_error_t        err;

static void print_fcfs_head(struct fcfs_head *h);
static void print_dsc_info(struct dsc_info *h);
static void get_passwd();
static void proc_opt(int argc, char *argv[]);

int main(int argc, char *argv[]) {
    if( argc < 2 ) {
        printf("[ERROR] Please specify file path\n");
        exit(-1);
    }

    proc_opt(argc, argv);

    struct dsc_info *info = calloc(1, sizeof(struct dsc_info));
    get_disc_info(argv[1], info);
    print_dsc_info(info);

    struct fcfs_head *fs_head = malloc(sizeof(struct fcfs_head)); //fill fs header with data
    fs_head->phy_blk_sz = info->sec_sz;
    fs_head->phy_blk_cnt= info->sec_cnt;
    fs_head->fsid           = 1337; //magic!
    time(&fs_head->ctime);
    time(&fs_head->atime);
    fs_head->hashsum    = sizeof(fcfs_head_t) ^ fs_head->ctime;
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

    err = gcry_cipher_encrypt(hd, head_block, dta_blk, head_block, dta_blk);
    if(err)
        die("head, %s", gcry_strerror(err));

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
    fs_table->hashsum = sizeof(fcfs_table_t) ^ fs_head->ctime;
    struct fcfs_table_entry *tbl_entry = &fs_table->entrs[0]; //create root directory
    tbl_entry->lnk_cnt = 1;
    tbl_entry->clrs[0] = 0;

    err = gcry_cipher_encrypt(hd, fs_table, tbl_sz, fs_table, tbl_sz);
    if(err)
        die("table, %s", gcry_strerror(err));

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
    bl->hashsum ^= fs_head->ctime;
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
    int clu_sz = dta_blk * (FCFS_BLOKS_PER_CLUSTER - 1);
    char clu[clu_sz];
    memset(clu, 0, clu_sz);

    err = gcry_cipher_encrypt(hd, clu, clu_sz, clu, clu_sz);
    if(err)
        die("root cluster write, %s", gcry_strerror(err));

    res = fwrite(clu, 1, clu_sz, device);
    if(res != clu_sz) {
        printf("[ERROR] cleaning root cluster\n");
        exit(-1);
    }
    printf("%d bytes writed.\n", dta_blk * (FCFS_BLOKS_PER_CLUSTER - 1));
    ////////////////////dispose resources////////////////////
    fclose(device);
    free(fs_head);
    gcry_cipher_close(hd);
    return 0;
}

static void print_fcfs_head(struct fcfs_head *h){
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
    printf("Hashsum                 : %lu\n", h->hashsum);
    printf("======================================\n");
}

static void print_dsc_info(struct dsc_info *h) {
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

static void open_ciph(const char *md)
{
    char *pass = malloc(MD_LEN / 2);
    char *iv   = malloc(MD_LEN / 2);

    memcpy(pass, md, MD_LEN / 2);
    memcpy(iv, md + MD_LEN / 2, MD_LEN / 2);

    err = gcry_cipher_open(&hd, GCRY_CIPHER_RIJNDAEL, GCRY_CIPHER_MODE_ECB, 0);
    if(err)
        die("open cipher, %s", gcry_strerror(err));

    err = gcry_cipher_setkey(hd, pass, MD_LEN / 2);
    if(err)
        die("set key, %s", gcry_strerror(err));

    err = gcry_cipher_setiv(hd, iv, MD_LEN / 2);
    if(err)
        die("set iv, %s", gcry_strerror(err));
}

static char *get_md(const char *str)
{
    char *ret = malloc(MD_LEN);
    gcry_md_hash_buffer(GCRY_MD_SHA256, ret, str, strlen(str));
    return ret;
}

static void get_passwd()
{
    char *buf = calloc(1, PASSWD_BUF_LEN);
    unsigned char c_buf_pos = 0;
    char c = '\0';
    char p_req = 1;
    printf("Enter password:\n");
    while(p_req)
    {
        switch(c = getch())
        {
            case '\n':
                if(c_buf_pos >= FCFS_MIN_PASSWORD_LEN)
                {
                    p_req = 0;
                }
                else
                {
                    printf("Password too short!\n");
                }
                break;
            default:
                buf[c_buf_pos] = c;
                break;
        }
        c_buf_pos++;
    }
    char *tmp = get_md(buf);
    open_ciph(tmp);
    free(tmp);
    if(buf != NULL)
        free(buf);
}

static void proc_opt(int argc, char *argv[]) {
    char *pass = NULL;
    for(size_t i = 1; i < argc; ++i) {
        if(argv[i][0] != '-')
            continue;
        switch(argv[i][1]) {
            case 'p':
                {
                    pass = malloc(strlen(&argv[i][2]));
                    strcpy(pass, &argv[i][2]);
                    char *tmp = get_md(pass);
                    open_ciph(tmp);
                    free(tmp);
                }
                break;
            default:
                break;
        }
    }
    if(pass == NULL) {
        get_passwd();
    }else{
        free(pass);
    }
}
