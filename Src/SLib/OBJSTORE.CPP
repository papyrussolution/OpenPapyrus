// OBJSTORE.CPP
// Copyright (c) Sobolev A. 1995, 2007
//
#include <errno.h>
#include <blkio_.h>
#include <blkio.h>
#include <slib.h>
#include <objstore.h>

//#define TEST_OBJSTORE

#define DSIZE (Ctrl->blksize - 4)
#define PHTBL ((PHdlList)Tbl)
#define HTBL  (*(PHdlList)Tbl)

struct HdlEntry {
	OBJHANDLE Handle;
	long      Block;
};

IMPL_CMPFUNC(HdlEntry, i1, i2)
{
	return (int)(*((POBJHANDLE)i1) - *((POBJHANDLE)i2));
}

typedef TSArray <HdlEntry> HdlList;
SPTRDEF(HdlList);

struct Header {
	long FreeBlk;
	size_t HdrSize;
	size_t BlkSize;
	long TblBlk;
};

SLAPI ObjStore::ObjStore()
{
	Error = 0;
	Ctrl = 0;
	Curr = 0;
	Block = 0;
	CurBlk = 0;
	LPos = 0;
	TblBlk = 0;
	PHTBL = 0;
}

SLAPI ObjStore::ObjStore(Pchar fn, int mode, int cacheSize)
{
	Error = !Open(fn, mode, cacheSize);
}

SLAPI ObjStore::~ObjStore()
{
	Close();
}

int SLAPI ObjStore::Create(Pchar fn, size_t blksize)
{
	EXCEPTVAR(errno);
	Header hdr;
	Pvoid ptr;
	PBLKFILE Ctrl;
	int ok = 1;
	THROW(Ctrl = bopen(fn, BF_CRTR, blksize, blksize, 0));
	hdr.FreeBlk = 0;
	hdr.HdrSize = blksize;
	hdr.BlkSize = blksize;
	hdr.TblBlk  = 0;
	THROW_V(ptr = calloc(blksize, 1), ENOMEM);
	THROW(bputh(Ctrl, memcpy(ptr, &hdr, sizeof(hdr))));
	THROW(bclose(Ctrl));
	CATCH
		remove(fn);
		ok = 0;
	ENDCATCH
	if(ptr)
		free(ptr);
	return ok;
}

#pragma warn -pia

int SLAPI ObjStore::Open(Pchar fn, int /*mode*/, int cacheSize)
{
	Header hdr;
	Error = 0;
	Curr = 0;
	Block = 0;
	CurBlk = 0;
	LPos = 0;
	TblBlk = 0;
	PHTBL = 0;
	Ctrl = bopen(fn, BF_RDWR, 0, 0, cacheSize);
	if(Ctrl) {
		bgethf(Ctrl, 0, &hdr, sizeof(hdr));
		PHTBL = new HdlList;
		if(PHTBL) {
			_SetupHandle(0, hdr.TblBlk);
			TblBlk = hdr.TblBlk;
			if(TblBlk) {
				if(HTBL.read(*this))
					return 1;
			}
			else if(HTBL.write(*this)) {
				TblBlk = Block;
				return 1;
			}
		}
	}
	return 0;
}

#pragma warn +pia

int SLAPI ObjStore::Close()
{
	Flush();
	if(PHTBL)
		delete PHTBL;
	PHTBL = 0;
	return (Ctrl && (Ctrl->flags & BIOOPEN)) ? bclose(Ctrl) : 1;
}

int SLAPI ObjStore::Flush()
{
	Header hdr;
	if(PHTBL) {
		THROW(bgethf(Ctrl, 0, &hdr, sizeof(hdr)));
		_SetupHandle(0, hdr.TblBlk = TblBlk);
		THROW(bputhf(Ctrl, 0, &hdr, sizeof(hdr)));
		THROW(HTBL.write(*this));
	}
	THROW(bflush(Ctrl));
	CATCH
		return 0;
	ENDCATCH
	return 1;
}

int SLAPI ObjStore::_SetupHandle(OBJHANDLE hdl, long blk)
{
	_SavePosition();
	Curr = hdl;
	Block = CurBlk = blk;
	return _Seek(0, 0);
}

void SLAPI ObjStore::_SavePosition()
{
	uint pos;
	int found;
	if(Curr && Block) {
		found = HTBL.bsearch(&Curr, &pos, PTR_CMPFUNC(HdlEntry));
		CHECK(found != 0);
		HTBL.at(pos).Block = Block;
	}
}

int SLAPI ObjStore::_Expand(long blk, Plong newblk, int force)
{
	EXCEPTVAR(errno);
	Pvoid temp = 0;
	int res = 1;
	if(*newblk == 0)
		if(force) {
				THROW(bflpop(Ctrl, (Pulong)newblk));
			temp = calloc(1, Ctrl->blksize);
			THROW_V(temp != 0, ENOMEM);
			THROW(bputbf(Ctrl, *newblk, 0, temp, Ctrl->blksize));
			if(blk)
					THROW(bputbf(Ctrl, blk, 0, newblk, sizeof(long)));
		}
		else
			res = 0;
   	CurBlk = *newblk;
	CATCH
		res = -1;
	ENDCATCH
	if(temp)
		free(temp);
	return res;
}

int SLAPI ObjStore::_Seek(long pos, int force)
{
	//EXCEPTVAR(errno);
	long p = pos;
	long blk = Block;
	long newblk = 0;
	int  expand = 1;
	int  ok = 1;
	long savePos = lseek(Ctrl->fd.i, 0L, SEEK_CUR);
	while(expand && (p >= DSIZE || !blk)) {
		if(blk)
			THROW(bgetbf(Ctrl, blk, 0, &newblk, sizeof(newblk)));
		THROW((expand = _Expand(blk, &newblk, force)) != -1);
		if(expand) {
			if(Block == 0)
				Block = newblk;
		   	blk = newblk;
			p -= DSIZE;
		}
   	}
	LPos = pos;
	if(expand)
		CurBlk = blk;
	CATCH
		lseek(Ctrl->fd.i, savePos, SEEK_SET);
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI ObjStore::Seek(long pos)
{
	return _Seek(pos, 0);
}

long SLAPI ObjStore::Tell()
{
	return LPos;
}

int SLAPI ObjStore::Write(Pvoid buf, size_t size)
{
	//EXCEPTVAR(errno);
	int ok = 1;
	size_t write_size;
	size_t s = size;
	long newblk = 0;
	Pchar b = (Pchar)buf;
	if(size) {
		if(CurBlk == 0)
			THROW(_Seek(LPos, 1));
		do {
			THROW(bgetbf(Ctrl, CurBlk, 0, &newblk, sizeof(newblk)));
			write_size = MIN(s, DSIZE - LPos % DSIZE);
			THROW(bputbf(Ctrl, CurBlk, LPos % DSIZE + 4, b, write_size));
			s -= write_size;
			LPos += write_size;
			b += write_size;
			if(s > 0) {
				THROW(_Expand(CurBlk, &newblk, 1) != -1);
			}
		} while(s > 0);
	}
	CATCH
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI ObjStore::Read(Pvoid buf, size_t size)
{
	EXCEPTVAR(errno);
	int ok = 1;
	size_t read_size;
	size_t s = size;
	long newblk = 0;
	Pchar b = (Pchar)buf;
	THROW_V(CurBlk != 0, BEEOF);
	do {
		THROW(bgetbf(Ctrl, CurBlk, 0, &newblk, sizeof(newblk)));
		read_size = MIN(s, DSIZE - LPos % DSIZE);
		THROW(bgetbf(Ctrl, CurBlk, LPos % DSIZE + 4, b, read_size));
		s -= read_size;
		LPos += read_size;
		b += read_size;
		if(s > 0) {
			THROW_V(newblk != 0, BEEOF);
			CurBlk = newblk;
		}
	} while(s > 0);
	CATCH
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI ObjStore::Clear()
{
	//EXCEPTVAR(errno);
	int ok = 1;
	int found;
	uint pos;
	long blk = Block;
	long newblk = 0;
	Header hdr;
	while(blk) {
		THROW(bgetbf(Ctrl, blk, 0, &newblk, sizeof(newblk)));
		THROW(bflpush(Ctrl, (const Pulong)&blk));
		blk = newblk;
	}
	found = HTBL.bsearch(&Curr, &pos, PTR_CMPFUNC(HdlEntry));
	CHECK(found != 0);
	HTBL.atFree(pos);
	Block = 0; // _SetupHandle must not call _SavePosition
	THROW(bgethf(Ctrl, 0, &hdr, sizeof(hdr)));
	THROW(_SetupHandle(0, hdr.TblBlk));
	CATCH
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI ObjStore::SelectHandle(OBJHANDLE hdl, int add)
{
	uint   pos;
	assert(PHTBL != 0);
	if(hdl == Curr)
		return _SetupHandle(hdl, Block);
	if(HTBL.bsearch(&hdl, &pos, PTR_CMPFUNC(HdlEntry)))
		return _SetupHandle(hdl, HTBL.at(pos).Block);
	else if(add) {
		HdlEntry entry;
		entry.Handle = hdl;
		entry.Block = 0;
		HTBL.ordInsert(&entry, &pos, PTR_CMPFUNC(HdlEntry));
		return _SetupHandle(hdl, 0);
	}
	return 0;
}

int SLAPI ObjStore::AllocHandle(POBJHANDLE hdl)
{
	assert(PHTBL != 0);
	OBJHANDLE h = HTBL.at(HTBL.getCount() - 1).Handle;
	if(h >= FIRST_DYNAMIC_HANDLE)
		*hdl = h + 1;
	else
		*hdl = FIRST_DYNAMIC_HANDLE;
	return 1;
}

#ifdef TEST_OBJSTORE

#include <iostream.h>
#include <stdio.h>

#define NUM_FILES 4

char * files[NUM_FILES] = {
	"1.ico", "2.ico", "3.ico", "4.ico"
};

int main()
{
	int i;
	size_t fl;
	Pvoid b = 0;
	OBJHANDLE hdl;
	FILE * f = 0;
	if(ObjStore::Create("ico.os", 512) == 0) {
		cout << "Error creating ObjStore file ico.os" << endl;
		return -1;
	}
	ObjStore os("ico.os", BF_RDWR);
	if(!os.Good()) {
		cout << "Error opening ObjStore file ico.os" << endl;
		return -1;
	}
	for(i = 0; i < NUM_FILES; i++) {
		f = fopen(files[i], "rb");
		if(f == 0) {
			cout << "Error opening file " << files[i] << endl;
			return -1;
		}
		fseek(f, 0, SEEK_END);
		fl = (size_t)ftell(f);
		rewind(f);
		b = realloc(b, fl+2);
		if(b == 0) {
			cout << "Not enough memory to allocate buffer" << endl;
			return -1;
		}
		*(Puint)b = fl;
		fread(((Pchar)b)+2, (size_t)fl, 1, f);
		if(!os.SelectHandle(i+1, 1)) {
			cout << "Error allocating ObjStor handle" << endl;
			return -1;
		}
		if(!os.Write(b, fl+2)) {
			cout << "Error writing object" << endl;
			return -1;
		}
		fclose(f);
	}

	if(os.SelectHandle(2, 0)) {
		os.Clear();
		os.AllocHandle(&hdl);
		os.SelectHandle(hdl, 1);
		f = fopen("5.bmp", "rb");
		fseek(f, 0L, SEEK_END);
		fl = ftell(f);
		rewind(f);
		b = realloc(b, fl+2);
		*(size_t*)b = fl;
		fread(((Pchar)b)+2, fl, 1, f);
		os.Write(b, fl+2);
		fclose(f);

		f = fopen("15.bmp", "w+b");
		os.SelectHandle(hdl, 0);
		os.Read(&fl, 2);
		os.Read(b, fl);
		fwrite(b, fl, 1, f);
		fclose(f);
	}

	for(i = 0; i < NUM_FILES; i++) {
		char fn[64];
		strcat(itoa(10+i+1, fn, 10), ".ico");
		f = fopen(fn, "w+b");
		if(f == 0) {
			cout << "Error opening file " << fn << endl;
			return -1;
		}
		if(!os.SelectHandle(i+1, 0)) {
			cout << "Error searching ObjStore handle " << i+1 << endl;
			continue;
		}
		if(!os.Read(&fl, sizeof(fl))) {
			cout << "Error reading object" << endl;
			return -1;
		}
		b = realloc(b, fl);
		if(b == 0) {
			cout << "Not enough memory to allocate buffer" << endl;
			return -1;
		}
		if(!os.Read(b, fl)) {
			cout << "Error reading object" << endl;
			return -1;
		}
		fwrite(b, fl, 1, f);
		fclose(f);
	}

	os.PrintStruct("debug.txt", 0);
	os.Close();
	os.Open("ico.os", BF_RDWR);
	os.PrintStruct("debug1.txt", 0);

	cout << "OK !!!" << endl;
	return 0;
}

#endif


