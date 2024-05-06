#include <pspiofilemgr.h>
#include <stdio.h>

#ifdef DEBUG

int printk_fd = -1;
static char path_buf[64];

static void open_printk_fd(){
	printk_fd = sceIoOpen(path_buf, PSP_O_WRONLY | PSP_O_CREAT, 0666);
}

int printk_init(const char *path){
	strcpy(path_buf, path);
	sceIoRemove(path);
	open_printk_fd();
	return printk_fd;
}

#endif


