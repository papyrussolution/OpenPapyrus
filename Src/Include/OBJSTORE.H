// OBJSTORE.H
// Copyright (c) Sobolev A. 1995

#ifndef __OBJSTORE_H
#define __OBJSTORE_H
#ifndef __SLIB_H
#	include <slib.h>
#endif
#ifndef __BLKIO_H
#	include <blkio.h>
#endif

#define DEFAULT_CACHE               4
#define FIRST_DYNAMIC_HANDLE  0x8000U

typedef unsigned int OBJHANDLE;
SPTRDEF(OBJHANDLE);

class ObjStore {
 public:
	SLAPI ObjStore();
	SLAPI ObjStore(Pchar fn, int mode, int cacheSize = DEFAULT_CACHE);
	SLAPI ~ObjStore();
	int SLAPI Good() {
		return !Error;
	}
	int SLAPI Open(Pchar fn, int mode, int cacheSize = DEFAULT_CACHE);
	int SLAPI Close();
	int SLAPI AllocHandle(POBJHANDLE);
	int SLAPI SelectHandle(OBJHANDLE, int add);
	long SLAPI Tell();
	int SLAPI Seek(long pos);
	int SLAPI Write(Pvoid buf, size_t size);
	int SLAPI Read(Pvoid, size_t);
	int SLAPI Clear();
	int SLAPI Flush();
	int SLAPI PrintStruct(Pchar fileName, int withData);	// debug
	static int SLAPI Create(Pchar fn, size_t blksize);
 protected:
	int SLAPI _Seek(long pos, int force);
	int SLAPI _Expand(long blk, Plong newblk, int force);
	int SLAPI _SetupHandle(OBJHANDLE hdl, long blk);
	void SLAPI _SavePosition();
	int Error;      // Constraction error flag
	PBLKFILE Ctrl;  // Control data of block file
	OBJHANDLE Curr; // Current handle
	long Block;     // First block of current handle
	long CurBlk;    // Current block of current handle
	//                 CurBlk == 0 if LPos is outsided from EOF
	long LPos;      // Current logical position
	long TblBlk;    // Location of Handle Table
	Pvoid Tbl;      // Handle Allocation Table
};

#endif // __OBJSTORE_H

