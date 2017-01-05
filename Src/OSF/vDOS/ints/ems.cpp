#include <string.h>
#include <stdlib.h>
#include "vdos.h"
#include "callback.h"
#include "regs.h"

#include "mem.h"
#include "xms.h"
#include "paging.h"
#include "bios.h"
#include "dos_inc.h"
#include "support.h"

#define EMS_PAGEFRAME			0xE000
#define	EMS_HANDLES				64
#define EMS_PAGE_SIZE			(16*1024U)
#define EMS_MAX_PHYS			4													// 4 16kb pages in pageframe

#define EMS_VERSION				0x32
#define	NULL_PAGE				0xffff

// EMS errors
#define EMS_NO_ERROR			0x00
#define EMS_SOFT_MAL			0x80
#define EMS_HARD_MAL			0x81
#define EMS_INVALID_HANDLE		0x83
#define EMS_FUNC_NOSUP			0x84
#define EMS_OUT_OF_HANDLES		0x85
#define EMS_SAVEMAP_ERROR		0x86
#define EMS_OUT_OF_PHYS			0x87
#define EMS_OUT_OF_LOG			0x88
#define EMS_ZERO_PAGES			0x89
#define EMS_LOG_OUT_RANGE		0x8a
#define EMS_ILL_PHYS			0x8b
#define EMS_PAGE_MAP_SAVED		0x8d
#define EMS_NO_SAVED_PAGE_MAP	0x8e
#define EMS_INVALID_SUB			0x8f
#define EMS_FEAT_NOSUP			0x91
#define EMS_MOVE_OVLAP			0x92
#define EMS_MOVE_OVLAPI			0x97
#define EMS_NOT_FOUND			0xa0

class EMSPageHandler : public PageHandler
	{
public:
	EMSPageHandler()
		{
		flags = PFLAG_READABLE|PFLAG_WRITEABLE;
		}
	Bit8u readb(PhysPt addr)
		{
		return *(PageBase+(addr&(EMS_PAGE_SIZE-1)));
		}
	Bit16u readw(PhysPt addr)
		{
		return *(Bit16u*)(PageBase+(addr&(EMS_PAGE_SIZE-1)));
		}
	Bit32u readd(PhysPt addr)
		{
		return *(Bit32u*)(PageBase+(addr&(EMS_PAGE_SIZE-1)));
		}
	void writeb(PhysPt addr, Bit8u val)
		{
		*(PageBase+(addr&(EMS_PAGE_SIZE-1))) = val;
		}
	void writew(PhysPt addr, Bit16u val)
		{
		*(Bit16u*)(PageBase+(addr&(EMS_PAGE_SIZE-1))) = val;
		}
	void writed(PhysPt addr, Bit32u val)
		{
		*(Bit32u*)(PageBase+(addr&(EMS_PAGE_SIZE-1))) = val;
		}
	HostPt GetHostPt(PhysPt addr)
		{
		return PageBase+(addr&(EMS_PAGE_SIZE-1));
		}
	HostPt PageBase;
	};

struct EMS_Mapping {
	Bit16u handle;
	Bit16u page;
};

struct EMS_Handle {
	Bit16u pages;
	PhysPt addr;
	bool saved_page_map;
	EMS_Mapping page_map[EMS_MAX_PHYS];
};

static EMS_Handle ems_handles[EMS_HANDLES];
static EMS_Mapping mappings[EMS_MAX_PHYS];
static EMSPageHandler pHandler[EMS_MAX_PHYS];


class device_EMM : public DOS_Device
	{
public:
	device_EMM()
		{
		SetName("EMMXXXX0");
		}
	bool Read(Bit8u * /*data*/, Bit16u * /*size*/) { return false;}
	bool Write(Bit8u * /*data*/, Bit16u * /*size*/) { return false;}
	bool Seek(Bit32u * /*pos*/, Bit32u /*type*/) { return false;}
	void Close(){}
	Bit16u GetInformation(void) { return 0xc0c0;}
	};

static bool ValidHandle(Bit16u handle)
	{
	if (handle >= EMS_HANDLES || ems_handles[handle].pages == NULL_PAGE)
		return false;
	return true;
	}


static Bit16u EMS_Defrag()															// Defrag EMS memory and return free space in pages
	{
	Bit16u sortedHandles[EMS_HANDLES-1];
	int numHandles = 0;
	for (int i = 1; i < EMS_HANDLES; i++)
		if (ems_handles[i].pages != NULL_PAGE)
			sortedHandles[numHandles++] = i;
	if (!numHandles)																// If no handles in use, return all EMS memory as free
		return TotEMSMB*1024*1024/MEM_PAGESIZE;

	for (int i = 0; i < numHandles-1; i++)											// Sort the handles in order of memory address
		for (int j = 0; j < numHandles-1-i; j++)
			if (ems_handles[sortedHandles[j]].addr > ems_handles[sortedHandles[j+1]].addr)
				{
				Bit16u temp = sortedHandles[j+1];
				sortedHandles[j+1] = sortedHandles[j];
				sortedHandles[j] = temp;
				}

	Bit32u lowAddr = 1024*1024;
	Bit32u moveOff = 0;
	for (int i = 0; i < numHandles; i++)											// Defrag compacting to start
		{
		EMS_Handle * handlePtr = &ems_handles[sortedHandles[i]];
		if (handlePtr->addr > lowAddr)
			{
			moveOff += handlePtr->addr - lowAddr;
			for (int j = 0; j < EMS_MAX_PHYS; j++)									// Relocate page handles if needed
				if ((Bit32u)(pHandler[j].PageBase-MemBase) >= handlePtr->addr && (Bit32u)(pHandler[j].PageBase-MemBase) < handlePtr->addr + handlePtr->pages*EMS_PAGE_SIZE)
					pHandler[j].PageBase -= moveOff;
			memmove(MemBase+lowAddr, MemBase+handlePtr->addr, handlePtr->pages*EMS_PAGE_SIZE);
			handlePtr->addr = lowAddr;
			}
		lowAddr += handlePtr->pages*EMS_PAGE_SIZE;
		}
	return (Bit16u)((((TotEMSMB+1)*1024*1024)-lowAddr)/EMS_PAGE_SIZE);
	}

static Bit16u EMS_GetFreePages(void)
	{
	Bitu free = TotEMSMB*1024*1024/MEM_PAGESIZE;
	for (Bit16u i = 1; i < EMS_HANDLES; i++)
		if (ems_handles[i].pages != NULL_PAGE)
			free -= ems_handles[i].pages;
	return (Bit16u)free;
	}

Bit16u EMS_FreeKBs()
	{
	return TotEMSMB ? EMS_GetFreePages()*16 : 0;
	}

static Bit8u EMS_AllocateMemory(Bit16u pages, Bit16u & dhandle)
	{
	if (!pages)																		// 0 page allocation no allowed
		return EMS_ZERO_PAGES;
	Bit16u handle = 1;
	while (ems_handles[handle].pages != NULL_PAGE)									// Any free handle?
		if (++handle == EMS_HANDLES)
			return EMS_OUT_OF_HANDLES;
	Bit16u freePages = EMS_Defrag();
	if (freePages < pages)															// Enough free pages?
		return EMS_OUT_OF_LOG;
	ems_handles[handle].pages = pages;
	ems_handles[handle].addr = (TotEMSMB+1)*1024*1024-freePages*EMS_PAGE_SIZE;
	ems_handles[handle].saved_page_map = false;
	dhandle = handle;
	return EMS_NO_ERROR;
	}

static Bit8u EMS_MapPage(Bitu phys_page, Bit16u handle, Bit16u log_page)
	{
	if (!ValidHandle(handle))														// Valid handle?
		return EMS_INVALID_HANDLE;
	if (phys_page >= EMS_MAX_PHYS)													// Valid physical page?
		return EMS_ILL_PHYS;
	if (log_page >= ems_handles[handle].pages)										// Valid logical page?
		return EMS_LOG_OUT_RANGE;
	
	mappings[phys_page].handle = handle;
	mappings[phys_page].page = log_page;
	pHandler[phys_page].PageBase = MemBase+ems_handles[handle].addr+log_page*EMS_PAGE_SIZE;
	return EMS_NO_ERROR;
	}

static Bit8u EMS_ReleaseMemory(Bit16u handle)
	{
	if (!ValidHandle(handle))														// Check for valid handle
		return EMS_INVALID_HANDLE;
	if (handle != 0)																// Not sure what to do with eventual refering page handle(s)
		ems_handles[handle].pages = NULL_PAGE;
	return EMS_NO_ERROR;
	}

static Bit8u EMS_SavePageMap(Bit16u handle)
	{
	if (!ValidHandle(handle))														// Check for valid handle
		return EMS_INVALID_HANDLE;
	for (Bitu i = 0; i < EMS_MAX_PHYS; i++)
		{
		ems_handles[handle].page_map[i].page = mappings[i].page;
		ems_handles[handle].page_map[i].handle = mappings[i].handle;
		}
	ems_handles[handle].saved_page_map = true;
	return EMS_NO_ERROR;
	}

static Bit8u EMS_RestorePageMap(Bit16u handle)
	{
	if (!ValidHandle(handle))														// Check for valid handle
		return EMS_INVALID_HANDLE;
	if (!ems_handles[handle].saved_page_map)										// Check for previous save
		return EMS_NO_SAVED_PAGE_MAP;
	for (Bitu i = 0; i < EMS_MAX_PHYS; i++)
		EMS_MapPage(i, ems_handles[handle].page_map[i].handle, ems_handles[handle].page_map[i].page);
	return EMS_NO_ERROR;
	}

static Bit8u EMS_GetPagesForAllHandles(PhysPt table, Bit16u & handles)
	{
	handles = 0;
	for (Bit16u i = 0; i < EMS_HANDLES; i++)
		if (ems_handles[i].pages != NULL_PAGE)
			{
			handles++;
			Mem_Stosw(table, i);
			Mem_Stosw(table+2, ems_handles[i].pages);
			table += 4;
			}
	return EMS_NO_ERROR;
	}


static Bitu INT67_Handler(void)
	{
	switch (reg_ah)
		{
	case 0x40:																		// Get status
		reg_ah = EMS_NO_ERROR;	
		break;
	case 0x41:																		// Get pageframe segment
		reg_bx = EMS_PAGEFRAME;
		reg_ah = EMS_NO_ERROR;
		break;
	case 0x42:																		// Get number of pages 
		reg_bx = EMS_GetFreePages();
		reg_dx = TotEMSMB*1024*1024/MEM_PAGESIZE;
		reg_ah = EMS_NO_ERROR;
		break;
	case 0x43:																		// Request handle and allocate pages
		reg_ah = EMS_AllocateMemory(reg_bx, reg_dx);
		break;
	case 0x44:																		// Map expanded memory page
		reg_ah = EMS_MapPage(reg_al, reg_dx, reg_bx);
		break;
	case 0x45:																		// Release handle and free pages
		reg_ah = EMS_ReleaseMemory(reg_dx);
		break;
	case 0x46:																		// Get EMS version
		reg_al = EMS_VERSION;
		reg_ah = EMS_NO_ERROR;
		break;
	case 0x47:																		// Save page map
		reg_ah = EMS_SavePageMap(reg_dx);
		break;
	case 0x48:																		// Restore page map
		reg_ah = EMS_RestorePageMap(reg_dx);
		break;
	case 0x4b:																		// Get handle count
		reg_bx = 0;
		for (Bitu i = 0; i < EMS_HANDLES; i++)
			if (ems_handles[i].pages != NULL_PAGE)
				reg_bx++;
		reg_bx = 64;
		reg_ah = EMS_NO_ERROR;
		break;
	case 0x4c:																		// Get pages for one handle
		if (!ValidHandle(reg_dx))
			{
			reg_ah = EMS_INVALID_HANDLE;
			break;
			}
		reg_bx = ems_handles[reg_dx].pages;
		reg_ah = EMS_NO_ERROR;
		break;
	case 0x4d:																		// Get pages for all handles
		reg_ah = EMS_GetPagesForAllHandles(SegPhys(es)+reg_di, reg_bx);
		break;
	default:
		reg_ah = EMS_FUNC_NOSUP;
		break;
		}
	return CBRET_NONE;
	}

void EMS_Init()
	{
	if (TotEMSMB)
		{
		Bit16u ems_baseseg = DOS_GetPrivatMemory(2);								// Request 32 bytes block from DOS
		Mem_CopyTo(SegOff2Ptr(ems_baseseg, 0xa), "EMMXXXX0", 8);
		Bitu cbID = CALLBACK_Allocate();
		CALLBACK_Setup(cbID, &INT67_Handler, CB_IRET, SegOff2Ptr(ems_baseseg, 4));	// Int 67 EMS
		RealSetVec(0x67, SegOff2dWord(ems_baseseg, 4));
	
		DOS_AddDevice(new device_EMM());											// Register the EMS device
		ems_handles[0].pages = 0;
		for (int i = 1; i < EMS_HANDLES; i++)										// Clear handle and page tables
			ems_handles[i].pages = NULL_PAGE;
		for (int i = 0; i < EMS_MAX_PHYS; i++)
			{
			mappings[i].page = NULL_PAGE;
			pHandler[i].PageBase = MemBase+(EMS_PAGEFRAME+i)*16;
			MEM_SetPageHandler(EMS_PAGEFRAME*16/MEM_PAGESIZE+i, 1, &pHandler[i]);
			}
		}
	}
