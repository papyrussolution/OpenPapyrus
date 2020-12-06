#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include "vDos.h"
#include "mem.h"
#include "inout.h"
#include "paging.h"
#include "cpu.h"
#include "bios.h"
#include "video.h"

static PageHandler * pageHandlers[64];												// Pagehandlers for first MB

unsigned int TotMemBytes;
unsigned int TotMemMB = 1;
unsigned int TotEXTMB = 0;
unsigned int TotXMSMB = 0;
unsigned int TotEMSMB = 0;
unsigned int EndConvMem = 0x9fff;

HostPt MemBase;

class RAMPageHandler : public PageHandler
	{
public:
	RAMPageHandler()
		{
		flags = PFLAG_READABLE|PFLAG_WRITEABLE;
		}
	Bit8u readb(PhysPt addr)
		{
		return *(MemBase+addr);
		}
	Bit16u readw(PhysPt addr)
		{
		return *(Bit16u*)(MemBase+addr);
		}
	Bit32u readd(PhysPt addr)
		{
		return *(Bit32u*)(MemBase+addr);
		}
	void writeb(PhysPt addr, Bit8u val)
		{
		*(MemBase+addr) = val;
		}
	void writew(PhysPt addr, Bit16u val)
		{
		*(Bit16u*)(MemBase+addr) = val;
		}
	void writed(PhysPt addr, Bit32u val)
		{
		*(Bit32u*)(MemBase+addr) = val;
		}
	HostPt GetHostPt(PhysPt addr)
		{
		return MemBase+addr;
		}
	};

class ROMPageHandler : public RAMPageHandler
	{
public:
	ROMPageHandler()
		{
		flags = PFLAG_READABLE;
		}
	void writeb(PhysPt addr, Bit8u val)
		{
		}
	void writew(PhysPt addr, Bit16u val)
		{
		}
	void writed(PhysPt addr, Bit32u val)
		{
		}
	};

class ILLPageHandler : public RAMPageHandler
	{
public:
	ILLPageHandler()
		{
		flags = 0;
		}
	Bit8u readb(PhysPt addr)
		{
		return 0xff;
		}
	Bit16u readw(PhysPt addr)
		{
		return 0xffff;
		}
	Bit32u readd(PhysPt addr)
		{
		return 0xffffffff;
		}
	void writeb(PhysPt addr, Bit8u val)
		{
		}
	void writew(PhysPt addr, Bit16u val)
		{
		}
	void writed(PhysPt addr, Bit32u val)
		{
		}
	};


static RAMPageHandler ram_page_handler;
static ROMPageHandler rom_page_handler;
static ILLPageHandler ill_page_handler;

#define TLB_SIZE 128															// 128 ebtries in TLB cache (it get cleared most often)
//#define TLB_SIZE 8192															// 8192 ebtries in TLB cache
static Bit32u TLB_phys[TLB_SIZE];
static Bit32u TLB_cache[TLB_SIZE];

void clearTLB(void)
	{
	memset(TLB_cache, 0xff, TLB_SIZE*4);
	}

PhysPt LinToPhys2(LinPt addr)
	{
	int TLB_idx = addr>>12;
	Bit32u pdAddr = (TLB_idx>>8)&0xffc;
	Bit32u pdEntry = *(Bit32u *)(MemBase+PAGING_GetDirBase()+pdAddr);
	if (pdEntry&1)
		{
		Bit32u ptAddr = (pdEntry&~0xfff)+((TLB_idx<<2)&0xffc);
		Bit32u ptEntry = *(Bit32u *)(MemBase+ptAddr);
		if (ptEntry&1)
			{
			TLB_cache[TLB_idx&(TLB_SIZE-1)] = TLB_idx;
			TLB_phys[TLB_idx&(TLB_SIZE-1)] = ptEntry&~0xfff;
			return TLB_phys[TLB_idx&(TLB_SIZE-1)]+(addr&0xfff);
			}
		}
	E_Exit("Page fault: directory or table entry not present: %x -> %03x : %03x : %03x", addr, TLB_idx>>10, TLB_idx&0x3ff, addr&0xfff);
	return 0;																		// To satisfy compiler
	}

__forceinline PhysPt LinToPhys(LinPt addr)
	{
	int TLB_idx = addr>>12;
	if (TLB_cache[TLB_idx&(TLB_SIZE-1)] == TLB_idx)
		return TLB_phys[TLB_idx&(TLB_SIZE-1)]+(addr&0xfff);
	return LinToPhys2(addr);
	}

__forceinline PageHandler * MEM_GetPageHandler(PhysPt addr)
	{
	if (addr < 0x100000)															// If in first MB
		return pageHandlers[addr/MEM_PAGESIZE];
	if (addr < TotMemBytes)															// If in extended (or XMS)
		return &ram_page_handler;
	return &ill_page_handler;
	}

void MEM_SetPageHandler(Bitu phys_page, Bitu pages, PageHandler * handler)
	{
	for (; pages > 0; pages--)
		pageHandlers[phys_page++] = handler;
	}

void MEM_ResetPageHandler(Bitu phys_page, Bitu pages)
	{
	for (; pages > 0; pages--)
		pageHandlers[phys_page++] = &ram_page_handler;
	}

void unhide_vDos()
	{
	if (winHidden && window.hideTill)												// Unhide window on access
		{
		Bit32u tickCount = GetTickCount();
		if (window.hideTill-tickCount < 2300)										// Prevent 4DOS forcing unhide at initialization
			window.hideTill = 0;													// And hide some DOS program intro screens (0.2 sec delay)
		}
	else
		idleCount++;
	}

Bit8u Mem_aLodsb(PhysPt addr)
	{
	return *(Bit8u *)(MemBase+addr);
	}

Bit8u Mem_Lodsb(LinPt addr)
	{
	if (PAGING_Enabled())
		addr = LinToPhys(addr);
	if (addr < 0xa0000)																// If in lower memory, read direct (always readable)
		{
		if ((Bit32u)(addr-BIOS_KEYBOARD_FLAGS1) <= 7)								// Access to keyboard info
			unhide_vDos();
		return *(MemBase+addr);
		}
	if ((addr >= 0xf0000 && addr < TotMemBytes))									// If in last 64KB or extended mem, read direct (always readable)
		return *(MemBase+addr);
	return MEM_GetPageHandler(addr)->readb(addr);
	}

Bit16u Mem_aLodsw(PhysPt addr)
	{
	if ((Bit32u)(addr-BIOS_KEYBOARD_FLAGS1) <= 7)									// Access to keyboard info
		unhide_vDos();
	return *(Bit16u *)(MemBase+addr);
	}

Bit16u Mem_Lodsw(LinPt addr)
	{
	if (PAGING_Enabled())
		addr = LinToPhys(addr);
	if (addr < 0x9ffff)																// If in lower mem, read direct (always readable)
		{
		if ((Bit32u)(addr-BIOS_KEYBOARD_FLAGS1) <= 7)								// Access to keyboard info
			unhide_vDos();
		return *(Bit16u *)(MemBase+addr);
		}
	if ((addr >= 0xf0000 && addr < TotMemBytes-1))									// If in last 64 KB ir extended mem, read direct (always readable)
		return *(Bit16u *)(MemBase+addr);
	if ((addr&(MEM_PAGESIZE-1)) != (MEM_PAGESIZE-1))
		return MEM_GetPageHandler(addr)->readw(addr);
	Bit16u ret = 0;
	for (Bit32u i = addr+1; i >= addr; i--)
		{
		ret <<= 8;
		ret |= MEM_GetPageHandler(i)->readb(i);
		}
	return ret;
	}

Bit32u Mem_aLodsd(PhysPt addr)
	{
	return *(Bit32u *)(MemBase+addr);
	}

Bit32u Mem_Lodsd(LinPt addr)
	{
	if (PAGING_Enabled())
		addr = LinToPhys(addr);
	if (addr < 0x9fffd || (addr >= 0xf0000 && addr < TotMemBytes-3))				// In lower, last 64 KB, or extended mem, read direct (always readable)
		return *(Bit32u *)(MemBase+addr);
	if ((addr&(MEM_PAGESIZE-1)) < (MEM_PAGESIZE-3))
		return MEM_GetPageHandler(addr)->readd(addr);
	Bit32u ret = 0;
	for (Bit32u i = addr+3; i >= addr; i--)
		{
		ret <<= 8;
		ret |= MEM_GetPageHandler(i)->readb(i);
		}
	return ret;
	}

void Mem_Stosb(LinPt addr, Bit8u val)
	{
	if (PAGING_Enabled())
		addr = LinToPhys(addr);

	if (addr < 0xa0000 || (addr > 0xfffff && addr < TotMemBytes))					// If in lower or extended mem, write direct (always writeable)
		{
		*(MemBase+addr) = val;
		return;
		}
	MEM_GetPageHandler(addr)->writeb(addr, val);
	}

void Mem_Stosw(LinPt addr, Bit16u val)
	{
	if (PAGING_Enabled())
		addr = LinToPhys(addr);
	if (addr < 0x9ffff || (addr > 0xfffff && addr < TotMemBytes-1))					// If in lower or extended mem, write direct (always writeable)
		{
		*(Bit16u *)(MemBase+addr) = val;
		return;
		}
	if ((addr&(MEM_PAGESIZE-1)) != (MEM_PAGESIZE-1))
		MEM_GetPageHandler(addr)->writew(addr, val);
	else
		for (int i = 0; i < 2; i++)
			{
			MEM_GetPageHandler(addr)->writeb(addr, val&0xff);
			val >>= 8;
			addr++;
			}
	}

void Mem_Stosd(LinPt addr, Bit32u val) 
{
	if (PAGING_Enabled())
		addr = LinToPhys(addr);
	if (addr < 0x9fffd || (addr > 0xfffff && addr < TotMemBytes-3))					// If in lower or extended mem, write direct (always writeable)
		{
		*(Bit32u *)(MemBase+addr) = val;
		return;
		}
	if ((addr&(MEM_PAGESIZE-1)) < (MEM_PAGESIZE-3))
		MEM_GetPageHandler(addr)->writed(addr, val);
	else
		for (int i = 0; i < 4; i++)
			{
			MEM_GetPageHandler(addr)->writeb(addr, (Bit8u)(val&0xff));
			val >>= 8;
			addr++;
			}
	}

void Mem_Movsb(LinPt dest, LinPt src)
	{
	if (PAGING_Enabled())
		{
		src = LinToPhys(src);
		dest = LinToPhys(dest);
		}
	MEM_GetPageHandler(dest)->writeb(dest, MEM_GetPageHandler(src)->readb(src));
	}

void Mem_rMovsb(LinPt dest, LinPt src, Bitu bCount)
	{
	Bit16u maxMove = PAGING_Enabled() ? 4096 : MEM_PAGESIZE;						// If paging, use 4096 bytes page size (strictly not needed?)
	while (bCount)																	// Set in chunks of MEM_PAGESIZE for mem mapping to take effect
		{
		PhysPt physSrc = PAGING_Enabled() ? LinToPhys(src) : src;
		PhysPt physDest = PAGING_Enabled() ? LinToPhys(dest) : dest;
		Bit16u srcOff = ((Bit16u)physSrc)&(maxMove-1);
		Bit16u destOff = ((Bit16u)physDest)&(maxMove-1);
		Bit16u bTodo = maxMove - MAX(srcOff, destOff);
		if (bTodo > bCount)
			bTodo = bCount;
		src += bTodo;
		dest += bTodo;
		bCount -= bTodo;
		PageHandler * phSrc = MEM_GetPageHandler(physSrc);
		PageHandler * phDest = MEM_GetPageHandler(physDest);

		if (phDest->flags&PFLAG_WRITEABLE && phSrc->flags&PFLAG_READABLE)
			{
			Bit8u *hSrc = phSrc->GetHostPt(physSrc);
			Bit8u *hDest = phDest->GetHostPt(physDest);
			if ((hSrc <= hDest && hSrc+bTodo > hDest) || (hDest <= hSrc && hDest+bTodo > hSrc))
				while (bTodo--)														// If source and destination overlap, do it "by hand"
					*(hDest++) = *(hSrc++);											// memcpy() messes things up in another way than rep movsb does!
			else
				memcpy(hDest, hSrc, bTodo);
			}
		else																		// Not writeable, or use (VGA)handler
			while (bTodo--)
				phDest->writeb(physDest++, phSrc->readb(physSrc++));
		}
	}

Bitu Mem_StrLen(LinPt addr)
	{
	for (Bitu len = 0; len < 65536; len++)
		if (!Mem_Lodsb(addr+len))
			return len;
	return 0;																		// This shouldn't happen
	}

void Mem_CopyTo(LinPt dest, void const * const src, Bitu bCount)
	{
	Bit8u const * srcAddr = (Bit8u const *)src;
	Bit16u maxMove = PAGING_Enabled() ? 4096 : MEM_PAGESIZE;						// If paging, use 4096 bytes page size (strictly not needed?)
	while (bCount)
		{
		PhysPt physDest = PAGING_Enabled() ? LinToPhys(dest) : dest;
		Bit16u bTodo = maxMove-(((Bit16u)physDest)&(maxMove-1));
		if (bTodo > bCount)
			bTodo = bCount;
		bCount -= bTodo;
		dest += bTodo;
		PageHandler * ph = MEM_GetPageHandler(physDest);
		if (ph->flags&PFLAG_WRITEABLE)
			{
			memcpy(ph->GetHostPt(physDest), srcAddr, bTodo);
			srcAddr += bTodo;
			}
		else
			while (bTodo--)
				ph->writeb(physDest++, *srcAddr++);
		}
	}

void Mem_CopyFrom(LinPt src, void * dest, Bitu bCount)
	{
	Bit8u * destAddr = (Bit8u *)dest;
	Bit16u maxMove = PAGING_Enabled() ? 4096 : MEM_PAGESIZE;						// If paging, use 4096 bytes page size (strictly not needed?)
	while (bCount)
		{
		PhysPt physSrc = PAGING_Enabled() ? LinToPhys(src) : src;
		Bit16u bTodo = maxMove - (((Bit16u)physSrc)&(maxMove-1));
		if (bTodo > bCount)
			bTodo = bCount;
		bCount -= bTodo;
		src += bTodo;
		PageHandler * ph = MEM_GetPageHandler(physSrc);
		if (ph->flags & PFLAG_READABLE)
			{
			memcpy(destAddr, ph->GetHostPt(physSrc), bTodo);
			destAddr += bTodo;
			}
		else
			while (bTodo--)
				*destAddr++ = ph->readb(physSrc++);
		}
	}

void Mem_StrnCopyFrom(char * data, LinPt pt, Bitu bCount)
	{
	while (bCount--)
		{
		Bit8u c = Mem_Lodsb(pt++);
		if (!c)
			break;
		*data++ = c;
		}
	*data = 0;
	}

void Mem_rStos4b(LinPt addr, Bit32u val32, Bitu bCount)								// Used by rStosw and rStosd (bytes are not the same)
	{
	Bit16u maxMove = PAGING_Enabled() ? 4096 : MEM_PAGESIZE;						// If paging, use 4096 bytes page size (strictly not needed?)
	while (bCount)																	// Set in chunks of MEM_PAGESIZE for mem mapping to take effect
		{
		PhysPt physAddr = PAGING_Enabled() ? LinToPhys(addr) : addr;
		Bit16u bTodo = maxMove - (((Bit16u)physAddr)&(maxMove-1));
		if (bTodo > bCount)
			bTodo = bCount;
		PageHandler *ph = MEM_GetPageHandler(physAddr);
		addr += bTodo;
		if (ph->flags&PFLAG_WRITEABLE)
			{
			HostPt hPtr = ph->GetHostPt(physAddr);
			while ((Bit32u)(hPtr)&3) { // Align start address to 32 bit
				*(hPtr++) = (Bit8u)(val32&0xff);
				val32 = _rotr(val32, 8);
				bTodo--;
				bCount--;
				}
			bCount -= bTodo&~3;
			for (bTodo >>= 2; bTodo; bTodo--) { // Set remaing with 32 bit value
				*(Bit32u *)hPtr = val32;
				hPtr += 4;
				}
				if(bCount < 4) { // Eventually last remaining bytes
				while (bCount--) {
					*hPtr++ = (Bit8u)(val32&0xff);
					val32 >>= 8;
					}
				return;
				}
			}
		else { // Not writeable, or use (VGA)handler
				while ((Bit32u)(physAddr)&3) { // Align start address to 32 bit
				ph->writeb(physAddr++, (Bit8u)(val32&0xff));
				val32 = _rotr(val32, 8);
				bTodo--;
				bCount--;
				}
			bCount -= bTodo&~3;
			for (bTodo >>= 2; bTodo; bTodo--) { // Set remaing with 32 bit value
				ph->writed(physAddr, val32);
				physAddr += 4;
				}
				if (bCount < 4) { // Eventually last remaining bytes
				while (bCount--) {
					ph->writeb(physAddr++, (Bit8u)(val32&0xff));
					val32 >>= 8;
					}
				return;
				}
			}
		}
	}

void Mem_rStosb(LinPt addr, Bit8u val, Bitu count)
	{
	Bit16u maxMove = PAGING_Enabled() ? 4096 : MEM_PAGESIZE;						// If paging, use 4096 bytes page size (strictly not needed?)
	while (count)																	// Set in chunks of MEM_PAGESIZE for mem mapping to take effect
		{
		PhysPt physAddr = PAGING_Enabled() ? LinToPhys(addr) : addr;
		Bit16u bTodo = maxMove-(((Bit16u)physAddr)&(maxMove-1));
		if (bTodo > count)
			bTodo = count;
		count -= bTodo;
		addr += bTodo;
		PageHandler *ph = MEM_GetPageHandler(physAddr);
		if (ph->flags&PFLAG_WRITEABLE)
			memset(ph->GetHostPt(physAddr), val, bTodo);							// memset() is optimized for 32 bit
		else																		// Not writeable, or use (VGA)handler
			while (bTodo--)
				ph->writeb(physAddr++, val);
		}
	}

static void write_p92(Bitu port, Bitu val)
	{
	}

static Bit8u read_p92(Bitu port)
	{
	return 0;
	}

void MEM_Init()
	{
	char *xmem = ConfGetString("xmem");
	if (*xmem)
		{
		int testVal;
		char testStr1[512];
		char testStr2[512];
		bool error = true;
		if (*xmem == '+')															// 704K option
			{
			EndConvMem += 0x1000;
			xmem++;
			}
		if (sscanf(xmem, "%d%s%s", &testVal, testStr1, testStr2) == 2)
			{
			if (testVal > 0 && testVal < 64)
				{
				error = false;
				if (!stricmp("EXT", testStr1))
					TotEXTMB = testVal;
				else if (!stricmp("XMS", testStr1))
					TotXMSMB = testVal;
				else if (!stricmp("EMS", testStr1))
					TotEMSMB = testVal;
				else
					error = true;
				if (!error)
					TotMemMB = TotEXTMB+TotXMSMB+1;
				}
			}
		if (error)
			ConfAddError("Invalid XMEM= parameters\n", xmem);
		}
	else
		{
		TotXMSMB = 4;																// Default 4MB XMS
		TotMemMB = TotXMSMB+1;
		}

	MemBase = (Bit8u *)_aligned_malloc((TotMemMB+TotEMSMB)*1024*1024, MEM_PAGESIZE);// Setup the physical memory
	if (!MemBase)
		E_Exit("Can't allocate main memory of %d MB", TotMemMB+TotEMSMB);
	TotMemBytes = TotMemMB*1024*1024;
	memset((void*)MemBase, 0, TotMemBytes);											// Clear the memory
		
	for (Bitu i = 0; i < 64; i++)													// Setup handlers for first MB
		pageHandlers[i] = &ram_page_handler;
	for (Bitu i = 0xc0000/MEM_PAGESIZE; i < 0xc4000/MEM_PAGESIZE; i++)				// Setup rom at 0xc0000-0xc3fff
		pageHandlers[i] = &rom_page_handler;
	for (Bitu i = 0xf0000/MEM_PAGESIZE; i < 0x100000/MEM_PAGESIZE; i++)				// Setup rom at 0xf0000-0xfffff
		pageHandlers[i] = &rom_page_handler;

	IO_RegisterWriteHandler(0x92, write_p92);										// (Dummy) A20 Line - PS/2 system control port A
	IO_RegisterReadHandler(0x92, read_p92);
	}
