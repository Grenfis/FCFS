#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#include <fuse3/fuse.h>
#include <string.h>
#include <errno.h>
#include <utils.h>

#include <fcfs_structs.h>

#include "fcfs.h"
#include "debug.h"
#include "ops.h"

#define PASSWD_BUF_LEN 1024
#define MD_LEN 32

enum {
    FCFS_OPT_VER = 0,
    FCFS_OPT_HELP,
    FCFS_OPT_PASSWD,
    FCFS_OPT_DEV
};

static struct fuse_operations fcfs_ops = {
    .getattr    = ops_getattr,
    .readdir    = ops_readdir,
    .mkdir      = ops_mkdir,
    .rmdir      = ops_rmdir,
    .open       = ops_open,
    .read       = ops_read,
    .write      = ops_write,
    .init       = ops_init,
    .destroy    = ops_destroy,
    .create     = ops_create,
    .unlink     = ops_unlink,
    .utimens    = ops_utimens,
    .chown      = ops_chown,
    .truncate   = ops_truncate,
    .flush      = ops_flush,
    .opendir    = ops_opendir,
    .releasedir = ops_releasedir,
    .release    = ops_release,
}; //file system operations

static struct fuse_opt fcfs_opt[] = {
    FUSE_OPT_KEY("-V", FCFS_OPT_VER),
    FUSE_OPT_KEY("--version", FCFS_OPT_VER),
    FUSE_OPT_KEY("-h", FCFS_OPT_HELP),
    FUSE_OPT_KEY("--help", FCFS_OPT_HELP),
    FUSE_OPT_KEY("-p %s", FCFS_OPT_PASSWD),
    FUSE_OPT_KEY("-b %s", FCFS_OPT_DEV)
};

static unsigned char need_pass = 1;

static void
open_ciph(fcfs_args_t *args, const char *md)
{
    gcry_error_t err;
    char *pass = malloc(MD_LEN / 2);
    char *iv   = malloc(MD_LEN / 2);

    memcpy(pass, md, MD_LEN / 2);
    memcpy(iv, md + MD_LEN / 2, MD_LEN / 2);

    err = gcry_cipher_open(&args->ciph, GCRY_CIPHER_RIJNDAEL, GCRY_CIPHER_MODE_ECB, 0);
    if(err)
        die("open cipher");

    err = gcry_cipher_setkey(args->ciph, pass, MD_LEN / 2);
    if(err)
        die("set key");

    err = gcry_cipher_setiv(args->ciph, iv, MD_LEN / 2);
    if(err)
        die("set iv");
}

static char *
get_md(const char *str)
{
    char *ret = malloc(MD_LEN);
    gcry_md_hash_buffer(GCRY_MD_SHA256, ret, str, strlen(str));
    return ret;
}

//input string format in s is "-arg=smth"
char *
get_opt(const char *s, int *len)
{
    DEBUG("");
    char *pos = strstr(s, "=");
    if(pos == NULL)
    {
        DEBUG("\"=\" not found");
        return NULL;
    }
    pos++;
    *len = strlen(pos);
    char *res = calloc(1, *len);
    strcpy(res, pos);

    return res;
}

static int
fcfs_opt_proc(void *data, const char *arg, int key, struct fuse_args *outargs)
{
    DEBUG("arg = %s", arg);

    int len = 0;
    char *a = NULL;
    switch(key)
    {
        case FCFS_OPT_VER:
            fuse_opt_add_arg(outargs, "--version");
            need_pass = 0;
            break;
        case FCFS_OPT_HELP:
            fprintf(stderr, "FCFS is a driver of file system.\n");
            fprintf(stderr, "   -b path_to_device   - path to block device.\n");
            fprintf(stderr, "   -p password         - password\n\n");
            fuse_opt_add_arg(outargs, "--help");
            need_pass = 0;
            break;
        case FCFS_OPT_PASSWD:
            len = strlen(arg);
            a = get_opt(arg, &len);
            {
                char *tmp = get_md(a);
                open_ciph(((fcfs_args_t*)data), tmp);
                free(tmp);
            }
            free(a);
            need_pass = 0;
            return 0;
            break;
        case FCFS_OPT_DEV:
            len = strlen(arg);
            a = get_opt(arg, &len);
            ((fcfs_args_t*)data)->p_dev = a;
            return 0;
            break;
    }
    return 1;
}

static void
get_passwd(fcfs_args_t *args)
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
    open_ciph(args, tmp);
    free(tmp);
    if(buf != NULL)
        free(buf);
}

int
main(int argc, char *argv[])
{
    DEBUG("");
    struct fuse_args fu_args = FUSE_ARGS_INIT(argc, argv);
    fcfs_args_t fc_args;
    memset(&fc_args, 0, sizeof(fcfs_args_t));
    fuse_opt_parse(&fu_args, &fc_args, fcfs_opt, fcfs_opt_proc);
    //get password if it not specify
    if(need_pass == 1)
    {
        get_passwd(&fc_args);
    }
#ifdef DBG
    for(size_t i = 0; i < fu_args.argc; ++i)
    {
        DEBUG("argv[%lu] = %s", i, fu_args.argv[i]);
    }
#endif
    //run daemon
    return fuse_main(fu_args.argc, fu_args.argv, &fcfs_ops, &fc_args);
}
