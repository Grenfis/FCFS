#ifndef IOCTL_DATA_H
#define IOCTL_DATA_H

#include <fcntl.h>
#include <unistd.h>
#include <linux/hdreg.h>

#define SCSI_CAPACITY   0
#define SCSI_INQR       1
#define SCSI_DATA       2


typedef struct dsc_info {
    unsigned char sn_len;   //serial number len
    unsigned char *sn;               //serial number value

    unsigned char vendor_len;
    unsigned char *vendor;

    unsigned char prod_len;
    unsigned char *product;

    unsigned char ver_len;
    unsigned char *ver;

    unsigned int sec_sz;    //size of one sector in bytes
    unsigned int sec_cnt;   //count of sectors
} dsc_info_t;

typedef struct scsi_cmd {
    unsigned char len;
    unsigned char *cmd;
} scsi_cmd_t;

int get_disc_info(const char *path, struct dsc_info *inf);

#endif
