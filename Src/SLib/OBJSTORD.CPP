// OBJSTORD.CPP
// Copyright (c) Sobolev A. 1995, 2007
//
#include <errno.h>
#include <blkio_.h>
#include <blkio.h>
#include <slib.h>
#include <objstore.h>
#include <fstream.h>

#define DSIZE (Ctrl->blksize - 4)
#define PHTBL ((PHdlList)Tbl)
#define HTBL  (*(PHdlList)Tbl)

struct HdlEntry {
	OBJHANDLE Handle;
	long      Block;
};

typedef TSArray <HdlEntry> HdlList;
SPTRDEF(HdlList);

struct Header {
	long FreeBlk;
	size_t HdrSize;
	size_t BlkSize;
	long TblBlk;
};

int SLAPI ObjStore::PrintStruct(Pchar fileName, int /*withData*/)
{
	ofstream ofs(fileName);
	ofs << " *** ObjStore file structure *** " << endl;
	ofs << "     Header Size  : " << Ctrl->hdrsize << endl;
	ofs << "     Block Size   : " << Ctrl->blksize << endl;
	ofs << "     Buffer Count : " << Ctrl->bufcnt << endl << endl;

	ofs << "     Handles      : " << HTBL.getCount() << endl;
	long pos = lseek(Ctrl->fd.i, 0L, SEEK_CUR);
	for(int i = 0; i < HTBL.getCount(); i++) {
		ofs << "            " << HTBL.at(i).Handle << " >> ";
		long blk = HTBL.at(i).Block;
		long newblk;
		do {
			ofs << blk << ' ';
			bgetbf(Ctrl, blk, 0, &newblk, 4);
			blk = newblk;
		} while(blk);
		ofs << endl;
	}
	lseek(Ctrl->fd.i, pos, SEEK_SET);
	return 1;
}
