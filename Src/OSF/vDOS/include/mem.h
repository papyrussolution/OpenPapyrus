#ifndef VDOS_MEM_H
#define VDOS_MEM_H

#ifndef VDOS_H
#include "vDos.h"
#endif

typedef Bit32u PhysPt;
typedef Bit32u LinPt;
typedef Bit8u * HostPt;
typedef Bit32u RealPt;

#define MEM_PAGESIZE (16*1024)														// Set to 16KB page size of EMS
#define MEM_PAGES (4096)															// Number of pages for max 64MB

extern HostPt MemBase;

extern unsigned int TotMemBytes;
extern unsigned int TotMemMB;
extern unsigned int TotEXTMB;
extern unsigned int TotXMSMB;
extern unsigned int TotEMSMB;
extern unsigned int EndConvMem;

// These functions address memory directly (no paging or address checking is needed)
Bit8u  Mem_aLodsb(PhysPt pt);
Bit16u Mem_aLodsw(PhysPt addr);
Bit32u Mem_aLodsd(PhysPt addr);

// These functions recognize the paged memory system
Bit8u  Mem_Lodsb(LinPt pt);
Bit16u Mem_Lodsw(LinPt pt);
Bit32u Mem_Lodsd(LinPt pt);
void Mem_Stosb(LinPt pt, Bit8u val);
void Mem_Stosw(LinPt pt, Bit16u val);
void Mem_Stosd(LinPt pt, Bit32u val);
void Mem_rStosb(LinPt address, Bit8u val, Bitu count);
void Mem_rStos4b(LinPt address, Bit32u val, Bitu count);

void clearTLB();


inline void Mem_rStosw(LinPt addr, Bit16u val, Bitu count)
	{
	Mem_rStos4b(addr, val + (val<<16), count*2);
	}

inline void Mem_rStosd(LinPt addr, Bit32u val, Bitu count)
	{
	Mem_rStos4b(addr, val, count*4);
	}

static void Mem_aStosb(PhysPt addr, Bit8u val)
	{
	*(Bit8u *)(MemBase+addr) = val;
	}

static void Mem_aStosw(PhysPt addr, Bit16u val)
	{
	*(Bit16u *)(MemBase+addr) = val;
	}

static void Mem_aStosd(PhysPt addr, Bit32u val)
	{
	*(Bit32u *)(MemBase+addr) = val;
	}

static void Mem_aWrites(PhysPt addr, const char* string, Bitu length)
	{
	memcpy(MemBase+addr, string, length);
	}

void Mem_rMovsb(LinPt dest, LinPt src, Bitu bSize);
void Mem_CopyTo(LinPt pt, void const * const data, Bitu size);
void Mem_CopyFrom(PhysPt pt, void * data, Bitu size);
void Mem_StrnCopyFrom(char * data, PhysPt pt, Bitu size);

Bitu Mem_StrLen(LinPt pt);

// The folowing functions are all shortcuts to the above functions using physical addressing

static void Mem_Stosw(Bit16u seg, Bit16u off, Bit16u val)
	{
	Mem_Stosw(((seg<<4)+off), val);
	}

static void Mem_Stosd(Bit16u seg, Bit16u off, Bit32u val)
	{
	Mem_Stosd(((seg<<4)+off), val);
	}

static Bit16u RealSeg(RealPt pt)
	{
	return (Bit16u)(pt>>16);
	}

static Bit16u RealOff(RealPt pt)
	{
	return (Bit16u)(pt&0xffff);
	}

static PhysPt dWord2Ptr(RealPt pt)
	{
	return (RealSeg(pt)<<4)+RealOff(pt);
	}

static PhysPt SegOff2Ptr(Bit16u seg, Bit16u off)
	{
	return (seg<<4)+off;
	}

static RealPt SegOff2dWord(Bit16u seg, Bit16u off)
	{
	return (seg<<16)+off;
	}

static void RealSetVec(Bit8u vec, RealPt pt)
	{
	Mem_aStosd(vec<<2, pt);
	}

static RealPt RealGetVec(Bit8u vec)
	{
	return Mem_aLodsd(vec<<2);
	}	

#endif

