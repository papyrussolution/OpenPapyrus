#include "vDos.h"
#include "mem.h"
#include "dos_inc.h"
#include "callback.h"

static Bit16u dos_memseg = DOS_PRIVATE_SEGMENT;

Bit16u DOS_GetPrivatMemory(Bit16u pages)
	{
	Bit16u page = dos_memseg;
	dos_memseg += pages;
	if (dos_memseg > DOS_PRIVATE_SEGMENT_END)
		E_Exit("DOS: Not enough memory for internal tables");
	return page;
	}

bool DOS_FreePrivatMemory(Bit16u seg, Bit16u pages)
	{
	if (dos_memseg-pages != seg)													// We can only release the last requested memoryblock
		return false;
	dos_memseg = seg;
	return true;
	}

static Bitu DOS_CaseMapFunc(void)
	{
	return CBRET_NONE;
	}

static Bit8u country_info[0x22] = {
	0x00, 0x00,										// Date format
	0x24, 0x00, 0x00, 0x00, 0x00,					// Currencystring
	0x2c, 0x00,										// Thousands sep
	0x2e, 0x00,										// Decimal sep
	0x2d, 0x00,										// Date sep
	0x3a, 0x00,										// Time sep
	0x00,											// Currency form
	0x02,											// Digits after dec
	0x00,											// Time format
	0x00, 0x00, 0x00, 0x00,							// Casemap
	0x2c, 0x00,										// Data sep
	0x00, 0x00, 0x00, 0x00, 0x00,					// Reservered 5
	0x00, 0x00, 0x00, 0x00, 0x00					// Reservered 5
};

static Bit8u filenamechartable[24] = {
	0x16, 0x00, 0x01,
	0x00, 0xff,										// Allowed chars from .. to
	0x00,
	0x00, 0x20,										// Excluded chars from .. to
	0x02,
	0x0e,											// Number of illegal separators
	0x2e, 0x22, 0x2f, 0x5c, 0x5b, 0x5d, 0x3a, 0x7c, 0x3c, 0x3e, 0x2b, 0x3d, 0x3b, 0x2c
};

void DOS_SetupTables(void)
	{
	Bit16u seg;
	LinPt absAddr;

	dos.tables.mediaid = SegOff2dWord(DOS_GetPrivatMemory(4), 0);
	dos.tables.tempdta = SegOff2dWord(DOS_GetPrivatMemory(4), 0);
	dos.tables.tempdta_fcbdelete = SegOff2dWord(DOS_GetPrivatMemory(4), 0);
	Mem_rStosb(dWord2Ptr(dos.tables.mediaid), 0, DOS_DRIVES*2);
	// Create the DOS Info Block
	dos_infoblock.SetLocation(DOS_INFOBLOCK_SEG);	// c2woody
   
	DOS_SDA(DOS_SDA_SEG, 0).Init();					// Create SDA

	// Some weird files >20 detection routine
	// Possibly obselete when SFT is properly handled
	absAddr =  DOS_CONSTRING_SEG*16;
	Mem_aStosd(absAddr+0x0a, 0x204e4f43);
	Mem_aStosd(absAddr+0x1a, 0x204e4f43);
	Mem_aStosd(absAddr+0x2a, 0x204e4f43);

	// Create a CON device driver
	absAddr =  DOS_CONDRV_SEG*16;
 	Mem_aStosd(absAddr+0x00, 0xffffffff);											// Next ptr
 	Mem_aStosw(absAddr+0x04, 0x8013);												// Attributes
  	Mem_aStosd(absAddr+0x06, 0xffffffff);											// Strategy routine
  	Mem_aStosd(absAddr+0x0a, 0x204e4f43);											// Driver name
  	Mem_aStosd(absAddr+0x0e, 0x20202020);											// Driver name
	dos_infoblock.SetDeviceChainStart(SegOff2dWord(DOS_CONDRV_SEG, 0));
   
	// Create a fake Current Directory Structure
	absAddr =  DOS_CDS_SEG*16;
	Mem_aStosd(absAddr, 0x005c3a43);
	dos_infoblock.SetCurDirStruct(SegOff2dWord(DOS_CDS_SEG, 0));

	// Allocate DCBS DOUBLE BYTE CHARACTER SET LEAD-BYTE TABLE
	dos.tables.dbcs = SegOff2dWord(DOS_GetPrivatMemory(12), 0);
	Mem_Stosd(dWord2Ptr(dos.tables.dbcs), 0);	// empty table
	// FILENAME CHARACTER TABLE
	dos.tables.filenamechar = SegOff2dWord(DOS_GetPrivatMemory(2), 0);
	PhysPt p_addr = dWord2Ptr(dos.tables.filenamechar);
	for (int i = 0; i < sizeof(filenamechartable); i++)
		Mem_Stosb(p_addr++, filenamechartable[i]);
	// COLLATING SEQUENCE TABLE + UPCASE TABLE
	// 256 bytes for col table, 128 for upcase, 4 for number of entries
	dos.tables.collatingseq = SegOff2dWord(DOS_GetPrivatMemory(25), 0);
	Mem_Stosw(dWord2Ptr(dos.tables.collatingseq), 0x100);
	for (int i = 0; i < 256; i++)
		Mem_Stosb(dWord2Ptr(dos.tables.collatingseq)+i+2, i);
	dos.tables.upcase = dos.tables.collatingseq+258;
	Mem_Stosw(dWord2Ptr(dos.tables.upcase), 0x80);
	for (int i = 0; i < 128; i++)
		Mem_Stosb(dWord2Ptr(dos.tables.upcase)+i+2, 0x80+i);
 
	seg = DOS_GetPrivatMemory(4);													// Create a fake FCB SFT
	absAddr = seg<<4;
	Mem_aStosd(absAddr+0, 0xffffffff);												// Last File Table
	Mem_aStosw(absAddr+4, 100);														// File Table supports 100 files
	dos_infoblock.SetFCBTable(SegOff2dWord(seg, 0));

	dos.tables.dpb = DOS_GetPrivatMemory(2);										// Create a fake DPB
	absAddr = dos.tables.dpb<<4;
	for (int d = 0; d < 26; d++)
		Mem_aStosb(absAddr+d, d);

	// Create a fake disk buffer head
	seg = DOS_GetPrivatMemory(6);
	absAddr = seg<<4;
	for (int ct = 0; ct < 0x20; ct++)
		Mem_Stosb(absAddr+ct, 0);
	Mem_aStosw(absAddr+0x00, 0xffff);												// forward ptr
	Mem_aStosw(absAddr+0x02, 0xffff);												// backward ptr
	Mem_aStosb(absAddr+0x04, 0xff);													// not in use
	Mem_aStosb(absAddr+0x0a, 0x01);													// number of FATs
	Mem_aStosd(absAddr+0x0d, 0xffffffff);											// pointer to DPB
	dos_infoblock.SetDiskBufferHeadPt(SegOff2dWord(seg, 0));

	// Set buffers to a nice value
	dos_infoblock.SetBuffers(50, 50);

	// case map routine INT 0x21 0x38
	Bitu cbID = CALLBACK_Allocate();
	CALLBACK_Setup(cbID, DOS_CaseMapFunc, CB_RETF);									// DOS CaseMap
	// Add it to country structure
	*(Bit32u *)(country_info + 0x12) = CALLBACK_RealPointer(cbID);
	dos.tables.country = country_info;
	}
