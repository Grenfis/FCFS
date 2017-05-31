#include "scsi.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <scsi/scsi.h>
#include <scsi/sg.h>
#include <sys/ioctl.h>

// Creates SCSI command by number
// type - number of command
struct scsi_cmd *get_scsi_cmd(unsigned int type) {
    struct scsi_cmd *cmd = calloc(1, sizeof(struct scsi_cmd));
    switch(type) {
        case SCSI_CAPACITY:
            cmd->len = 10;
            cmd->cmd = calloc(1, 10);
            cmd->cmd[0] = 0x25;
            break;
        case SCSI_INQR:
            cmd->len = 6;
            cmd->cmd = calloc(1, 6);
            cmd->cmd[0] = 0x12;
            cmd->cmd[1] = 0x01;
            cmd->cmd[2] = 0x80;
            cmd->cmd[4] = 0x0;
            break;
        case SCSI_DATA:
            cmd->len = 6;
            cmd->cmd = calloc(1, 6);
            cmd->cmd[0] = 0x12;
            cmd->cmd[4] = 0xff;
            break;
        default:
            return NULL;
    }
    return cmd;
}

// Send SCSI command to device
// fd       - file descriptor of device
// cmd      - command
// buf      - buffer for resolve
// buf_len  - length of buffer
static int do_scsi_cmd(int fd, struct scsi_cmd *cmd, unsigned char *buf, unsigned short buf_len) {
    if(!cmd)
        return -1;

    unsigned char sense[32];
    struct sg_io_hdr hdr;
    int res;

    memset(&hdr, 0, sizeof(hdr));
    hdr.interface_id    = 'S';
    hdr.cmdp            = cmd->cmd;
    hdr.cmd_len         = cmd->len;
    hdr.dxferp          = buf;
    hdr.dxfer_len       = buf_len;
    hdr.dxfer_direction = SG_DXFER_FROM_DEV;
    hdr.sbp             = sense;
    hdr.mx_sb_len       = sizeof(sense);
    hdr.timeout         = 5000;

    res = ioctl(fd, SG_IO, &hdr);

    if(res < 0)
        return res;

    if((hdr.info & SG_INFO_OK_MASK) != SG_INFO_OK)
        return 1;

    free(cmd->cmd);
    free(cmd);
    return 0;
}

// swaps two unsigned bytes
// a, b - bytes to swap
void swap_byte(unsigned char *a, unsigned char *b) {
    unsigned char tmp = *a;
    *a = *b;
    *b = tmp;
}

// fill the dsc_info field with given data
// f        - pointer to string-field
// f_len    - pointer to data length-fiel
// buf      - buffer with data
// start    - start position in buffer
// len      - length to read from buffer
void fill_field(unsigned char **f, unsigned char *f_len, unsigned char buf[],
    unsigned char start, unsigned char len) {
    *f_len = len;
    *f = calloc(1, len);
    memcpy(*f, &buf[start], len);
}

void get_sn(int drv, unsigned char *buf, unsigned short buf_len, struct dsc_info *inf) {
    int res = do_scsi_cmd(drv, get_scsi_cmd(SCSI_INQR), buf, buf_len);
    if(res != 0) {
        printf("[ERROR] get volume S/N\n");
        exit(-1);
    }
    inf->sn_len = buf[3];
    inf->sn = calloc(1, inf->sn_len);
    memcpy(inf->sn, &buf[4], inf->sn_len);
}

// Returns struct with info about given drive
// path - path to file of device
// inf  - an empty struct
int get_disc_info(const char *path, struct dsc_info *inf) {
    int res = 0;

    int drv = open(path, O_RDONLY);
    if(!drv) {
        printf("[ERROR] open drv\n");
        exit(-1);
    }
    //create buf
    const unsigned short buf_len = 512;
    unsigned char buf[buf_len];
    memset(&buf[0], 0, buf_len);

    //get SN
    get_sn(drv, buf, buf_len, inf);

    //get vendor data
    res = do_scsi_cmd(drv, get_scsi_cmd(SCSI_DATA), buf, buf_len);
    if(res != 0) {
        printf("[ERROR] get vendor data\n");
        exit(-1);
    }
    //get vendor
    fill_field(&inf->vendor, &inf->vendor_len, buf, 8, 8);

    //get product
    fill_field(&inf->product, &inf->prod_len, buf, 16, 16);

    //get ver
    fill_field(&inf->ver, &inf->ver_len, buf, 32, 8);

    //get capacity
    res = do_scsi_cmd(drv, get_scsi_cmd(SCSI_CAPACITY), buf, buf_len);
    if(res != 0) {
        printf("[ERROR] get volume capacity\n");
        exit(-1);
    }
    //becouse big endian
    //for sec cnt
    swap_byte(&buf[0], &buf[3]);
    swap_byte(&buf[1], &buf[2]);
    //for sec sz
    swap_byte(&buf[4], &buf[7]);
    swap_byte(&buf[5], &buf[6]);
    //fill data
    memcpy(&inf->sec_cnt, &buf[0], 4);
    memcpy(&inf->sec_sz, &buf[4], 4);

    close(drv);
    return res;
}
