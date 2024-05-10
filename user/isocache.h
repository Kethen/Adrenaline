#ifndef H_ISOCACHE
#define H_ISOCACHE
struct IoReadArg{
    uint32_t offset;
    char *address;
    uint32_t size;
};

int iso_cache_read(struct IoReadArg *arg);
void iso_cache_init(const char *path);
void iso_cache_stop();
int iso_cache_fd();
#endif
