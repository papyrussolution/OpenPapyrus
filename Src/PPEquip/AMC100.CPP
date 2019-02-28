// AMC100.CPP
// Copyright (c) A.Sobolev 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2006, 2007, 2008, 2009, 2011, 2013, 2015, 2016
// @codepage windows-1251
// Интерфейс (синхронный) с ККМ АМС-100Ф
//
#include <pp.h>
#pragma hdrstop
#include <process.h>
//
// AMC-100
//
class SCS_AMC100 : public PPSyncCashSession {
public:
	SLAPI  SCS_AMC100(PPID n, char * name, char * port);
	SLAPI ~SCS_AMC100();
	virtual int SLAPI PrintCheck(CCheckPacket *, uint flags);
	virtual int SLAPI GetSummator(double * val);
	virtual int SLAPI AddSummator(double add);
	virtual int SLAPI EnableCashKeyb(int);
private:
	virtual int SLAPI InitChannel();
	int SLAPI PutLine(char * b, FILE *);
	int SLAPI MakeHeader(char * fn, CCheckPacket *, uint flags);
	int SLAPI MakeFooter(char * fn, CCheckPacket *, uint flags);
	int SLAPI MakeBody(char * fn, CCheckPacket *, uint flags);
	int SLAPI MakeExecFileName(char * fn, uint flags);
	int SLAPI Exec(char * exc, char * hdr, char * bdy, char * ftr, uint flags);
	int SLAPI ExecKLD(int param, int forceRedraw);
	int SLAPI OpenBox();
};

class CM_AMC100 : public PPCashMachine {
public:
	SLAPI  CM_AMC100(PPID cashID) : PPCashMachine(cashID) {}
	PPSyncCashSession * SLAPI SyncInterface();
};

PPSyncCashSession * SLAPI CM_AMC100::SyncInterface()
{
	PPSyncCashSession * cs = 0;
	if(IsValid()) {
		cs = new SCS_AMC100(NodeID, NodeRec.Name, NodeRec.Port);
		CALLPTRMEMB(cs, Init(NodeRec.Name, NodeRec.Port));
	}
	return cs;
}

REGISTER_CMT(AMC100,1,0);

SLAPI SCS_AMC100::SCS_AMC100(PPID n, char * name, char * port) : PPSyncCashSession(n, name, port)
{
}

SLAPI SCS_AMC100::~SCS_AMC100()
{
}

int SLAPI SCS_AMC100::InitChannel()
{
	return 1;
}

int SLAPI SCS_AMC100::PutLine(char * b, FILE * f)
{
	fputs(b, f);
	fputc('\n', f);
	return 1;
}

static FILE * opentemp(int firstLetter, char * buf)
{
	long wt = 0x7477L; // "wt"
	buf[0] = firstLetter;
	memset(buf+1, 'X', 6);
	buf[7] = 0;
	return fopen(mktemp(buf), (char *)&wt);
}

int SLAPI SCS_AMC100::MakeHeader(char * fn, CCheckPacket * /*pack*/, uint)
{
	int    ok = 1;
	PPObjPerson pobj;
	PPID main_org_id = 0;
	PPPersonPacket main_org_pack;
	FILE * f = opentemp('H', fn);
	char   buf[64], nam[64];
	SString temp_buf;
	if(GetMainOrgID(&main_org_id) > 0)
		if(pobj.GetPacket(main_org_id, &main_org_pack, 0) > 0) {
			STRNSCPY(nam, main_org_pack.Rec.Name);
			buf[0] = 'C';
			strfmt(nam, MKSFMT(17, ALIGN_CENTER | STRF_UPPER), buf+1);
			PutLine(buf, f);
			if(main_org_pack.GetRegNumber(PPREGT_TPID, temp_buf) > 0) {
				temp_buf.CopyTo(nam, sizeof(nam));
				buf[0] = 'C';
				buf[1] = 'И';
				buf[2] = 'Н';
				buf[3] = 'Н';
				buf[4] = ' ';
				strfmt(nam, MKSFMT(13, ALIGN_LEFT), buf+5);
				PutLine(buf, f);
			}
		}
	memset(buf + 1, ' ', 17);
	buf[18] = 0;
	PutLine(buf, f);
	SFile::ZClose(&f);
	return ok;
}

int SLAPI SCS_AMC100::MakeFooter(char * fn, CCheckPacket *, uint)
{
	int    ok = 1;
	FILE * f = opentemp('F', fn);
	char buf[20];
	buf[0] = 'C';
	memset(buf + 1, '-', 17);
	buf[18] = 0;
	PutLine(buf, f);
	buf[0] = 'W';
	memset(stpcpy(buf + 1, "‘ЏЂ‘€ЃЋ"), ' ', 10);
	buf[18] = 0;
	PutLine(buf, f);
	SFile::ZClose(&f);
	return ok;
}

int SLAPI SCS_AMC100::MakeBody(char * fn, CCheckPacket * pack, uint)
{
	int    ok = 1;
	FILE * f = opentemp('B', fn);
	double ipart, fract;
	char   buf[80], * p;
	char * fmt = "%0*.0lf";
	uint   i;
	SString goods_name;
	CCheckLineTbl::Rec cc_item;
	for(i = 0; pack->EnumLines(&i, &cc_item);) {
		double price = intmnytodbl(cc_item.Price);
		double dscnt = cc_item.Dscnt; // @v5.2.7 intmnytodbl(ti->Discount);
		p    = buf;
		GetGoodsName(cc_item.GoodsID, goods_name);
		p   += sstrlen(strfmt(goods_name, MKSFMT(17, ALIGN_LEFT | STRF_UPPER), p));
		*p++ = ' ';
		*p++ = '1'; // Отдел
		*p++ = ' ';
		fract = modf(price - dscnt, &ipart);
		sprintf(p, fmt, 7, ipart);
		p   += 7;
		*p++ = ' ';
		sprintf(p, fmt, 2, fract * 100.0);
		p   += 2;
		*p++ = ' ';
		sprintf(p, fmt, 3, fabs(cc_item.Quantity));
		p   += 3;
		*p   = 0;
		PutLine(buf, f);
	}
	SFile::ZClose(&f);
	return ok;
}

int SLAPI SCS_AMC100::MakeExecFileName(char * fn, uint)
{
	long   exe = 0x455845L; // "EXE"
	char * p = fn + sstrlen(fn);
	*p++ = Handle + '0';
	*p++ = '.';
	strcpy(p, (char *)&exe);
	return 1;
}

int SLAPI SCS_AMC100::Exec(char * exc, char * hdr, char * bdy, char * ftr, uint flags)
{
#ifndef __WIN32__
	TScreen::clearScreen();
#endif
	char   l[2];
	if(bdy)
		if(flags & PRNCHK_RETURN)
			l[0] = (flags & PRNCHK_REPRINT) ? 'W' : 'Y';
		else
			l[0] = (flags & PRNCHK_REPRINT) ? 'R' : 'C';
	else {
		l[0] = 'P';
		bdy = ftr;
		ftr = 0;
	}
	l[1] = 0;
	intptr_t ret = spawnlp(P_WAIT, exc, exc, l, hdr, bdy, ftr, 0);
	APPL->redraw();
	return ret ? PPSetError(PPERR_CASHDEVPRINT) : 1;
}

int SLAPI SCS_AMC100::PrintCheck(CCheckPacket * pack, uint flags)
{
	int ok = 1, empty = 0;
	char hdr[MAXPATH];
	char ftr[MAXPATH];
	char bdy[MAXPATH];
	char exc[MAXPATH];
	hdr[0] = ftr[0] = bdy[0] = 0;
	if(pack == 0) {
		pack = new CCheckPacket;
		empty = 1;
	}
	else if(flags & PRNCHK_OPENBOX) {
		OpenBox();
	}
	THROW(MakeHeader(hdr, pack, flags));
	THROW(MakeFooter(ftr, pack, flags));
	if(!empty) {
		THROW(MakeBody(bdy, pack, flags));
	}
	THROW(MakeExecFileName(strcpy(exc, "AMC_DOS"), flags));
	THROW(Exec(exc, hdr, (empty ? 0 : bdy), ftr, flags));
	CATCHZOK
	if(empty)
		delete pack;
	else
		SFile::Remove(bdy);
	SFile::Remove(hdr);
	SFile::Remove(ftr);
	return ok;
}

int SLAPI SCS_AMC100::GetSummator(double * val)
{
	*val = -1;
	return 1;
}

int SLAPI SCS_AMC100::AddSummator(double)
{
	return 1;
}

int SLAPI SCS_AMC100::ExecKLD(int param, int forceRedraw)
{
	char   exc[MAXPATH], l[2];
	MakeExecFileName(strcpy(exc, "OUT_KLD"), 0);
	l[0] = param;
	l[1] = 0;
	intptr_t r = spawnlp(P_WAIT, exc, exc, l, 0);
	if(forceRedraw)
		APPL->redraw();
	return (r == 0);
}

int SLAPI SCS_AMC100::OpenBox()
{
	return ExecKLD('B', 0);
}

int SLAPI SCS_AMC100::EnableCashKeyb(int enbl)
{
	return ExecKLD((enbl ? 'U' : 'L'), 1);
}
//
// AMC-200
//

#if 1 // {

class SCS_AMC200 : public PPSyncCashSession {
public:
	SLAPI  SCS_AMC200(PPID n, char * name, char * port);
	SLAPI ~SCS_AMC200();
	virtual int SLAPI PrintCheck(CCheckPacket *, uint flags);
	virtual int SLAPI GetSummator(double * val);
	virtual int SLAPI AddSummator(double add);
	virtual int SLAPI EnableCashKeyb(int);
private:
	virtual int SLAPI InitChannel();
	int SLAPI PutLine(char * b, FILE *);
	int SLAPI MakeHeader(char * fn, CCheckPacket *, uint flags);
	int SLAPI MakeFooter(char * fn, CCheckPacket *, uint flags);
	int SLAPI MakeBody(char * fn, CCheckPacket *, uint flags);
	int SLAPI MakeExecFileName(char * fn, uint flags);
	int SLAPI Exec(char * exc, int cmd, char * hdr, char * bdy, char * ftr, uint flags);
	int SLAPI OpenBox();
};

class CM_AMC200 : public PPCashMachine {
public:
	SLAPI CM_AMC200(PPID cashID) : PPCashMachine(cashID) {}
	PPSyncCashSession * SLAPI SyncInterface();
};

PPSyncCashSession * SLAPI CM_AMC200::SyncInterface()
{
	PPSyncCashSession * p_cs = 0;
	if(IsValid()) {
		p_cs = new SCS_AMC200(NodeID, NodeRec.Name, NodeRec.Port);
		CALLPTRMEMB(p_cs, Init(NodeRec.Name, NodeRec.Port));
	}
	return p_cs;
}

REGISTER_CMT(AMC200,1,0);

SLAPI SCS_AMC200::SCS_AMC200(PPID n, char * name, char * port) : PPSyncCashSession(n, name, port)
{
}

SLAPI SCS_AMC200::~SCS_AMC200()
{
}

int SLAPI SCS_AMC200::InitChannel()
{
	return 1;
}

int SLAPI SCS_AMC200::PutLine(char * b, FILE * f)
{
	fputs(b, f);
	fputc('\n', f);
	return 1;
}

int SLAPI SCS_AMC200::MakeHeader(char * fn, CCheckPacket * /*pack*/, uint)
{
	int ok = 1;
	PPObjPerson pobj;
	PPID mainOrgID = 0;
	PPPersonPacket mainOrgPack;
	FILE * f = opentemp('H', fn);
	char buf[64];
	char nam[64];

	buf[0] = '1';
	buf[1] = 0;
	PutLine(buf, f);

	if(GetMainOrgID(&mainOrgID) > 0)
		if(pobj.GetPacket(mainOrgID, &mainOrgPack, 0) > 0) {
			STRNSCPY(nam, mainOrgPack.Rec.Name);
			buf[0] = '1';
			strcpy(buf+1, nam);
			PutLine(buf, f);
		}
	buf[0] = '1';
	memset(buf + 1, ' ', 17);
	buf[18] = 0;
	PutLine(buf, f);
	SFile::ZClose(&f);
	return ok;
}

int SLAPI SCS_AMC200::MakeFooter(char * fn, CCheckPacket *, uint)
{
	int ok = 1;
	FILE * f = opentemp('F', fn);
	char buf[20];
	buf[0] = '1';
	buf[1] = 0;
	PutLine(buf, f);
	buf[0] = '2';
	strnzcpy(buf+1, "‘ЏЂ‘€ЃЋ", 9);
	PutLine(buf, f);
	SFile::ZClose(&f);
	return ok;
}

int SLAPI SCS_AMC200::MakeBody(char * fn, CCheckPacket * pack, uint)
{
	int    ok = 1;
	FILE * f = opentemp('B', fn);
	char   buf[80];
	const char * fmt = "%0*ld";
	SString goods_name;
	CCheckLineTbl::Rec cc_item;
	if(pack) {
		for(uint i = 0; pack->EnumLines(&i, &cc_item);) {
			long val;
			// Наименование товара
			GetGoodsName(cc_item.GoodsID, goods_name);
			PutLine(strfmt(goods_name, MKSFMT(16, ALIGN_LEFT), buf), f);
			// Цена
			val = (long)(R2(intmnytodbl(cc_item.Price)-cc_item.Dscnt) * 100.0);
			sprintf(buf, fmt, 8, val);
			PutLine(buf, f);
			// Количество
			val = (long)(R3(fabs(cc_item.Quantity)) * 1000.0);
			sprintf(buf, fmt, 9, val);
			PutLine(buf, f);
			// Номер отдела
			buf[0] = '1';
			buf[1] = 0;
			PutLine(buf, f);
			// Код товарной группы
			buf[0] = '0';
			buf[1] = 0;
			PutLine(buf, f);
		}
	}
	SFile::ZClose(&f);
	return ok;
}

int SLAPI SCS_AMC200::MakeExecFileName(char * fn, uint)
{
	long exe = 0x455845L; // "EXE"
	char * p = fn + sstrlen(fn);
	//*p++ = handle + '0';
	*p++ = '.';
	strcpy(p, (char *)&exe);
	return 1;
}

int SLAPI SCS_AMC200::Exec(char * exc, int cmd, char * hdr, char * bdy, char * ftr, uint /*flags*/)
{
#ifndef __WIN32__
	TScreen::clearScreen();
#endif
	char   l[2];
	char   port_str[32];
	itoa(Handle, port_str, 10);
	l[0] = cmd;
	l[1] = 0;
	intptr_t ret = spawnlp(P_WAIT, exc, exc, port_str, l, hdr, bdy, ftr, 0);
	APPL->redraw();
	return ret ? PPSetError(PPERR_CASHDEVPRINT) : 1;
}

int SLAPI SCS_AMC200::PrintCheck(CCheckPacket * pack, uint flags)
{
	int ok = 1, empty = 0;
	int cmd = 0;
	char hdr[MAXPATH];
	char ftr[MAXPATH];
	char bdy[MAXPATH];
	char exc[MAXPATH];
	hdr[0] = ftr[0] = bdy[0] = 0;
	if(pack == 0) {
		pack = new CCheckPacket;
		empty = 1;
	}
	else if(flags & PRNCHK_OPENBOX) {
		OpenBox();
	}
	THROW(MakeHeader(hdr, pack, flags));
	THROW(MakeFooter(ftr, pack, flags));
	THROW(MakeBody(bdy, empty ? 0 : pack, flags));
	THROW(MakeExecFileName(strcpy(exc, "A200_DOS"), flags));
	if(flags & PRNCHK_RETURN)
		cmd = 'Y';
	else
		cmd = (flags & PRNCHK_REPRINT) ? 'N' : 'C';
	THROW(Exec(exc, cmd, hdr, (empty ? 0 : bdy), ftr, flags));
	CATCHZOK
	if(empty)
		delete pack;
	SFile::Remove(bdy);
	SFile::Remove(hdr);
	SFile::Remove(ftr);
	return ok;
}

int SLAPI SCS_AMC200::GetSummator(double * val)
{
	*val = -1;
	return 1;
}

int SLAPI SCS_AMC200::AddSummator(double)
{
	return 1;
}

int SLAPI SCS_AMC200::OpenBox()
{
	return -1; //ExecKLD('B', 0);
}

int SLAPI SCS_AMC200::EnableCashKeyb(int enbl)
{
	char exc[MAXPATH];
	MakeExecFileName(strcpy(exc, "A200_DOS"), 0);
	return Exec(exc, (enbl ? 'U' : 'B'), 0, 0, 0, 0);
}

#endif // } 0
