// TVREZ.CPP
// Copyright (c) Sobolev A. 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2003, 2005, 2007, 2008, 2009, 2010, 2011, 2016, 2017, 2019, 2020, 2024, 2025
//
#include <slib-internal.h>
#pragma hdrstop

static constexpr size_t MaxRcStringLen = 126;

TVRez::TVRez(const char * pFileName, bool useIndex) : /*Stream(0),*/P_Index(0), HeaderType(hdrUnknown)
{
	open(pFileName, useIndex);
}

TVRez::~TVRez()
{
	delete P_Index;
	//SFile::ZClose(&Stream);
}

int TVRez::open(const char * pFileName, bool useIndex)
{
	int    ok = 1;
	error = 0;
	//FileName = pFileName;
	ZDELETE(P_Index);
	THROW(fileExists(pFileName));
	/*Stream = fopen(pFileName, "rb");
	if(!Stream) {
		SLS.SetError(SLERR_OPENFAULT, pFileName);
		CALLEXCEPT();
	}*/
	if(!F.Open(pFileName, SFile::mRead|SFile::mBinary)) {
		SLS.SetError(SLERR_OPENFAULT, pFileName);
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

bool TVRez::Seek(long offs, int origin/*SEEK_SET || SEEK_CUR || SEEK_END*/)
{
	return F.Seek(offs, origin);
}

int TVRez::getChar()
{
	//return fgetc(Stream);
	char   v = 0;
	return F.ReadV(&v, sizeof(v)) ? v : 0;
}

uint TVRez::getUINT()
{
	uint16 v = 0;
	//fread(&v, sizeof(v), 1, Stream);
	//return v;
	return F.ReadV(&v, sizeof(v)) ? static_cast<uint>(v) : 0;
}

long TVRez::getLONG() // @v12.2.3
{
	uint16 v = 0;
	return F.ReadV(&v, sizeof(v)) ? static_cast<long>(v) : 0;
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

SString & STDCALL TVRez::getString(SString & rBuf, int kind /*0 - 866, 1 - w_char, 2 - 1251*/)
{
	rBuf.Z();
	for(char c = !0; c != 0;) {
		if(kind == 1 && HeaderType == hdr32) {
			int16 wc;
			//fread(&wc, sizeof(wc), 1, Stream);
			if(F.ReadV(&wc, sizeof(wc)))
				c = wchar_to_1251(wc);
			else
				c = 0;
		}
		else {
			//c = fgetc(Stream);
			if(!F.ReadV(&c, sizeof(c)))
				c = 0;
		}
		/*if(feof(Stream)) {
			c = 0;
		}
		else*/if(c != 0)
			rBuf.CatChar(c);
	}
	if(kind == 2)
		rBuf.Transf(CTRANSF_OUTER_TO_INNER);
	return rBuf;
}

char * STDCALL TVRez::getString(char * buf, int kind /*0 - 866, 1 - w_char, 2 - 1251*/)
{
	size_t i = 0;
	if(buf) {
		for(char c = !0; c != 0;) {
			if(kind == 1 && HeaderType == hdr32) {
				int16 wc;
				//fread(&wc, sizeof(wc), 1, Stream);
				if(F.ReadV(&wc, sizeof(wc)))
					c = wchar_to_1251(wc);
				else
					c = 0;
			}
			else {
				//c = fgetc(Stream);
				if(!F.ReadV(&c, sizeof(c)))
					c = 0;
			}
			/*if(feof(Stream))
				c = 0;*/
			if(i < MaxRcStringLen)
				buf[i++] = c;
		}
		buf[i] = 0;
		if(kind == 2)
			SCharToOem(buf);
	}
	return buf;
}

TRect TVRez::getRect()
{
	TRect v;
	//fread(&v, sizeof(v), 1, Stream);
	//return v;
	return F.ReadV(&v, sizeof(v)) ? v : v.Z();
}

TYPEID TVRez::getType(int defaultLen)
{
	struct { 
		uint16 N;
		uint16 Len;
		uint16 Dec; 
	} tp;
	//fread(&tp, sizeof(tp), 1, Stream);
	//if(!feof(Stream) && !ferror(Stream) && !error)
	if(F.ReadV(&tp, sizeof(tp))) {
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
	}
	else {
		error = 1;
		return 0;
	}
}

long TVRez::getFormat(int defaultLen)
{
	struct {
		uint16 flags;
		uint16 len;
		uint16 dec;
	} fmt;
	//fread(&fmt, sizeof(fmt), 1, Stream);
	//if(!ferror(Stream) && !feof(Stream) && !error) {
	if(F.ReadV(&fmt, sizeof(fmt))) {
		if(fmt.len == 0)
			fmt.len = defaultLen;
		if(fmt.dec)
			return MKSFMTD(fmt.len, fmt.dec, fmt.flags);
		else
			return MKSFMT(fmt.len, fmt.flags);
	}
	else {
		error = 1;
		return 0;
	}
}

int TVRez::setHdrType()
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
	//fseek(Stream, 0L, SEEK_SET);
	//if(fread(buf, 8, 1, Stream) && !ferror(Stream) && !feof(Stream)) {
	if(F.Seek(0, SEEK_SET)) {
		if(F.ReadV(buf, 8)) {
			if(buf[0] == 0xFF) {
				HeaderType = hdr16;
				ok = 1;
			}
			else {
				//if(fread(buf + 8, 24, 1, Stream) && !ferror(Stream) && !feof(Stream) && memcmp(buf, res32prefix, 32) == 0) {
				if(F.ReadV(buf + 8, 24) && memcmp(buf, res32prefix, 32) == 0) {
					HeaderType = hdr32;
					ok = 1;
				}
			}
		}
	}
	return ok;
}

struct ResIdxItem { // @flat
	uint16 type;
	uint16 id;
	uint32 pos;
	uint32 sz;
};

static IMPL_CMPFUNC(ResIdxItem, _i1, _i2) { RET_CMPCASCADE2(static_cast<const ResIdxItem *>(_i1), static_cast<const ResIdxItem *>(_i2), type, id); }

int TVRez::buildIndex()
{
	if(P_Index)
		P_Index->freeAll();
	else {
		P_Index = new SVector(sizeof(ResIdxItem));
		if(P_Index == 0)
			return 0;
	}
	WResHeaderInfo hdr;
	for(ulong ofs = (HeaderType == hdr32) ? 32L : 0L; readHeader(ofs, &hdr, beginOfData); ofs = hdr.Next) {
		ResIdxItem item;
		MEMSZERO(item);
		if(hdr.IdKind == 0) {
			item.type = hdr.Type;
			item.id   = hdr.IntID;
			item.pos  = F.Tell(); //ftell(Stream);
			item.sz   = hdr.Size;
			if(!P_Index->insert(&item)) {
				ZDELETE(P_Index);
				return 0;
			}
		}
		else {
			item.id = 0;
			item.pos = F.Tell(); //ftell(Stream);
		}
	}
	P_Index->sort(PTR_CMPFUNC(ResIdxItem));
	return 1;
}

int TVRez::_readHeader16(ulong ofs, WResHeaderInfo * hdr, ResPosition rpos)
{
	uint16  w;
	uint32 dw;
	ulong  sz_ofs;
	char   c;
	//fseek(Stream, ofs, SEEK_SET);
	F.Seek(ofs, SEEK_SET);
	//if(fgetc(Stream) == 0xFF && !ferror(Stream) && !feof(Stream)) {
	if(F.ReadV(&c, sizeof(c)) && c == '\xff') {
		//fread(&w, sizeof(w), 1, Stream);
		F.ReadV(&w, sizeof(w));
		hdr->Type = w;
		//if(fgetc(Stream) == 0xFF) {
		if(F.ReadV(&c, sizeof(c)) && c == '\xff') {
			hdr->IdKind = 0;
			//fread(&w, sizeof(w), 1, Stream);
			F.ReadV(&w, sizeof(w));
			hdr->IntID = w;
		}
		else {
			hdr->IdKind = 1;
			//fseek(Stream, -1, SEEK_CUR);
			F.Seek(-1, SEEK_CUR);
			getString(hdr->StrID);
		}
		//fread(&w, sizeof(w), 1, Stream);
		F.ReadV(&w, sizeof(w));
		hdr->Flags = w;
		sz_ofs = F.Tell(); //ftell(Stream);
		//fread(&dw, sizeof(dw), 1, Stream);
		F.ReadV(&dw, sizeof(dw));
		hdr->Size = dw;
		hdr->Next = sz_ofs + sizeof(dw) + hdr->Size;
		switch(rpos) {
			case beginOfResource: F.Seek(ofs, SEEK_SET); /*fseek(Stream, ofs, SEEK_SET);*/break;
			case sizeField:       F.Seek(sz_ofs, SEEK_SET); /*fseek(Stream, sz_ofs, SEEK_SET);*/ break;
			case beginOfData:     break;
			case nextResource:    F.Seek(hdr->Next, SEEK_SET); /*fseek(Stream, hdr->Next, SEEK_SET);*/break;
			default:              break;
		}
		return 1;
	}
	else {
		//fseek(Stream, ofs, SEEK_SET);
		F.Seek(ofs, SEEK_SET);
	}
	return 0;
}

int TVRez::_readHeader32(ulong ofs, WResHeaderInfo * hdr, ResPosition rpos)
{
	int    ok = 1;
	uint16 w  = 0;
	uint32 dw = 0;
	ulong  hdr_sz, next_pos;
	//fseek(Stream, ofs, SEEK_SET);
	F.Seek(ofs, SEEK_SET);
	//THROW(fread(&dw, sizeof(dw), 1, Stream) && !ferror(Stream) && !feof(Stream));
	THROW(F.ReadV(&dw, sizeof(dw)));
	hdr->Size = dw;
	//THROW(fread(&dw, sizeof(dw), 1, Stream) && !ferror(Stream) && !feof(Stream));
	THROW(F.ReadV(&dw, sizeof(dw)));
	hdr_sz = dw;
	//THROW(fread(&w, sizeof(w), 1, Stream) && !ferror(Stream) && !feof(Stream));
	THROW(F.ReadV(&w, sizeof(w)));
	THROW(w == 0xFFFF);
	//THROW(fread(&w, sizeof(w), 1, Stream) && !ferror(Stream) && !feof(Stream));
	THROW(F.ReadV(&w, sizeof(w)));
	hdr->Type = w;
	//THROW(fread(&w, sizeof(w), 1, Stream) && !ferror(Stream) && !feof(Stream));
	THROW(F.ReadV(&w, sizeof(w)));
	if(w == 0xFFFF) {
		hdr->IdKind = 0;
		//THROW(fread(&w, sizeof(w), 1, Stream) && !ferror(Stream) && !feof(Stream));
		THROW(F.ReadV(&w, sizeof(w)));
		hdr->IntID = w;
	}
	else {
		hdr->IdKind = 1;
		//fseek(Stream, -2, SEEK_CUR);
		F.Seek(-2, SEEK_CUR);
		getString(hdr->StrID, 1);
		next_pos = F.Tell(); //ftell(Stream);
		if(next_pos % sizeof(dw) != 0)
			next_pos = (next_pos / sizeof(dw) + 1) * sizeof(dw);
		//fseek(Stream, next_pos, SEEK_SET);
		F.Seek(next_pos, SEEK_CUR);
	}
	//THROW(fread(&dw, sizeof(dw), 1, Stream) && !ferror(Stream) && !feof(Stream));
	THROW(F.ReadV(&dw, sizeof(dw)));
	//THROW(fread(&w, sizeof(w), 1, Stream) && !ferror(Stream) && !feof(Stream));
	THROW(F.ReadV(&w, sizeof(w)));
	hdr->Flags = w;
	next_pos = ofs + hdr_sz + hdr->Size;
	if(next_pos % sizeof(dw) != 0)
		next_pos = (next_pos / sizeof(dw) + 1) * sizeof(dw);
	hdr->Next = next_pos;
	switch(rpos) {
		case sizeField:
		case beginOfResource: F.Seek(ofs, SEEK_SET);/*fseek(Stream, ofs, SEEK_SET);*/break;
		case beginOfData:     F.Seek(ofs + hdr_sz, SEEK_SET);/*fseek(Stream, ofs + hdr_sz, SEEK_SET);*/break;
		case nextResource:    F.Seek(hdr->Next, SEEK_SET);/*fseek(Stream, hdr->Next, SEEK_SET);*/ break;
		default:              break;
	}
	CATCH
		memzero(hdr, sizeof(WResHeaderInfo));
		//fseek(Stream, ofs, SEEK_SET);
		F.Seek(ofs, SEEK_SET);
		ok = 0;
	ENDCATCH
	return ok;
}

int TVRez::readHeader(ulong ofs, WResHeaderInfo * hdr, ResPosition rpos)
{
	if(HeaderType == hdr32)
		return _readHeader32(ofs, hdr, rpos);
	else if(HeaderType == hdr16)
		return _readHeader16(ofs, hdr, rpos);
	else
		return 0;
}

int TVRez::findResource(uint id, uint rscType, long * pOffs, long * pSz)
{
	int    ok = 0;
	long   offs = 0L, sz = 0L;
	WResHeaderInfo hdr;
	//if(!error && !ferror(Stream) && !feof(Stream)) {
	if(!F.IsEof()) {
		if(P_Index) {
			uint   p = 0;
			ResIdxItem item;
			item.type = rscType;
			item.id = id;
			if(P_Index->bsearch(&item, &p, PTR_CMPFUNC(ResIdxItem))) {
				item = *static_cast<const ResIdxItem *>(P_Index->at(p));
				offs = item.pos;
				sz   = item.sz;
				//fseek(Stream, item.pos, SEEK_SET);
				F.Seek(item.pos, SEEK_SET);
				ok = 1;
			}
		}
		for(ulong ofs = 0; ok == 0 && readHeader(ofs, &hdr, beginOfData) > 0; ofs = hdr.Next) {
			if(hdr.Type == rscType && hdr.IdKind == 0 && hdr.IntID == id) {
				offs = F.Tell();/*ftell(Stream);*/
				sz   = hdr.Size;
				ok = 1;
			}
		}
	}
	ASSIGN_PTR(pOffs, offs);
	ASSIGN_PTR(pSz,   sz);
	return ok;
}

int TVRez::getSizeField(long * pSz)
{
	long   sz = 0;
	//fread(&sz, sizeof(sz), 1, Stream);
	int    ok = F.ReadV(&sz, sizeof(sz));
	ASSIGN_PTR(pSz, sz);
	return ok;
}

long TVRez::getStreamPos()
{
	//return ftell(Stream);
	return F.Tell();
}

int TVRez::enumResources(uint rscType, uint * pRscID, ulong * pDwPos)
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
	LongArray * pCtlList;
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

int TVRez::CheckDialogs(const char * pLogFileName)
{
	double debug_val = 0; // @debug
	char   msg_buf[512];
	LongArray id_list;
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
			LongArray ctl_list;
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

int TVRez::CheckDialogs(const char * pLogFileName)
{
	return -1;
}

#endif // } NDEBUG

