#include <stdlib.h>
#include "vDos.h"
#include "mem.h"
#include "paging.h"
#include "regs.h"
#include "cpu.h"

PagingBlock paging;

Bit8u PageHandler::readb(PhysPt addr)
	{
	E_Exit("No read handler for address %x", addr);	
	return 0;
	}

Bit16u PageHandler::readw(PhysPt addr)
	{
	return readb(addr+0)|(readb(addr+1) << 8);
	}

Bit32u PageHandler::readd(PhysPt addr)
	{
	return readb(addr+0)|(readb(addr+1)<<8)|(readb(addr+2)<<16)|(readb(addr+3)<<24);
	}

void PageHandler::writeb(PhysPt addr, Bit8u /*val*/)
	{
	E_Exit("No write handler for address %x", addr);	
	}

void PageHandler::writew(PhysPt addr, Bit16u val)
	{
	writeb(addr+0, (Bit8u)(val >> 0));
	writeb(addr+1, (Bit8u)(val >> 8));
	}

void PageHandler::writed(PhysPt addr, Bit32u val)
	{
	writeb(addr+0, (Bit8u)(val >> 0));
	writeb(addr+1, (Bit8u)(val >> 8));
	writeb(addr+2, (Bit8u)(val >> 16));
	writeb(addr+3, (Bit8u)(val >> 24));
	}

HostPt PageHandler::GetHostPt(PhysPt addr)
	{
	E_Exit("Access to invalid memory location %x", addr);
	return 0;
	}

void PAGING_SetDirBase(Bitu cr3)
	{
	if (paging.cr3 != cr3)
		{
		if (cr3&0xfff)
			E_Exit("Page fault: address (%x) CR3 not page aligned", cr3);
		paging.cr3 = cr3;
		clearFetchCache();
		clearTLB();
		}
	}

void PAGING_Enable(bool enabled)
	{
	paging.enabled = enabled;
	clearFetchCache();
//	if (paging.enabled != enabled)
//		if (!(paging.enabled = enabled))
//			clearFetchCache();
	}

//void PAGING_Init()
//	{
//	}
