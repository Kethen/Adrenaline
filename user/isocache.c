// this is the inferno iso cache implementation

#include <psp2/kernel/threadmgr.h>
#include <psp2/io/fcntl.h>
#include <psp2/kernel/sysmem.h>

#include <stdio.h>
#include <string.h>

#include "isocache.h"
#include "main.h"
#include "utils.h"

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define ALIGN(x, a)	(((x) + ((a) - 1)) & ~((a) - 1))

static char g_iso_path[1024];
static int g_iso_fd = -1;
static SceUID worker_thid = -1;
static SceUID worker_sema = -1;
static SceUID worker_done_sema = -1;
static int worker_ret = 0;
static struct IoReadArg *worker_task;

#define LOG(...) { \
	debugPrintf(__VA_ARGS__); \
}

#define g_caches_cap 4096
#define g_caches_num (141 * 1024 * 1024 / g_caches_cap)

struct ISOCache{
	char *buf;
	int bufsize;
	int pos;
	int age;
};

static uint32_t read_call = 0;
static uint32_t read_missed = 0;
static uint32_t read_hit = 0;

static struct ISOCache *g_caches = NULL;
static SceUID g_caches_memid;

int iso_cache_fd(){
	return g_iso_fd;
}

static inline int is_within_range(int pos, int start, int len)
{
	if(start != -1 && pos >= start && pos < start + len) {
		return 1;
	}

	return 0;
}

static int binary_search(const struct ISOCache *caches, uint32_t n, int pos)
{
	int low, high, mid;

	low = 0;
	high = n - 1;

	while (low <= high) {
		mid = (low + high) / 2;

		if(is_within_range(pos, caches[mid].pos, caches[mid].bufsize)) {
			return mid;
		} else if (pos < caches[mid].pos) {
			high = mid - 1;
		} else {
			low = mid + 1;
		}
	}

	return -1;
}

static struct ISOCache *get_matched_buffer(int pos)
{
	int cache_pos;

	cache_pos = binary_search(g_caches, g_caches_num, pos);

	if(cache_pos == -1) {
		return NULL;
	}

	return &g_caches[cache_pos];
}

static struct ISOCache *get_retirng_cache(void)
{
	int i, retiring;

	retiring = 0;

	// invalid cache first
	for(i=0; i<g_caches_num; ++i) {
		if(g_caches[i].pos == -1) {
			retiring = i;
			goto exit;
		}
	}

	for(i=0; i<g_caches_num; ++i) {
		if(g_caches[i].age > g_caches[retiring].age) {
			retiring = i;
		}
	}

exit:
	return &g_caches[retiring];
}

static int get_hit_caches(int pos, int len, char *data, struct ISOCache **last_cache)
{
	uint32_t cur;
	int read_len;
	struct ISOCache *cache = NULL;

	*last_cache = NULL;

	for(cur = pos; cur < pos + len;) {
		*last_cache = cache;
		cache = get_matched_buffer(cur);

		if(cache == NULL) {
			break;
		}

		read_len = MIN(len - (cur - pos), cache->pos + cache->bufsize - cur);

		if(data != NULL) {
			memcpy(data + cur - pos, cache->buf + cur - cache->pos, read_len);
		}

		cur += read_len;
		cache->age = -1;
	}

	if(cache == NULL)
		return -1;

	read_hit += len;

	return cur - pos;
}

static void disable_cache(struct ISOCache *cache)
{
	cache->pos = -1;
	cache->age = -1;
	cache->bufsize = 0;
}

static void reorder_iso_cache(int idx)
{
	struct ISOCache tmp;
	int i;

	if(idx < 0 && idx >= g_caches_num) {
		LOG("%s: wrong idx\n", __func__);
		return;
	}

	memmove(&tmp, &g_caches[idx], sizeof(g_caches[idx]));
	memmove(&g_caches[idx], &g_caches[idx+1], sizeof(g_caches[idx]) * (g_caches_num - idx - 1));

	for(i=0; i<g_caches_num-1; ++i) {
		if(g_caches[i].pos >= tmp.pos) {
			break;
		}
	}

	memmove(&g_caches[i+1], &g_caches[i], sizeof(g_caches[idx]) * (g_caches_num - i - 1));
	memmove(&g_caches[i], &tmp, sizeof(tmp));
}

static void iso_open(){
	if(g_iso_fd >= 0){
		sceIoClose(g_iso_fd);
		g_iso_fd = -1;
	}
	g_iso_fd = sceIoOpen(g_iso_path, 0x000F0001, 0666);
	if(g_iso_fd < 0){
		LOG("failed opening iso %s\n", g_iso_path);
	}
}

static int _iso_cache_read(struct IoReadArg *arg);
static int iso_read(struct IoReadArg *arg);
static int iso_cache_worker(SceSize args, void *argp){
	while(1){
		sceKernelWaitSema(worker_sema, 1, 0);
		//LOG("%s: offset 0x%08x, address 0x%08x, size 0x%08x\n", __func__, worker_task->offset, worker_task->address, worker_task->size);
		//worker_ret = iso_read(worker_task);
		worker_ret = _iso_cache_read(worker_task);
		sceKernelSignalSema(worker_done_sema, 1);
	}
}

int iso_cache_read(struct IoReadArg *arg){
	//LOG("%s: offset 0x%08x, address 0x%08x, size 0x%08x\n", __func__, arg->offset, arg->address, arg->size);
	worker_task = arg;
	sceKernelSignalSema(worker_sema, 1);
	sceKernelWaitSema(worker_done_sema, 1, 0);
	return worker_ret;
}

void iso_cache_init(const char *path){
	if(g_caches == NULL){
		SceUID memid = sceKernelAllocMemBlock("ScePspemuISOCache", SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_RW, ALIGN((g_caches_num * sizeof(struct ISOCache) + g_caches_num * g_caches_cap), (1024 * 4)), NULL);
		if(memid < 0){
			LOG("failed allocating memory for cache, 0x%08x, exiting now\n", memid);
			ScePspemuErrorExit(0);
		}
		int res = sceKernelGetMemBlockBase(memid, &g_caches);
		if(res != 0){
			LOG("failed grabbing base memory for cache, 0x%08x, exiting now\n", res);
			ScePspemuErrorExit(0);
		}

		char *pbuf = (char *)g_caches + g_caches_num * sizeof(struct ISOCache);
		for(int i = 0;i < g_caches_num; i++){
			struct ISOCache *cache = &g_caches[i];
			cache->buf = pbuf + i * g_caches_cap;
		}
		g_caches_memid = memid;
	}
	for(int i = 0;i < g_caches_num; i++){
		struct ISOCache *cache = &g_caches[i];
		disable_cache(cache);
	}

	if(worker_sema < 0){
	    worker_sema = sceKernelCreateSema("ScePspemuISOCacheWorkerSema", 0, 0, 1, NULL);
	    if(worker_sema < 0){
	        LOG("failed creating iso cache worker semaphore, 0x%08x, exiting now\n", worker_sema);
	        ScePspemuErrorExit(0);
	    }
	}

	if(worker_done_sema < 0){
	    worker_done_sema = sceKernelCreateSema("ScePspemuISOCacheWorkerDoneSema", 0, 0, 1, NULL);
	    if(worker_done_sema < 0){
	        LOG("failed creating iso cache worker done semaphore, 0x%08x, exiting now\n", worker_done_sema);
	        ScePspemuErrorExit(0);
	    }
	}

	if(worker_thid < 0){
	    worker_thid = sceKernelCreateThread("ScePspemuISOCacheWorker", iso_cache_worker, 0x60, 0x1000, 0, SCE_KERNEL_CPU_MASK_USER_1, NULL);
	    if(worker_thid < 0){
	        LOG("failed creating iso cache worker thread, 0x%08x, exiting now\n", worker_thid);
	        ScePspemuErrorExit(0);
	    }
	    sceKernelStartThread(worker_thid, 0, NULL);
	}

	if(path[0] != '\0'){
		strcpy(g_iso_path, path);
	}
	iso_open();
	LOG("iso cache initialized for %s\n", g_iso_path);
}

void iso_cache_stop(){
	if(g_iso_fd >= 0){
		sceIoClose(g_iso_fd);
	}
	if(g_caches != NULL){
		sceKernelFreeMemBlock(g_caches_memid);
		g_caches = NULL;
	}
}

static int iso_read(struct IoReadArg *arg)
{

	int ret, i;
	SceOff ofs;

	if(g_iso_fd < 0){
		return 0x80010013;
	}

	i = 0;

	do {
		i++;
		ofs = sceIoLseek(g_iso_fd, arg->offset, SCE_SEEK_SET);

		if(ofs < 0) {
			LOG("%s: offset 0x%08x, address 0x%08x, size 0x%08x\n", __func__, arg->offset, arg->address, arg->size);
			LOG("%s: lseek retry %d error 0x%08X\n", __func__, i, (int)ofs);
			iso_open();
			continue;
		}

		int bytes_read = 0;
		while(bytes_read != arg->size){
			ret = sceIoRead(g_iso_fd, arg->address + bytes_read, arg->size - bytes_read);
			if(ret < 0){
				LOG("%s: offset 0x%08x, address 0x%08x, size 0x%08x\n", __func__, arg->offset, arg->address, arg->size);
				LOG("%s: read retry %d error 0x%08X\n", __func__, i, ret);
				iso_open();
				bytes_read = -1;
				break;
			}
			if(ret == 0){
				break;
			}
			bytes_read = bytes_read + ret;
		}
		if(bytes_read == -1){
			continue;
		}

		ret = bytes_read;
		i = 0;
		break;
	} while(i < 16);

	if(i == 16) {
		ret = 0x80010013;
	}

	return ret;
}

static int add_cache(struct IoReadArg *arg)
{
	int read_len, len, ret;
	struct IoReadArg cache_arg;
	struct ISOCache *cache, *last_cache;
	uint32_t pos, cur;
	char *data;

	len = arg->size;
	pos = arg->offset;
	data = (char*)arg->address;

	for(cur = pos; cur < pos + len;) {
		if(data == NULL) {
			ret = get_hit_caches(cur, len - (cur - pos), NULL, &last_cache);
		} else {
			ret = get_hit_caches(cur, len - (cur - pos), data + cur - pos, &last_cache);
		}

		if(ret >= 0) {
			cur += ret;
			continue;
		}

		if(last_cache != NULL) {
			if(pos + len <= last_cache->pos + last_cache->bufsize) {
				LOG("%s: error pos, panic!\n", __func__);
				ScePspemuErrorExit(0);
			}

			cur = last_cache->pos + last_cache->bufsize;
		}

		cache = get_retirng_cache();
		disable_cache(cache);
		cache_arg.offset = cur & (~(64-1));
		cache_arg.address = (char*)cache->buf;
		cache_arg.size = g_caches_cap;

		ret = iso_read(&cache_arg);

		if(ret > 0) {
			cache->pos = cache_arg.offset;
			cache->age = -1;
			cache->bufsize = ret;

			read_len = MIN(len - (cur - pos), cache->pos + cache->bufsize - cur);

			if(data != NULL) {
				memcpy(data + cur - pos, cache->buf + cur - cache->pos, read_len);
			}

			cur += read_len;
			reorder_iso_cache(cache - g_caches);
		} else if (ret == 0) {
			// EOF reached, time to exit
			reorder_iso_cache(cache - g_caches);
			break;
		} else {
			reorder_iso_cache(cache - g_caches);
			LOG("%s: read -> 0x%08X\n", __func__, ret);
			return ret;
		}
	}

	return cur - pos;
}

static int _iso_cache_read(struct IoReadArg *arg)
{
	int ret, len;
	uint32_t pos;
	char *data;
	struct ISOCache *last_cache;

	data = (char*)arg->address;
	pos = arg->offset;
	len = arg->size;
	ret = get_hit_caches(pos, len, data, &last_cache);

	if(ret < 0) {
		// abandon the caching, because the bufsize is too small
		// if we cache it then random access performance will be hurt
		if(arg->size < (uint32_t)g_caches_cap) {
			return iso_read(arg);
		}

		ret = add_cache(arg);
		read_missed += len;
	} else {
		// we have to sleep a bit to prevent mystery freeze in KHBBS (from worldmap to location)
		// it's caused by caching too fast
		// tested NFS Carbon: Own the city, seems delaying 100 wasn't enough...

		// we are really really slow on vita side for some reason
		//sceKernelDelayThread(MAX(512, len / 16));
	}

	read_call += len;

	return ret;
}
