.set noreorder

#include "pspimport.s"

IMPORT_START "sceKermit_driver",0x00010011
IMPORT_FUNC  "sceKermit_driver",0x36666181,sceKermitSendRequest661
IMPORT_FUNC  "sceKermit_driver",0x5280B410,sceKermitRegisterVirtualIntrHandler661

IMPORT_START "sceKermitMemory_driver",0x00090011
IMPORT_FUNC  "sceKermitMemory_driver",0xAAF047AC,sceKermitMemorySetArgument661
