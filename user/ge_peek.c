#include <stdint.h>

#include "main.h"
#include <psp2/kernel/threadmgr.h>
#include <psp2/io/fcntl.h>

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

static uint32_t arg_buf_addr = 0;
static const char *log_path = "ux0:data/adrenaline_ge_peek.txt";
static int log_fd = -1;

#define LOG(...) { \
	if(log_fd < 0){ \
		log_fd = sceIoOpen(log_path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0666); \
	} \
	if (log_fd >= 0){ \
		char _buf[1024]; \
		uint32_t _len = sprintf(_buf, __VA_ARGS__); \
		if(sceIoWrite(log_fd, _buf, _len) != _len){ \
			sceIoClose(log_fd); \
			log_fd = -1; \
		} \
	} \
}

#define LOG_FLUSH(...) { \
	LOG(__VA_ARGS__); \
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
static int ge_peek_loop(SceSize args, void *argp){
	int next_call_is_addr = 0;
	while(1){
		SceKermitRequest *request;
		ScePspemuKermitWaitAndGetRequest(1, &request);

		uint32_t res = 0xFEDCBA98;
		if(request->cmd == CMD_SET_ARG_BUF){
			LOG("incoming cmd buf address 0x%08x\n", (uint32_t)request->args[0]);
			arg_buf_addr = (uint32_t)&request->args[0];
		}else{
			switch(request->cmd){
				case CMD_EDRAM_GET_ADDR:{
					LOG_CMD("CMD_EDRAM_GET_ADDR\n");
					break;
				}
				case CMD_LIST_ENQUEUE:{
					uint32_t listAddress;
					uint32_t stallAddress;
					int32_t callbackId;
					uint32_t optParamAddr;
					memcpy(&listAddress, &request->args[0], 4);
					memcpy(&stallAddress, &request->args[1], 4);
					memcpy(&callbackId, &request->args[2], 4);
					memcpy(&optParamAddr, &request->args[3], 4);
					LOG_CMD("CMD_LIST_ENQUEUE listAddress 0x%08x stallAddress 0x%08x callbackId %d optParamAddr 0x%08x\n", listAddress, stallAddress, callbackId, optParamAddr);
					break;
				}
				case CMD_LIST_ENQUEUE_HEAD:{
					uint32_t listAddress;
					uint32_t stallAddress;
					int32_t callbackId;
					uint32_t optParamAddr;
					memcpy(&listAddress, &request->args[0], 4);
					memcpy(&stallAddress, &request->args[1], 4);
					memcpy(&callbackId, &request->args[2], 4);
					memcpy(&optParamAddr, &request->args[3], 4);
					LOG_CMD("CMD_LIST_ENQUEUE_HEAD listAddress 0x%08x stallAddress 0x%08x callbackId %d optParamAddr 0x%08x\n", listAddress, stallAddress, callbackId, optParamAddr);
					break;
				}
				case CMD_LIST_UPDATE_STALL_ADDR:{
					uint32_t displayListID;
					uint32_t stallAddress;
					memcpy(&displayListID, &request->args[0], 4);
					memcpy(&stallAddress, &request->args[1], 4);
					LOG_CMD("CMD_LIST_UPDATE_STALL_ADDR displayListID 0x%08x stallAddress 0x%08x\n", displayListID, stallAddress);
					break;
				}
				case CMD_LIST_SYNC:{
					uint32_t displayListID;
					uint32_t mode;
					memcpy(&displayListID, &request->args[0], 4);
					memcpy(&mode, &request->args[1], 4);
					LOG_CMD("CMD_LIST_SYNC displayListID 0x%08x mode 0x%08x\n", displayListID, mode);
					break;
				}
				case CMD_DRAW_SYNC:{
					uint32_t mode;
					memcpy(&mode, &request->args[0], 4);
					LOG_CMD("CMD_DRAW_SYNC mode 0x%08x\n", mode);
					break;
				}
				case CMD_BREAK:{
					uint32_t mode;
					uint32_t unknownPtr;
					memcpy(&mode, &request->args[0], 4);
					memcpy(&unknownPtr, &request->args[1], 4);
					LOG_CMD("CMD_BREAK mode 0x%08x unknownPtr 0x%08x\n", mode, unknownPtr);
					break;
				}
				case CMD_CONTINUE:{
					LOG_CMD("CMD_CONTINUE\n");
					break;
				}
				case CMD_SET_CALLBACK:{
					uint32_t structAddr;
					memcpy(&structAddr, &request->args[0], 4);
					LOG_CMD("CMD_SET_CALLBACK structAddr 0x%08x\n", structAddr);
					break;
				}
				case CMD_UNSET_CALLBACK:{
					uint32_t cbID;
					memcpy(&cbID, &request->args[0], 4);
					LOG_CMD("CMD_UNSET_CALLBACK cbID 0x%08x\n", cbID);
					break;
				}
				case CMD_EDRAM_GET_SIZE:{
					LOG_CMD("CMD_EDRAM_GET_SIZE\n");
					break;
				}
				case CMD_EDRAM_SET_ADDR_TRANSLATION:{
					uint32_t new_size;
					memcpy(&new_size, &request->args[0], 4);
					LOG_CMD("CMD_EDRAM_SET_ADDR_TRANSLATION new_size 0x%08x\n", new_size);
					break;
				}
				case CMD_GET_CMD:{
					int32_t cmd;
					memcpy(&cmd, &request->args[0], 4);
					LOG_CMD("CMD_GET_CMD cmd %d\n", cmd);
					break;
				}
				case CMD_GET_MTX:{
					int32_t type;
					uint32_t matrixPtr;
					memcpy(&type, &request->args[0], 4);
					memcpy(&matrixPtr, &request->args[1], 4);
					LOG_CMD("CMD_GET_MTX type %d matrixPtr 0x%08x\n", type, matrixPtr);
					break;
				}
				case CMD_SAVE_CONTEXT:{
					uint32_t ctxAddr;
					memcpy(&ctxAddr, &request->args[0], 4);
					LOG_CMD("CMD_SAVE_CONTEXT ctxAddr 0x%08x\n", ctxAddr);
					break;
				}
				case CMD_RESTORE_CONTEXT:{
					uint32_t ctxAddr;
					memcpy(&ctxAddr, &request->args[0], 4);
					LOG_CMD("CMD_RESTORE_CONTEXT ctxAddr 0x%08x\n", ctxAddr);
					break;
				}
				case CMD_LIST_DEQUEUE:{
					uint32_t listID;
					memcpy(&listID, &request->args[0], 4);
					LOG_CMD("CMD_LIST_DEQUEUE listID 0x%08x\n", listID);
					break;
				}
				case CMD_GET_STACK:{
					int32_t index;
					uint32_t stackPtr;
					memcpy(&index, &request->args[0], 4);
					memcpy(&stackPtr, &request->args[1], 4);
					LOG_CMD("CMD_GET_STACK index %d stackPtr 0x%08x\n", index, stackPtr);
					break;
				}
				default:{ LOG_CMD("cmd 0x%08x,", request->cmd); for(int i = 0;i < 14; i++){
						LOG_CMD(" 0x%08x", (uint32_t)&request->args[i]);
					}
					LOG_CMD("\n");
				}
			}

		}

		/*
		if(request->cmd == 0xEEEEEEEE){
			LOG_CMD("received command 0x%08x, setting address next call\n", request->cmd);
			next_call_is_addr = 1;
		}else if(next_call_is_addr){
			arg_buf_addr = request->cmd;
			LOG_CMD("initializing arg_buf_addr to 0x%08x\n", request->cmd);
		}else{
			uint32_t nid = request->cmd;
			LOG_CMD("nid: 0x%08x", nid);

			LOG_CMD(" arg_buf_addr 0x%08x, arg_buf: ", (uint32_t)arg_buf_addr);
			uint32_t *arg_addr = ScePspemuConvertAddress(arg_buf_addr, KERMIT_INPUT_MODE, 10 * 4);
			for(int i = 0;i < 10; i++){
				LOG_CMD(" 0x%08x", arg_addr[i]);
			}
			LOG_CMD("\n");
		}
		*/

		ScePspemuKermitSendResponse(14, request, (uint64_t)res);
	}
}

int init_ge_peek(){
	sceIoRemove(log_path);

	LOG("initializing ge_peek\n");

	SceUID thid = sceKernelCreateThread("GePeek", ge_peek_loop, 0x60, 0x1000, 0, SCE_KERNEL_CPU_MASK_USER_1 | SCE_KERNEL_CPU_MASK_SYSTEM, NULL);
	if(thid < 0){
		LOG("failed initializing ge_peek_loop\n");
		return thid;
	}
	sceKernelStartThread(thid, 0, NULL);

	LOG("finished initializing ge_peek\n");
	return 0;
}
