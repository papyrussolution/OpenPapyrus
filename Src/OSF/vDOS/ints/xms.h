#ifndef __XMS_H__
#define __XMS_H__

Bit8u XMS_QueryFreeMemory(Bit16u& freeXMS);
Bit8u XMS_AllocateMemory(Bit16u size, Bit16u& handle);
Bit16u EMS_FreeKBs();

extern RealPt xms_callback;

#endif
