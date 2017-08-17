// TVREZ.CPP
// Copyright (c) Sobolev A. 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2003, 2005, 2007, 2008, 2009, 2010, 2011, 2016, 2017
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop

SLAPI TVRez::TVRez(const char * fName, int useIndex)
{
	Stream = 0;
	Index = 0;
	HeaderType = hdrUnknown;
	open(fName, useIndex);
}

SLAPI TVRez::~TVRez()
{
	delete Index;
	SFile::ZClose(&Stream);
}

int SLAPI TVRez::open(const char * fName, int useIndex)
{
	int    ok = 1;
	error = 0;
	FileName = fName;
	ZDELETE(Index);
	THROW(fileExists(fName));
	Stream = fopen(fName, "rb");
	if(!Stream) {
		SLS.SetError(SLERR_OPENFAULT, fName);
		CALLEXCEPT();
	}
	THROW(setHdrType());
	if(useIndex)
		THROW(buildIndex());
	CATCH
		ok = 0;
		error = 1;
	ENDCATCH
	return ok;
}

int SLAPI TVRez::getChar()
{
	return fgetc(Stream);
}

uint SLAPI TVRez::getUINT()
{
	uint16 t;
	fread(&t, sizeof(t), 1, Stream);
	return t;
}

static char FASTCALL wchar_to_1251(int16 src)
{
	uchar c, buf[2];
	*PTR16(buf) = *PTR16(&src);
	if(buf[1] == 0x00)
		c = buf[0];
	else if(buf[1] == 0x04) {
		if(buf[0] >= 0x10 && buf[0] <= 0x4F)
			c = buf[0] + 0xB0;
		else if(buf[0] == 0x01)
			c = 0xA8;
		else if(buf[0] == 0x51)
			c = 0xB8;
		else
			c = ' ';
	}
	else
		c = ' ';
	return c;
}

SString & SLAPI TVRez::getString(SString & rBuf, int kind /*0 - 866, 1 - w_char, 2 - 1251*/)
{
	rBuf.Z();
	for(char c = !0; c != 0;) {
		if(kind == 1 && HeaderType == hdr32) {
			int16 wc;
			fread(&wc, sizeof(wc), 1, Stream);
			c = wchar_to_1251(wc);
		}
		else
			c = fgetc(Stream);
		if(feof(Stream))
			c = 0;
		else if(c != 0)
			rBuf.CatChar(c);
	}
	if(kind == 2)
		rBuf.ToOem();
	return rBuf;
}

char * SLAPI TVRez::getString(char * buf, int kind /*0 - 866, 1 - w_char, 2 - 1251*/)
{
	size_t i = 0;
	if(buf) {
		for(char c = !0; c != 0;) {
			if(kind == 1 && HeaderType == hdr32) {
				int16 wc;
				fread(&wc, sizeof(wc), 1, Stream);
				c = wchar_to_1251(wc);
			}
			else
				c = fgetc(Stream);
			if(feof(Stream))
				c = 0;
			if(i < 126)
				buf[i++] = c;
		}
		buf[i] = 0;
		if(kind == 2)
			SCharToOem(buf);
	}
	return buf;
}

TRect SLAPI TVRez::getRect()
{
	TRect r;
	fread(&r, sizeof(r), 1, Stream);
	return r;
}

TYPEID SLAPI TVRez::getType(int defaultLen)
{
	struct { uint16 N, Len, Dec; } tp;
	fread(&tp, sizeof(tp), 1, Stream);
	if(!feof(Stream) && !ferror(Stream) && !error)
		if(tp.N == S_MONEY || tp.N == S_DEC) {
			if(tp.Len == 0)
				tp.Len = 8;
			if(tp.Dec == 0)
				tp.Dec = 2;
			return MKSTYPED(tp.N, tp.Len, tp.Dec);
		}
		else {
			if((tp.N == S_ZSTRING || tp.N == S_CHAR) && tp.Len == 0)
				tp.Len = defaultLen;
			return MKSTYPE(tp.N, tp.Len);
		}
	error = 1;
	return 0;
}

long SLAPI TVRez::getFormat(int defaultLen)
{
	struct {
		uint16 flags;
		uint16 len;
		uint16 dec;
	} fmt;
	fread(&fmt, sizeof(fmt), 1, Stream);
	if(!ferror(Stream) && !feof(Stream) && !error) {
		if(fmt.len == 0)
			fmt.len = defaultLen;
		if(fmt.dec)
			return MKSFMTD(fmt.len, fmt.dec, fmt.flags);
		else
			return MKSFMT(fmt.len, fmt.flags);
	}
	error = 1;
	return 0;
}

int SLAPI TVRez::setHdrType()
{
	int    ok = 0;
	uchar  buf[32];
	uchar  res32prefix[32];
	memzero(res32prefix, sizeof(res32prefix));
	res32prefix[4]  = 0x20;
	res32prefix[8]  = 0xFF;
	res32prefix[9]  = 0xFF;
	res32prefix[12] = 0xFF;
	res32prefix[13] = 0xFF;
	fseek(Stream, 0L, SEEK_SET);
	if(fread(buf, 8, 1, Stream) && !ferror(Stream) && !feof(Stream))
		if(buf[0] == 0xFF) {
			HeaderType = hdr16;
			ok = 1;
		}
		else if(fread(buf + 8, 24, 1, Stream) && !ferror(Stream) && !feof(Stream)
			&& memcmp(buf, res32prefix, 32) == 0) {
			HeaderType = hdr32;
			ok = 1;
		}
	return ok;
}

struct ResIdxItem {
	uint16 type;
	uint16 id;
	uint32 pos;
	uint32 sz;
};

static IMPL_CMPFUNC(ResIdxItem, _i1, _i2)
{
	int    si = 0;
	CMPCASCADE2(si, (ResIdxItem*)_i1, (ResIdxItem*)_i2, type, id);
	return si;
}

int SLAPI TVRez::buildIndex()
{
	if(Index)
		Index->freeAll();
	else {
		Index = new SArray(sizeof(ResIdxItem));
		if(Index == 0)
			return 0;
	}
	WResHeaderInfo hdr;
	for(ulong ofs = (HeaderType == hdr32) ? 32L : 0L; readHeader(ofs, &hdr, beginOfData); ofs = hdr.Next) {
		ResIdxItem item;
		MEMSZERO(item);
		if(hdr.IdKind == 0) {
			item.type = hdr.Type;
			item.id   = hdr.IntID;
			item.pos  = ftell(Stream);
			item.sz   = hdr.Size;
			if(!Index->insert(&item)) {
				ZDELETE(Index);
				return 0;
			}
		}
		else {
			item.id = 0;
			item.pos = ftell(Stream);
		}
	}
	Index->sort(PTR_CMPFUNC(ResIdxItem));
	return 1;
}

int SLAPI TVRez::_readHeader16(ulong ofs, WResHeaderInfo * hdr, ResPosition rpos)
{
	uint16  w;
	uint32 dw;
	ulong  sz_ofs;
	fseek(Stream, ofs, SEEK_SET);
	if(fgetc(Stream) == 0xFF && !ferror(Stream) && !feof(Stream)) {
		fread(&w, sizeof(w), 1, Stream);
		hdr->Type = w;
		if(fgetc(Stream) == 0xFF) {
			hdr->IdKind = 0;
			fread(&w, sizeof(w), 1, Stream);
			hdr->IntID = w;
		}
		else {
			hdr->IdKind = 1;
			fseek(Stream, -1, SEEK_CUR);
			getString(hdr->StrID);
		}
		fread(&w, sizeof(w), 1, Stream);
		hdr->Flags = w;
		sz_ofs = ftell(Stream);
		fread(&dw, sizeof(dw), 1, Stream);
		hdr->Size = dw;
		hdr->Next = sz_ofs + sizeof(dw) + hdr->Size;
		switch(rpos) {
			case beginOfResource: fseek(Stream, ofs, SEEK_SET);       break;
			case sizeField:       fseek(Stream, sz_ofs, SEEK_SET);    break;
			case beginOfData:     break;
			case nextResource:    fseek(Stream, hdr->Next, SEEK_SET); break;
			default:              break;
		}
		return 1;
	}
	else
		fseek(Stream, ofs, SEEK_SET);
	return 0;
}

int SLAPI TVRez::_readHeader32(ulong ofs, WResHeaderInfo * hdr, ResPosition rpos)
{
	int    ok = 1;
	uint16 w  = 0;
	uint32 dw = 0;
	ulong  hdr_sz, next_pos;
	fseek(Stream, ofs, SEEK_SET);
	THROW(fread(&dw, sizeof(dw), 1, Stream) && !ferror(Stream) && !feof(Stream));
	hdr->Size = dw;
	THROW(fread(&dw, sizeof(dw), 1, Stream) && !ferror(Stream) && !feof(Stream));
	hdr_sz = dw;
	THROW(fread(&w, sizeof(w), 1, Stream) && !ferror(Stream) && !feof(Stream));
	THROW(w == 0xFFFF);
	THROW(fread(&w, sizeof(w), 1, Stream) && !ferror(Stream) && !feof(Stream));
	hdr->Type = w;
	THROW(fread(&w, sizeof(w), 1, Stream) && !ferror(Stream) && !feof(Stream));
	if(w == 0xFFFF) {
		hdr->IdKind = 0;
		THROW(fread(&w, sizeof(w), 1, Stream) && !ferror(Stream) && !feof(Stream));
		hdr->IntID = w;
	}
	else {
		hdr->IdKind = 1;
		fseek(Stream, -2, SEEK_CUR);
		getString(hdr->StrID, 1);
		next_pos = ftell(Stream);
		if(next_pos % sizeof(dw) != 0)
			next_pos = (next_pos / sizeof(dw) + 1) * sizeof(dw);
		fseek(Stream, next_pos, SEEK_SET);
	}
	THROW(fread(&dw, sizeof(dw), 1, Stream) && !ferror(Stream) && !feof(Stream));
	THROW(fread(&w, sizeof(w), 1, Stream) && !ferror(Stream) && !feof(Stream));
	hdr->Flags = w;
	next_pos = ofs + hdr_sz + hdr->Size;
	if(next_pos % sizeof(dw) != 0)
		next_pos = (next_pos / sizeof(dw) + 1) * sizeof(dw);
	hdr->Next = next_pos;
	switch(rpos) {
		case sizeField:
		case beginOfResource: fseek(Stream, ofs, SEEK_SET);          break;
		case beginOfData:     fseek(Stream, ofs + hdr_sz, SEEK_SET); break;
		case nextResource:    fseek(Stream, hdr->Next, SEEK_SET);    break;
		default:              break;
	}
	CATCH
		memzero(hdr, sizeof(WResHeaderInfo));
		fseek(Stream, ofs, SEEK_SET);
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI TVRez::readHeader(ulong ofs, WResHeaderInfo * hdr, ResPosition rpos)
{
	if(HeaderType == hdr32)
		return _readHeader32(ofs, hdr, rpos);
	else if(HeaderType == hdr16)
		return _readHeader16(ofs, hdr, rpos);
	else
		return 0;
}

int SLAPI TVRez::findResource(uint id, uint rscType, long * pOffs, long * pSz)
{
	int    ok = 0;
	long   offs = 0L, sz = 0L;
	WResHeaderInfo hdr;
	if(!error && !ferror(Stream) && !feof(Stream)) {
		if(Index) {
			uint   p = 0;
			ResIdxItem item;
			item.type = rscType;
			item.id = id;
			if(Index->bsearch(&item, &p, PTR_CMPFUNC(ResIdxItem))) {
				item = *(ResIdxItem*)Index->at(p);
				offs = item.pos;
				sz   = item.sz;
				fseek(Stream, item.pos, SEEK_SET);
				ok = 1;
			}
		}
		for(ulong ofs = 0; ok == 0 && readHeader(ofs, &hdr, beginOfData) > 0; ofs = hdr.Next)
			if(hdr.Type == rscType && hdr.IdKind == 0 && hdr.IntID == id) {
				offs = ftell(Stream);
				sz   = hdr.Size;
				ok = 1;
			}
	}
	ASSIGN_PTR(pOffs, offs);
	ASSIGN_PTR(pSz,   sz);
	return ok;
}

int SLAPI TVRez::getSizeField(long * pSz)
{
	long   sz;
	fread(&sz, sizeof(sz), 1, Stream);
	ASSIGN_PTR(pSz, sz);
	return 1;
}

long SLAPI TVRez::getStreamPos()
{
	return ftell(Stream);
}

int SLAPI TVRez::enumResources(uint rscType, uint * pRscID, ulong * pDwPos)
{
	WResHeaderInfo hdr;
	for(ulong ofs = *pDwPos; readHeader(ofs, &hdr, beginOfData/*nextResource*/); ofs = hdr.Next)
		if(hdr.Type == rscType) {
			ASSIGN_PTR(pRscID, hdr.IntID);
			*pDwPos = hdr.Next;
			return 1;
		}
	*pDwPos = 0;
	return -1;
}

#if 0 // {

static int _LogMsg(const char * pLogFileName, const char * pMsg)
{
	int    ok = 0;
	FILE * f_log = 0;
	if(pLogFileName && pMsg) {
		f_log = fopen(pLogFileName, "a");
		if(f_log) {
			fprintf(f_log, "%s\n", pMsg);
			ok = 1;
		}
	}
	SFile::ZClose(&f_log);
	return ok;
}

struct CheckDlgCtrlParam {
	const char * pLogFileName;
	IntArray * pCtlList;
};

static void CheckDlgCtrl(TView * pV, void * ptr)
{
	CheckDlgCtrlParam * p_param = (CheckDlgCtrlParam *)ptr;
	if(pV->Id) {
		int i_ctl_id = pV->Id;
		if(p_param->pCtlList->lsearch(&i_ctl_id, 0, PTR_CMPFUNC(int))) {
			char   msg_buf[512];
			int    owner_id = pV->owner ? pV->owner->Id : 0;
			sprintf(msg_buf, "Дублирование контрола %i диалога %i", i_ctl_id, owner_id);
			_LogMsg(p_param->pLogFileName, msg_buf);
		}
		else
			p_param->pCtlList->insert(&i_ctl_id);
	}
}

#include <crtdbg.h>

int SLAPI TVRez::CheckDialogs(const char * pLogFileName)
{
	double debug_val = 0; // @debug
	char   msg_buf[512];
	IntArray id_list;
	uint   res_id = 0;
	ulong  res_pos = 0;
	TDialog * dlg = 0;
	long   fil_dlg_count = 0;
	long   idx_dlg_count = 0;
	while(enumResources(TV_DIALOG, &res_id, &res_pos) > 0) {
		fil_dlg_count++;
		if(id_list.lsearch(&res_id, 0, PTR_CMPFUNC(int))) {
			sprintf(msg_buf, "Дублирование идентификатора диалога %u", res_id);
			_LogMsg(pLogFileName, msg_buf);
		}
		else
			id_list.insert(&res_id);
		delete dlg;
		dlg = new TDialog(TRect(0, 0, 10, 10), "TEST");
		if(TDialog::LoadDialog(this, res_id, dlg, 0)) {
			if(!_CrtCheckMemory())
				debug_val = 1;
			IntArray ctl_list;
			CheckDlgCtrlParam param;
			param.pLogFileName = pLogFileName;
			param.pCtlList = &ctl_list;
			dlg->forEach(CheckDlgCtrl, &param);
		}
		else {
			sprintf(msg_buf, "Ошибка загрузки диалога %u", res_id);
			_LogMsg(pLogFileName, msg_buf);
		}
	}
	if(Index) {
		for(uint i = 0; i < Index->getCount(); i++) {
			ResIdxItem * p_item = (ResIdxItem *)Index->at(i);
			if(p_item->type == TV_DIALOG) {
				idx_dlg_count++;
			}
		}
	}
	sprintf(msg_buf, "Диалогов: в файле = %ld, в индексе = %ld", fil_dlg_count, idx_dlg_count);
	_LogMsg(pLogFileName, msg_buf);
	strcpy(msg_buf, "\n");
	_LogMsg(pLogFileName, msg_buf);

	delete dlg;
	return 1;
}

#else

int SLAPI TVRez::CheckDialogs(const char * pLogFileName)
{
	return -1;
}

#endif // } NDEBUG

