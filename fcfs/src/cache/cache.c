#include "../cache.h"

void
cache_init()
{
    cache_stat_init();
    cache_seq_init();
}

void
cache_destroy()
{
    cache_seq_destroy();
    cache_stat_destroy();
}
