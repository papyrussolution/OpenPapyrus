#include "vDos.h"
#include "mem.h"
#include "dos_inc.h"
#include "callback.h"

#define MAXENV 512

static Bit16u memAllocStrategy = 0;

static void DOS_DefragFreeMCB(void)
	{
	Bit16u mcb_segment = dos.firstMCB;
	DOS_MCB mcb(mcb_segment);
	DOS_MCB mcb_next(0);
	Bitu counter = 0;
	while (mcb.GetType() != 'Z')
		{
		if (counter++ > 1000)
			E_Exit("DOS MCB list is corrupt!");
		mcb_next.SetPt((Bit16u)(mcb_segment+mcb.GetSize()+1));
		if ((mcb.GetPSPSeg() == MCB_FREE) && (mcb_next.GetPSPSeg() == MCB_FREE))
			{
			mcb.SetSize(mcb.GetSize()+mcb_next.GetSize()+1);
			mcb.SetType(mcb_next.GetType());
			}
		else
			{
			mcb_segment += mcb.GetSize()+1;
			mcb.SetPt(mcb_segment);
			if (mcb.GetType() == 'Z' && mcb_segment < EndConvMem && dos_infoblock.GetStartOfUMBChain() == EndConvMem)	// to UMB if used
				{
				mcb_segment = EndConvMem;
				mcb.SetPt(mcb_segment);
				}
			}
		}
	}

void DOS_FreeProcessMemory(Bit16u pspseg)
	{
	Bit16u mcb_segment = dos.firstMCB;
	DOS_MCB mcb(mcb_segment);

	for (int counter = 0; ;)
		{
		if (counter++ > 1000)
			E_Exit("DOS MCB list is corrupt!");
		if (mcb.GetPSPSeg() == pspseg)
			mcb.SetPSPSeg(MCB_FREE);
		if (mcb.GetType() == 'Z')
			if (mcb_segment < EndConvMem && dos_infoblock.GetStartOfUMBChain() == EndConvMem)	// to UMB if used
				mcb_segment = EndConvMem;
			else
				break;
		else
			mcb_segment += mcb.GetSize()+1;
		mcb.SetPt(mcb_segment);
		}
	DOS_DefragFreeMCB();
	}


/*
Values for DOS memory allocation strategy:
	00h low memory first fit
	01h low memory best fit
	02h low memory last fit
	40h high memory first fit
	41h high memory best fit
	42h high memory last fit
	80h first fit, try high then low memory
	81h best fit, try high then low memory
	82h last fit, try high then low memory
*/

Bit16u DOS_GetMemAllocStrategy()
	{
	return memAllocStrategy;
	}

bool DOS_SetMemAllocStrategy(Bit16u strat)
	{
	if (((strat&0x43) == strat || (strat&0x83) == strat) && (strat&0x3) < 3)
		{
		memAllocStrategy = strat;
		return true;
		}
	return false;
	}

bool DOS_GetFreeUMB(Bit16u * total, Bit16u * largest, Bit16u * count)
	{
	*total = *largest = *count = 0;
	Bit16u mcb_segment = dos_infoblock.GetStartOfUMBChain();
	if (mcb_segment != EndConvMem)
		return false;

	DOS_DefragFreeMCB();
	DOS_MCB mcb(mcb_segment);
	while (1)
		{
		Bit16u mcb_size = mcb.GetSize();
		if (mcb.GetPSPSeg() == MCB_FREE)
			{
			*count += 1;
			*total += mcb_size;
			if (*largest < mcb_size)
				*largest = mcb_size;
			}
		if (mcb.GetType() == 'Z')
			break;
		mcb_segment += mcb_size+1;
		mcb.SetPt(mcb_segment);
		}
	return true;
	}

bool DOS_AllocateMemory(Bit16u * segment, Bit16u * reqBlocks)
	{
	DOS_DefragFreeMCB();															// Mainly to check the NCB chain
	Bit16u bigsize = 0;
	Bit16u mem_strat = memAllocStrategy;
	Bit16u mcb_segment = dos.firstMCB;

	Bit16u umb_start = dos_infoblock.GetStartOfUMBChain();
	if (umb_start == EndConvMem && (mem_strat&0xc0))								// Start with UMBs if requested (bits 7 or 6 set)
		mcb_segment = umb_start;

	DOS_MCB mcb(0);
	DOS_MCB mcb_next(0);
	DOS_MCB psp_mcb(dos.psp()-1);
	char psp_name[9];
	psp_mcb.GetFileName(psp_name);
	Bit16u foundSeg = 0, foundSize = 0;
	for (;;)
		{
		mcb.SetPt(mcb_segment);
		if (mcb.GetPSPSeg() == MCB_FREE)
			{
			Bit16u block_size = mcb.GetSize();			
			if (block_size < (*reqBlocks))											// Check for enough free memory in current block
				{
				if (bigsize < block_size)											// Current block is largest block that was found, but still not as big as requested
					bigsize = block_size;
				}
			else if ((block_size == *reqBlocks) && ((mem_strat & 0x3f) < 2))		// MCB fits precisely, use it if search strategy is firstfit or bestfit
				{
				mcb.SetPSPSeg(dos.psp());
				mcb.SetFileName(psp_name);
				*segment = mcb_segment+1;
				return true;
				}
			else																	// Found block is larger than requested
				{
				switch (mem_strat & 0x3f)
					{
				case 0:																// Firstfit
					mcb_next.SetPt((Bit16u)(mcb_segment+*reqBlocks+1));
					mcb_next.SetPSPSeg(MCB_FREE);
					mcb_next.SetType(mcb.GetType());
					mcb_next.SetSize(block_size-*reqBlocks-1);
					mcb.SetSize(*reqBlocks);
					mcb.SetType('M');		
					mcb.SetPSPSeg(dos.psp());
					mcb.SetFileName(psp_name);
					*segment = mcb_segment+1;
					return true;
				case 1:																// Bestfit
					if ((foundSize == 0) || (block_size < foundSize))				// First fitting MCB, or smaller than the last that was found
						{
						foundSeg = mcb_segment;
						foundSize = block_size;
						}
					break;
				default:															// Everything else is handled as lastfit by DOS
					foundSeg = mcb_segment;											// MCB is large enough, note it down
					foundSize = block_size;
					break;
					}
				}
			}
		
		if (mcb.GetType() == 'Z')													// Onward to the next MCB if there is one
			{
			if ((mem_strat&0x80) && (umb_start == EndConvMem))
				{
				mcb_segment = dos.firstMCB;											// Bit 7 set: try upper memory first, then low
				mem_strat &= (~0xc0);
				}
			else																	// Finished searching all requested MCB chains
				{
				if (foundSeg)														// A matching MCB was found (cannot occur for firstfit)
					{
					if ((mem_strat&0x3f) == 1)										// Bestfit, allocate block at the beginning of the MCB
						{
						mcb.SetPt(foundSeg);
						mcb_next.SetPt((Bit16u)(foundSeg+*reqBlocks+1));
						mcb_next.SetPSPSeg(MCB_FREE);
						mcb_next.SetType(mcb.GetType());
						mcb_next.SetSize(foundSize-*reqBlocks-1);
						mcb.SetSize(*reqBlocks);
						mcb.SetType('M');		
						mcb.SetPSPSeg(dos.psp());
						mcb.SetFileName(psp_name);
						*segment = foundSeg+1;
						}
					else															// Lastfit, allocate block at the end of the MCB
						{
						mcb.SetPt(foundSeg);
						if (foundSize == *reqBlocks)								// If requested size
							{
							mcb.SetPSPSeg(dos.psp());
							mcb.SetFileName(psp_name);
							*segment = foundSeg+1;
							return true;
							}
						*segment = foundSeg+1+foundSize-*reqBlocks;
						mcb_next.SetPt((Bit16u)(*segment-1));
						mcb_next.SetSize(*reqBlocks);
						mcb_next.SetType(mcb.GetType());
						mcb_next.SetPSPSeg(dos.psp());
						mcb_next.SetFileName(psp_name);
						mcb.SetSize(foundSize-*reqBlocks-1);
						mcb.SetPSPSeg(MCB_FREE);
						mcb.SetType('M');
						}
					return true;
					}
				*reqBlocks = bigsize;												// No fitting MCB found, return size of largest block
				DOS_SetError(DOSERR_INSUFFICIENT_MEMORY);
				return false;
				}
			}
		else
			mcb_segment += mcb.GetSize()+1;
		}
	return false;
	}

bool DOS_ResizeMemory(Bit16u segment, Bit16u * blocks)
	{
	if (segment < DOS_MEM_START+1)
		E_Exit("Program tried to resize MCB block %X", segment);
	DOS_MCB mcb(segment-1);
	Bit8u mcb_type = mcb.GetType();
	if (mcb_type != 'M' && mcb_type != 'Z')
		{
		DOS_SetError(DOSERR_MCB_DESTROYED);
		return false;
		}

	Bit16u total = mcb.GetSize();
	if (*blocks == total)															// Same size, nothing to do
		return true;
	if (*blocks < total)															// Shrinking MCB
		{
		DOS_MCB	mcb_next(segment+(*blocks));
		mcb_next.SetType(mcb_type);
		mcb_next.SetSize(total-*blocks-1);
		mcb_next.SetPSPSeg(MCB_FREE);
		mcb.SetSize(*blocks);
		mcb.SetType('M');															// Further blocks follow
		DOS_DefragFreeMCB();
		return true;
		}

	DOS_MCB	mcb_next(segment+total);												// MCB will grow, try to join with following MCB
	DOS_DefragFreeMCB();
	if (mcb_type != 'Z' && mcb_next.GetPSPSeg() == MCB_FREE)
		{
		total += mcb_next.GetSize()+1;
		mcb_type = mcb_next.GetType();
		if (*blocks == total)
			{
			mcb.SetType(mcb_type);
			mcb.SetSize(*blocks);
			return true;
			}
		if (*blocks < total)
			{
			mcb_next.SetPt((Bit16u)(segment+*blocks));
			mcb_next.SetSize(total-*blocks-1);
			mcb_next.SetType(mcb_type);
			mcb_next.SetPSPSeg(MCB_FREE);
			mcb.SetSize(*blocks);
			mcb.SetType('M');
			return true;
			}
		}
	mcb.SetType(mcb_type);															// Not the required blocks available
	mcb.SetSize(total);
	*blocks = total;																// Return maximum
	DOS_SetError(DOSERR_INSUFFICIENT_MEMORY);
	return false;
	}

bool DOS_FreeMemory(Bit16u segment)
	{
// TODO Check if allowed to free this segment
	DOS_PSP callpsp(dos.psp());
	DOS_MCB mcb(segment-1);
	if (segment < DOS_MEM_START+1)
		E_Exit("Program tried to free MCB block %X", segment);
      
//	DOS_MCB mcb(segment-1);
	if (mcb.GetType() != 'M' && mcb.GetType() != 'Z')
		{
		DOS_SetError(DOSERR_MB_ADDRESS_INVALID);
		return false;
		}
	mcb.SetPSPSeg(MCB_FREE);
//	DOS_DefragFreeMCB();
	return true;
	}

void DOS_BuildUMBChain()
	{
	Bit16u first_umb_seg = DOS_PRIVATE_SEGMENT_END;
	Bit16u first_umb_size = 0xf000 - first_umb_seg - (TotEMSMB ? 0x1000 : 0);

	dos_infoblock.SetStartOfUMBChain(EndConvMem);
	dos_infoblock.SetUMBChainState(0);										// UMBs not linked yet

	DOS_MCB umb_mcb(first_umb_seg);
	umb_mcb.SetPSPSeg(MCB_FREE);											// Currently free
	umb_mcb.SetSize(first_umb_size-1);
	umb_mcb.SetType('Z');

	Bit16u mcb_segment = dos.firstMCB;										// Scan MCB-chain for last block
	DOS_MCB mcb(mcb_segment);
	while (mcb.GetType() != 'Z')
		{
		mcb_segment += mcb.GetSize()+1;
		mcb.SetPt(mcb_segment);
		}
	Bit16u cover_mcb = (Bit16u)(mcb_segment+mcb.GetSize()+1);				// A system MCB has to cover the space between the regular MCB-chain and the UMBs
	mcb.SetPt(cover_mcb);
	mcb.SetType('M');
	mcb.SetPSPSeg(0x0008);
	mcb.SetSize(first_umb_seg-cover_mcb-1);
	mcb.SetFileName("SC      ");
	}

bool DOS_LinkUMBsToMemChain(Bit16u linkstate)
	{
	Bit16u umb_start = dos_infoblock.GetStartOfUMBChain();					// Get start of UMB-chain
	if (umb_start != EndConvMem)
		return false;
	if ((linkstate&1) == (dos_infoblock.GetUMBChainState()&1))
		return true;
	
	Bit16u mcb_segment = dos.firstMCB;										// Scan MCB-chain for last block before UMB-chain
	Bit16u prev_mcb_segment = dos.firstMCB;
	DOS_MCB mcb(mcb_segment);
	while (mcb_segment != umb_start && mcb.GetType() != 'Z')
		{
		prev_mcb_segment = mcb_segment;
		mcb_segment += mcb.GetSize()+1;
		mcb.SetPt(mcb_segment);
		}
	DOS_MCB prev_mcb(prev_mcb_segment);

	switch (linkstate)
		{
	case 0:																	// Unlink
		if (prev_mcb.GetType() == 'M' && mcb_segment == umb_start)
			prev_mcb.SetType('Z');
		dos_infoblock.SetUMBChainState(0);
		break;
	case 1:																	// Link
		if (mcb.GetType() == 'Z')
			{
			mcb.SetType('M');
			dos_infoblock.SetUMBChainState(1);
			}
		break;
	default:
		return false;
		}
	return true;
	}

static Bitu DOS_default_handler(void)
	{
	return CBRET_NONE;
	}

void DOS_SetupMemory(bool low)
	{
	/* Let DOS claim a few bios interrupts. Makes vDos more compatible with 
	 * buggy games, which compare against the interrupt table. (probably a 
	 * broken linked list implementation) */
	Bitu cbID = CALLBACK_Allocate();												// DOS default int
	CALLBACK_Setup(cbID, &DOS_default_handler, CB_IRET, 0x708);
	RealSetVec(0x01, 0x700008);
	RealSetVec(0x03, 0x700008);
	RealSetVec(0x04, 0x700008);

	// Create a dummy device MCB with PSPSeg=0x0008
	DOS_MCB mcb_devicedummy((Bit16u)DOS_MEM_START);
	mcb_devicedummy.SetPSPSeg(MCB_DOS);												// Devices
	mcb_devicedummy.SetSize(1);
	mcb_devicedummy.SetType(0x4d);													// More blocks will follow
//	mcb_devicedummy.SetFileName("SD      ");

	Bit16u mcb_sizes = 2;

	int extra_size = low ? 0 : 4096-DOS_MEM_START-mcb_sizes-17-289;					// Disable first 64Kb if low not set in config (289 = size 4DOS)
	DOS_MCB tempmcb2((Bit16u)DOS_MEM_START+mcb_sizes);
//	tempmcb2.SetPSPSeg(0x40);
	tempmcb2.SetPSPSeg(MCB_DOS);
	tempmcb2.SetSize(16+extra_size);
	mcb_sizes += 17+extra_size;
	tempmcb2.SetType(0x4d);

	DOS_MCB mcb((Bit16u)DOS_MEM_START+mcb_sizes);
	mcb.SetPSPSeg(MCB_FREE);														// Free
	mcb.SetType(0x5a);																// Last Block
//	mcb.SetSize(0x9FFE - DOS_MEM_START - mcb_sizes);
	mcb.SetSize(EndConvMem - 1 - DOS_MEM_START - mcb_sizes);

	dos.firstMCB = DOS_MEM_START;
	dos_infoblock.SetFirstMCB(DOS_MEM_START);
	}
