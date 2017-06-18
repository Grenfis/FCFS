#include "../cache.h"

void
cache_init()
{
    cache_fid_init();
    cache_seq_init();
}

void
cache_destroy()
{
    cache_seq_destroy();
    cache_fid_destroy();
}
