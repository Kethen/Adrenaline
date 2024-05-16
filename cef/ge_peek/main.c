// referencing adrenaline.c in systemctrl

#include <pspsdk.h>
#include <pspkernel.h>
#include <pspiofilemgr.h>

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>

#include <systemctrl.h>

#define ALIGN(x, align) (((x) + ((align) - 1)) & ~((align) - 1))
typedef struct {
	uint32_t cmd; //0x0
	SceUID sema_id; //0x4
	uint64_t *response; //0x8
	uint32_t padding; //0xC
	uint64_t args[14]; // 0x10
} SceKermitRequest; //0x80

int sceKermitSendRequest661(SceKermitRequest *request, uint32_t mode, uint32_t cmd, uint32_t args, uint32_t is_callback, uint64_t *resp);
int sceKermitRegisterVirtualIntrHandler661(int num, int (* handler)());
int sceKermitMemorySetArgument661(SceKermitRequest *request, int argc, const void *data, int size, int mode);

PSP_MODULE_INFO("ge_peek", 0x1007, 1, 0);

static const char *log_path = "ms0:/ge_peek.txt";
static int log_fd = -1;

#define LOG(...) { \
	if(log_fd < 0){ \
		log_fd = sceIoOpen(log_path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_APPEND, 0666); \
	} \
	if (log_fd >= 0){ \
		char _buf[1024]; \
		uint32_t _len = sprintf(_buf, __VA_ARGS__); \
		if(sceIoWrite(log_fd, _buf, _len) != _len){ \
			sceIoClose(log_fd); \
			log_fd = -1; \
		} \
	} \
	if(log_fd >=0){ \
		sceIoClose(log_fd); \
		log_fd = -1; \
	} \
}

#if 0
#define LOG_CMD(...) LOG(__VA_ARGS__)
#else
#define LOG_CMD(...)
#endif
// hook everything ppsspp ge implements
uint32_t (* _sceGeEdramGetAddr)();
uint32_t (* _sceGeListEnQueue)(uint32_t listAddress, uint32_t stallAddress, int32_t callbackId, uint32_t optParamAddr);
uint32_t (* _sceGeListEnQueueHead)(uint32_t listAddress, uint32_t stallAddress, int callbackId, uint32_t optParamAddr);
int32_t (* _sceGeListUpdateStallAddr)(uint32_t displayListID, uint32_t stallAddress);
int32_t (* _sceGeListSync)(uint32_t displayListID, uint32_t mode);
uint32_t (* _sceGeDrawSync)(uint32_t mode);
int32_t (* _sceGeBreak)(uint32_t mode, uint32_t unknownPtr);
int32_t (*_sceGeContinue)();
uint32_t (* _sceGeSetCallback)(uint32_t structAddr);
int32_t (* _sceGeUnsetCallback)(uint32_t cbID);
uint32_t (* _sceGeEdramGetSize)();
uint32_t (* _sceGeEdramSetAddrTranslation)(uint32_t new_size);
uint32_t (* _sceGeGetCmd)(int32_t cmd);
int32_t (* _sceGeGetMtx)(int32_t type, uint32_t matrixPtr);
uint32_t (* _sceGeSaveContext)(uint32_t ctxAddr);
uint32_t (* _sceGeRestoreContext)(uint32_t ctxAddr);
int32_t (* _sceGeListDeQueue)(uint32_t listID);
int32_t (* _sceGeGetStack)(int32_t index, uint32_t stackPtr);

#define CMD_SET_ARG_BUF 0x1
#define CMD_EDRAM_GET_ADDR 0x2
#define CMD_LIST_ENQUEUE 0x3
#define CMD_LIST_ENQUEUE_HEAD 0x4
#define CMD_LIST_UPDATE_STALL_ADDR 0x5
#define CMD_LIST_SYNC 0x6
#define CMD_DRAW_SYNC 0x7
#define CMD_BREAK 0x8
#define CMD_CONTINUE 0x9
#define CMD_SET_CALLBACK 0xA
#define CMD_UNSET_CALLBACK 0xB
#define CMD_EDRAM_GET_SIZE 0xC
#define CMD_EDRAM_SET_ADDR_TRANSLATION 0xD
#define CMD_GET_CMD 0xE
#define CMD_GET_MTX 0xF
#define CMD_SAVE_CONTEXT 0x10
#define CMD_RESTORE_CONTEXT 0x11
#define CMD_LIST_DEQUEUE 0x12
#define CMD_GET_STACK 0x13


// #9 for return values
// #8 for interrupt type
static uint32_t arg_buf[10];

static void call_function(uint32_t cmd, uint32_t *ret, int arg_count, ...){
	char buf[sizeof(SceKermitRequest) + 0x40];
	SceKermitRequest *request_aligned = (SceKermitRequest *)ALIGN((u32)buf, 0x40);
	SceKermitRequest *request_uncached = (SceKermitRequest *)((u32)request_aligned | 0x20000000);
	sceKernelDcacheInvalidateRange(request_aligned, sizeof(SceKermitRequest));
	sceKernelDcacheInvalidateRange(arg_buf, sizeof(arg_buf));

	va_list args;
	va_start(args, arg_count);
	for(int i = 0;i < arg_count; i++){
		uint32_t arg = va_arg(args, uint32_t);
		request_uncached->args[i] = arg;
	}
	va_end(args);

	LOG_CMD("cmd: 0x%08x, ", cmd);
	for(int i = 0;i < 10; i++){
		LOG_CMD(" 0x%08x", (uint32_t)request_uncached->args[i]);
	}
	LOG_CMD("\n");

	int k1 = pspSdkSetK1(0);

	uint64_t resp;
	sceKermitSendRequest661(request_uncached, 1, cmd, 0, 0, &resp);
	pspSdkSetK1(k1);

    LOG_CMD("kermit resp 0x%08x\n", (uint32_t)resp);

    *ret = resp;
}

uint32_t sceGeEdramGetAddrPatched(){
	uint32_t ret;

	call_function(CMD_EDRAM_GET_ADDR, &ret, 0);

	ret = _sceGeEdramGetAddr();
	return ret;
}

uint32_t sceGeListEnQueuePatched(uint32_t listAddress, uint32_t stallAddress, int32_t callbackId, uint32_t optParamAddr){
	uint32_t ret;

	call_function(CMD_LIST_ENQUEUE, &ret, 4, listAddress, stallAddress, callbackId, optParamAddr);

	ret = _sceGeListEnQueue(listAddress, stallAddress, callbackId, optParamAddr);
	return ret;
}

uint32_t sceGeListEnQueueHeadPatched(uint32_t listAddress, uint32_t stallAddress, int callbackId, uint32_t optParamAddr){
	uint32_t ret;

	call_function(CMD_LIST_ENQUEUE_HEAD, &ret, 4, listAddress, stallAddress, callbackId, optParamAddr);

	ret = _sceGeListEnQueueHead(listAddress, stallAddress, callbackId, optParamAddr);
	return ret;
}

int32_t sceGeListUpdateStallAddrPatched(uint32_t displayListID, uint32_t stallAddress){
	int32_t ret;

	call_function(CMD_LIST_UPDATE_STALL_ADDR, &ret, 2, displayListID, stallAddress);

	ret = _sceGeListUpdateStallAddr(displayListID, stallAddress);
	return ret;
}

int32_t sceGeListSyncPatched(uint32_t displayListID, uint32_t mode){
	int32_t ret;

	call_function(CMD_LIST_SYNC, &ret, 2, displayListID, mode);

	ret = _sceGeListSync(displayListID, mode);
	return ret;
}

uint32_t sceGeDrawSyncPatched(uint32_t mode){
	uint32_t ret;

	call_function(CMD_DRAW_SYNC, &ret, 1, mode);

	ret = _sceGeDrawSync(mode);
	return ret;
}

int32_t sceGeBreakPatched(uint32_t mode, uint32_t unknownPtr){
	int32_t ret;

	call_function(CMD_BREAK, &ret, 2, mode, unknownPtr);

	ret = _sceGeBreak(mode, unknownPtr);
	return ret;
}

int32_t sceGeContinuePatched(){
	int32_t ret;

	call_function(CMD_CONTINUE, &ret, 0);

	ret = _sceGeContinue();
	return ret;
}

uint32_t sceGeSetCallbackPatched(uint32_t structAddr){
	uint32_t ret;

	call_function(CMD_SET_CALLBACK, &ret, 1, structAddr);

	// might have to handle emulating this on this side

	ret = _sceGeSetCallback(structAddr);
	return ret;
}

int32_t sceGeUnsetCallbackPatched(uint32_t cbID){
	int32_t ret;

	call_function(CMD_UNSET_CALLBACK, &ret, 1, cbID);

	// might have to handle emulating this on this side

	ret = _sceGeUnsetCallback(cbID);
	return ret;
}

uint32_t sceGeEdramGetSizePatched(){
	uint32_t ret;

	call_function(CMD_EDRAM_GET_SIZE, &ret, 0);

	ret = _sceGeEdramGetSize();
	return ret;
}

uint32_t sceGeEdramSetAddrTranslationPatched(uint32_t new_size){
	uint32_t ret;

	call_function(CMD_EDRAM_SET_ADDR_TRANSLATION, &ret, 1, new_size);

	ret = _sceGeEdramSetAddrTranslation(new_size);
	return ret;
}

uint32_t sceGeGetCmdPatched(int32_t cmd){
	uint32_t ret;

	call_function(CMD_GET_CMD, &ret, 1, cmd);

	ret = _sceGeGetCmd(cmd);
	return ret;
}

int32_t sceGeGetMtxPatched(int32_t type, uint32_t matrixPtr){
	int32_t ret;

	call_function(CMD_GET_MTX, &ret, 2, type, matrixPtr);

	ret = _sceGeGetMtx(type, matrixPtr);
	return ret;
}

uint32_t sceGeSaveContextPatched(uint32_t ctxAddr){
	uint32_t ret;

	call_function(CMD_SAVE_CONTEXT, &ret, 1, ctxAddr);

	ret = _sceGeSaveContext(ctxAddr);
	return ret;
}

uint32_t sceGeRestoreContextPatched(uint32_t ctxAddr){
	uint32_t ret;

	call_function(CMD_RESTORE_CONTEXT, &ret, 1, ctxAddr);

	ret = _sceGeRestoreContext(ctxAddr);
	return ret;
}

int32_t sceGeListDeQueuePatched(uint32_t listID){
	int32_t ret;

	call_function(CMD_LIST_DEQUEUE, &ret, 1, listID);

	ret = _sceGeListDeQueue(listID);
	return ret;
}

int32_t sceGeGetStackPatched(int32_t index, uint32_t stackPtr){
	int32_t ret;

	call_function(CMD_GET_STACK, &ret, 2, index, stackPtr);

	ret = _sceGeGetStack(index, stackPtr);
	return ret;
}

int interrupt_handler(){
	// interrupts should be disabled already during interrupt handling..?
	sceKernelDcacheInvalidateRange(arg_buf, sizeof(arg_buf));
	uint32_t interrupt_type = arg_buf[8];
	return 0;
}

int module_start(SceSize args, void *argp){
	sceIoRemove(log_path);
	LOG("starting ge_peek\n");

	uint32_t ret;
	LOG("registering argbuf location 0x%08x\n", (uint32_t)arg_buf);
	call_function(CMD_SET_ARG_BUF, &ret, 1, arg_buf);

	LOG("registering kermit interrupt\n");
	sceKermitRegisterVirtualIntrHandler661(0xEEEEEEEE, interrupt_handler);

	LOG("hooking functions\n");
	_sceGeEdramGetAddr = (void *)FindProc("sceGE_Manager", "sceGe_driver", 0xE47E40E4);
	_sceGeListEnQueue = (void *)FindProc("sceGE_Manager", "sceGe_driver", 0XAB49E76A);
	_sceGeListEnQueueHead = (void *)FindProc("sceGE_Manager", "sceGe_driver", 0X1C0D95A6);
	_sceGeListUpdateStallAddr = (void *)FindProc("sceGE_Manager", "sceGe_driver", 0XE0D68148);
	_sceGeListSync = (void *)FindProc("sceGE_Manager", "sceGe_driver", 0X03444EB4);
	_sceGeDrawSync = (void *)FindProc("sceGE_Manager", "sceGe_driver", 0XB287BD61);
	_sceGeBreak = (void *)FindProc("sceGE_Manager", "sceGe_driver", 0XB448EC0D);
	_sceGeContinue = (void *)FindProc("sceGE_Manager", "sceGe_driver", 0X4C06E472);
	_sceGeSetCallback = (void *)FindProc("sceGE_Manager", "sceGe_driver", 0XA4FC06A4);
	_sceGeUnsetCallback = (void *)FindProc("sceGE_Manager", "sceGe_driver", 0X05DB22CE);
	_sceGeEdramGetSize = (void *)FindProc("sceGE_Manager", "sceGe_driver", 0X1F6752AD);
	_sceGeEdramSetAddrTranslation = (void *)FindProc("sceGE_Manager", "sceGe_driver", 0XB77905EA);
	_sceGeGetCmd = (void *)FindProc("sceGE_Manager", "sceGe_driver", 0XDC93CFEF);
	_sceGeGetMtx = (void *)FindProc("sceGE_Manager", "sceGe_driver", 0X57C8945B);
	_sceGeSaveContext = (void *)FindProc("sceGE_Manager", "sceGe_driver", 0X438A385A);
	_sceGeRestoreContext = (void *)FindProc("sceGE_Manager", "sceGe_driver", 0X0BF608FB);
	_sceGeListDeQueue = (void *)FindProc("sceGE_Manager", "sceGe_driver", 0X5FB86AB0);
	_sceGeGetStack = (void *)FindProc("sceGE_Manager", "sceGe_driver", 0XE66CB92E);

	sctrlHENPatchSyscall((uint32_t)_sceGeEdramGetAddr, sceGeEdramGetAddrPatched);
	sctrlHENPatchSyscall((uint32_t)_sceGeListEnQueue, sceGeListEnQueuePatched);
	sctrlHENPatchSyscall((uint32_t)_sceGeListEnQueueHead, sceGeListEnQueueHeadPatched);
	sctrlHENPatchSyscall((uint32_t)_sceGeListUpdateStallAddr, sceGeListUpdateStallAddrPatched);
	sctrlHENPatchSyscall((uint32_t)_sceGeListSync, sceGeListSyncPatched);
	sctrlHENPatchSyscall((uint32_t)_sceGeDrawSync, sceGeDrawSyncPatched);
	sctrlHENPatchSyscall((uint32_t)_sceGeBreak, sceGeBreakPatched);
	sctrlHENPatchSyscall((uint32_t)_sceGeContinue, sceGeContinuePatched);
	sctrlHENPatchSyscall((uint32_t)_sceGeSetCallback, sceGeSetCallbackPatched);
	sctrlHENPatchSyscall((uint32_t)_sceGeUnsetCallback, sceGeUnsetCallbackPatched);
	sctrlHENPatchSyscall((uint32_t)_sceGeEdramGetSize, sceGeEdramGetSizePatched);
	sctrlHENPatchSyscall((uint32_t)_sceGeEdramSetAddrTranslation, sceGeEdramSetAddrTranslationPatched);
	sctrlHENPatchSyscall((uint32_t)_sceGeGetCmd, sceGeGetCmdPatched);
	sctrlHENPatchSyscall((uint32_t)_sceGeGetMtx, sceGeGetMtxPatched);
	sctrlHENPatchSyscall((uint32_t)_sceGeSaveContext, sceGeSaveContextPatched);
	sctrlHENPatchSyscall((uint32_t)_sceGeRestoreContext, sceGeRestoreContextPatched);
	sctrlHENPatchSyscall((uint32_t)_sceGeListDeQueue, sceGeListDeQueuePatched);
	sctrlHENPatchSyscall((uint32_t)_sceGeGetStack, sceGeGetStackPatched);

	LOG("finished starting ge_peek\n");
}
