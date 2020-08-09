// SPRINTER.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998-2001, 2006, 2009, 2010, 2011, 2012, 2015, 2016, 2017, 2019, 2020
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
#include <report.h>

#define max MAX // @v9.8.11
#define min MIN // @v9.8.11
#include <gdiplus.h>
using namespace Gdiplus;

int (*SPrinter::HandlePrintError)(int) = 0;

#ifdef __WIN32__ // {
//
// Win32 stubs
//
SLAPI SPrinter::SPrinter()              {}
SLAPI SPrinter::~SPrinter()             {}
int   SLAPI SPrinter::printLine(const char *, size_t) { return 1; }
int   SLAPI SPrinter::startPage()       { return 1; }
int   SLAPI SPrinter::endPage()         { return 1; }
//
// @todo перенести функционал в SPrinter
//
SPrinting::SPrinting(HWND screenHwnd) : Top(2), PrinterDc(0), OldFont(0), PageStarted(0), DocStarted(0), OldColor(-1)
{
	HDC hdc = ::GetDC(screenHwnd);
	ScreenDpiX = ::GetDeviceCaps(hdc, LOGPIXELSX);
	ScreenDpiY = ::GetDeviceCaps(hdc, LOGPIXELSY);
	ReleaseDC(screenHwnd, hdc);
}

SPrinting::~SPrinting()
{
	if(PrinterDc) {
		if(PageStarted)
			EndPage(PrinterDc);
		if(DocStarted)
			EndDoc(PrinterDc);
		SetTextColor(PrinterDc, OldColor);
		if(OldFont)
			SelectObject(PrinterDc, OldFont);
		DeleteDC(PrinterDc);
	}
}

int SPrinting::Init(const char * pPort)
{
	int    ok = 1;
	SString port(pPort);
	if(!port.NotEmpty()) {
		TSVector <SPrinting::PrnInfo> prn_list; // @v9.8.4 TSArray-->TSVector
		SPrinting::GetListOfPrinters(&prn_list);
		for(uint j = 0; !port.NotEmpty() && j < prn_list.getCount(); j++) {
			if(prn_list.at(j).Flags & SPrinting::PrnInfo::fDefault)
				port = prn_list.at(j).PrinterName;
		}
	}
	if(port.NotEmpty()) {
		PrinterDc = CreateDC(_T("WINSPOOL\0"), SUcSwitch(port), 0, 0); // @unicodeproblem
		if(!PrinterDc)
			ok = SLS.SetError(SLERR_WINDOWS);
	}
	else
		ok = SLS.SetError(SLERR_NODEFPRINTER);
	return ok;
}

int SPrinting::StartDocument()
{
	int    ok = 1;
	if(!DocStarted) {
		DOCINFO di;
		COLORREF old_color = SetTextColor(PrinterDc, GetColorRef(SClrBlack));
		//
		// Set printer font
		//
		SetBkMode(PrinterDc, TRANSPARENT);
		MEMSZERO(di);
		di.cbSize = sizeof(DOCINFO);
		di.lpszDocName  = _T("Document");
		di.lpszOutput   = NULL;
		di.lpszDatatype = NULL;
		di.fwType = 0;
		THROW(StartDoc(PrinterDc, &di) != SP_ERROR); // @unicodeproblem
		DocStarted = 1;
	}
	CATCHZOK
	return ok;
}

int SPrinting::PrintImage(const char * pImgPath)
{
	int    ok = 0;
	if(PrinterDc && !isempty(pImgPath)) {
		long   height = 0;
		Graphics graphics(PrinterDc);
		RECT   coord;
		SImage img;
		MEMSZERO(coord);
		//SetMapMode(PrinterDc, MM_LOMETRIC);
		int w = 0, h = 0;
		int scale_x = 0, scale_y = 0;
		img.Load(pImgPath);
		{
			w = ::GetDeviceCaps(PrinterDc, HORZRES);
			h = ::GetDeviceCaps(PrinterDc, VERTRES);
			scale_x = ::GetDeviceCaps(PrinterDc, LOGPIXELSX) / ScreenDpiX;
			scale_y = ::GetDeviceCaps(PrinterDc, LOGPIXELSY) / ScreenDpiY;
			w /= scale_x;
			h /= scale_y;
		}
		SETIFZ(coord.right,  (long)img.GetWidth());
		SETIFZ(coord.bottom, (long)img.GetHeight());
		SETIFZ(coord.left, 2);
		SETIFZ(coord.top,  2);
		SETMIN(coord.right, (w - coord.left));
		SETMIN(coord.bottom, (h - coord.top));
		height = coord.bottom;
		if(!PageStarted || (Top + labs(height)) > (h - 4)) {
			StartDocument();
			if(PageStarted)
				EndPage(PrinterDc);
			StartPage(PrinterDc);
			PageStarted = 1;
			Top  = 2;
		}
		coord.top  = Top;
		img.Draw(PrinterDc, &coord, 0, 0);
		Top += labs(height);
		ok = 1;
	}
	return ok;
}

/*static*/int SLAPI SPrinting::GetListOfPrinters(TSVector <PrnInfo> * pInfoList) // @v9.8.4 TSArray-->TSVector
{
	int    ok = 0;
	STempBuffer info_buf(32*1024);
	TCHAR  def_prn_name[256];
	DWORD  def_prn_name_size = SIZEOFARRAY(def_prn_name);
	DWORD  wr_bytes = 0;
	DWORD  count = 0;
	if(pInfoList)
		pInfoList->freeAll();
	GetDefaultPrinter(def_prn_name, &def_prn_name_size); // @unicodeproblem
	BOOL   r = EnumPrinters(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, 0, 2, static_cast<LPBYTE>(info_buf.vptr()), info_buf.GetSize(), &wr_bytes, &count);
	if(!r && wr_bytes > info_buf.GetSize()) {
		info_buf.Alloc(wr_bytes);
		wr_bytes = 0;
		count = 0;
		r = EnumPrinters(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, 0, 2, static_cast<LPBYTE>(info_buf.vptr()), info_buf.GetSize(), &wr_bytes, &count);
	}
	if(r) {
		ok = -1;
		const PRINTER_INFO_2 * info = static_cast<const PRINTER_INFO_2 *>(info_buf.vcptr());
		for(uint i = 0; i < count; i++) {
			if(pInfoList) {
				PrnInfo item;
				MEMSZERO(item);
				STRNSCPY(item.ServerName, SUcSwitch(info[i].pServerName)); // @unicodeproblem
				STRNSCPY(item.PrinterName, SUcSwitch(info[i].pPrinterName)); // @unicodeproblem
				STRNSCPY(item.ShareName, SUcSwitch(info[i].pShareName)); // @unicodeproblem
				SETFLAG(item.Flags, PrnInfo::fLocal, info[i].Attributes & PRINTER_ATTRIBUTE_LOCAL);
				SETFLAG(item.Flags, PrnInfo::fNetwork, info[i].Attributes & PRINTER_ATTRIBUTE_NETWORK);
				if(stricmp(item.PrinterName, SUcSwitch(def_prn_name)) == 0)
					item.Flags |= PrnInfo::fDefault;
				pInfoList->insert(&item);
			}
			ok = 1;
		}
	}
	return ok;
}

#else // } __WIN32__ {

#define ESC '\x1b'

class EpsonCmdSet : public SPrnCmdSet {
public:
	SLAPI EpsonCmdSet();
	virtual int SLAPI InitPrinter(char *);
	virtual int SLAPI ResetPrinter(char *);
	virtual int SLAPI SetPageLength(int, char *);
	virtual int SLAPI SetMargin(int, int, char *);
	virtual int SLAPI SetQuality(int, char *);
	virtual int SLAPI SetCPI(int, char *);
	virtual int SLAPI SetFontStyle(int, int, char *);
	//virtual int SLAPI SetLinesPerInch(int, char *);
};

class PCLCmdSet : public SPrnCmdSet {
public:
	SLAPI PCLCmdSet();
	virtual int SLAPI InitPrinter(char *);
	virtual int SLAPI ResetPrinter(char *);
	virtual int SLAPI SetPageLength(int, char *);
	virtual int SLAPI SetOrientation(int, char *);
	virtual int SLAPI SetMargin(int, int, char *);
	virtual int SLAPI SetCPI(int, char *);
	virtual int SLAPI SetFontStyle(int, int, char *);
	virtual int SLAPI SetLinesPerInch(int, char *);
};

SLAPI SPrnCmdSet::SPrnCmdSet()
{
}

SLAPI SPrnCmdSet::~SPrnCmdSet()
{
}

/*static*/SPrnCmdSet * SLAPI SPrnCmdSet::CreateInstance(long id, long /* extra */)
{
	if(id == SPCMDSET_PCL)
		return new PCLCmdSet;
	else if(id == SPCMDSET_EPSON)
		return new EpsonCmdSet;
	else
		return new SPrnCmdSet;
}
//
// EpsonCmdSet
//
SLAPI EpsonCmdSet::EpsonCmdSet() : SPrnCmdSet()
{
}

int SLAPI EpsonCmdSet::InitPrinter(char * b)
{
	b[0] = ESC;
	b[1] = '@';
	return 2;
}

int SLAPI EpsonCmdSet::ResetPrinter(char * b)
{
	return InitPrinter(b);
}

int SLAPI EpsonCmdSet::SetPageLength(int l, char * b)
{
	if(checkirange(l, 1, 127)) {
		b[0] = ESC;
		b[1] = 'C';
		b[2] = (char)l;
		return 3;
	}
	return 0;
}

int SLAPI EpsonCmdSet::SetMargin(int what, int m, char * b)
{
	if(what == SPMRG_LEFT) {
		b[0] = ESC;
		b[1] = 'l';
		b[2] = (char) m;
		return 3;
	}
	return 0;
}

int SLAPI EpsonCmdSet::SetQuality(int q, char * b)
{
   	b[0] = ESC;
	b[1] = 'x';
	b[2] = (q == SPQLTY_DRAFT) ? 0 : 1;
	return 3;
}

int SLAPI EpsonCmdSet::SetCPI(int cpi, char * b)
{
	int i = 0;
	if(cpi == SPCPI_10) {
		b[i++] = 0x12; // Condenced off
		b[i++] = ESC;
		b[i++] = 'P';
	}
	else if(cpi == SPCPI_12) {
		b[i++] = 0x12; // Condenced off
		b[i++] = ESC;
		b[i++] = 'M';
	}
	else if(cpi == SPCPI_COND) {
		b[i++] = ESC; //
		b[i++] = 'M'; // CPI 12 + condenced
		b[i++] = ESC;
		b[i++] = 0x0F;
	}
	return i;
}

int SLAPI EpsonCmdSet::SetFontStyle(int s, int on_off, char * b)
{
	switch(s) {
		case SPFS_BOLD:
			b[0] = ESC;
			b[1] = on_off ? 'E' : 'F';
			return 2;
		case SPFS_ITALIC:
			b[0] = ESC;
			b[1] = on_off ? '4' : '5';
			return 2;
		case SPFS_UNDERLINE:
			b[0] = ESC;
			b[1] = '-';
			b[2] = on_off ? 1 : 0;
			return 3;
	}
	return 0;
}
//
// PCLCmdSet
//
SLAPI PCLCmdSet::PCLCmdSet() : SPrnCmdSet()
{
}

int SLAPI PCLCmdSet::InitPrinter(char * b)
{
	int i = 0;
	b[i++] = ESC;
	b[i++] = 'E';
	//
	b[i++] = ESC;
	b[i++] = '(';
	i += strlen(itoa(1014, b + i, 10));
	b[i++] = 'X';
	//
	return i;
}

int SLAPI PCLCmdSet::ResetPrinter(char * b)
{
	return InitPrinter(b);
}

int SLAPI PCLCmdSet::SetPageLength(int l, char * b)
{
	// Esc&l#P
	int i = 0;
	if(checkirange(l, 1, 127)) {
		b[i++] = ESC;
		b[i++] = '&';
		b[i++] = 'l';
		i += strlen(itoa(l, b + i, 10));
		b[i++] = 'P';
		return i;
	}
	return i;
}

int SLAPI PCLCmdSet::SetOrientation(int o, char * b)
{
	// Esc&l#O
	b[0] = ESC;
	b[1] = '&';
	b[2] = 'l';
	b[3] = o ? '1' : '0';
	b[4] = 'O';
	return 5;
}

int SLAPI PCLCmdSet::SetMargin(int what, int m, char * b)
{
	int i = 0;
	if(what == SPMRG_LEFT) {     // Esc&a#L
		b[i++] = ESC;
		b[i++] = '&';
		b[i++] = 'a';
		i += strlen(itoa(m, b + i, 10));
		b[i++] = 'L';
	}
	else if(what == SPMRG_TOP) { // Esc&l#E
		b[i++] = ESC;
		b[i++] = '&';
		b[i++] = 'l';
		i += strlen(itoa(m, b + i, 10));
		b[i++] = 'E';
	}
	return i;
}

int SLAPI PCLCmdSet::SetCPI(int cpi, char * b)
{
	// Esc(s#H
	int c, i = 0;
	if(cpi == SPCPI_10)
		c = 10;
	else if(cpi == SPCPI_12)
		c = 12;
	else if(cpi == SPCPI_COND)
		c = 20;
	else
		return 0;
	b[i++] = ESC;
	b[i++] = '(';
	b[i++] = 's';
	i += strlen(itoa(c, b + i, 10));
	b[i++] = 'H';
	return i;
}

int SLAPI PCLCmdSet::SetFontStyle(int s, int on_off, char * b)
{
	switch(s) {
		case SPFS_BOLD:   // Esc(s#B
			b[0] = ESC;
			b[1] = '(';
			b[2] = 's';
			b[3] = on_off ? '1' : '0';
			b[4] = 'B';
			return 5;
		case SPFS_ITALIC: // Esc(s#S
			b[0] = ESC;
			b[1] = '(';
			b[2] = 's';
			b[3] = on_off ? '1' : '0';
			b[4] = 'S';
			return 5;
		case SPFS_UNDERLINE:
			b[0] = ESC;
			b[1] = '&';
			b[2] = 'd';
			if(on_off) {  // Esc&d0D
				b[3] = '0';
				b[4] = 'D';
				return 5;
			}
			else {        // Esc&d@
				b[3] = '@';
				return 4;
			}
	}
	return 0;
}

int SLAPI PCLCmdSet::SetLinesPerInch(int l, char * b)
{
	// Esc&l#D
	int i = 0;
	b[i++] = ESC;
	b[i++] = '&';
	b[i++] = 'l';
	i += strlen(itoa(l, b + i, 10));
	b[i++] = 'D';
	return i;
}

SPrinter * SLAPI getDefaultPrinter()
{
	static SPrinter _printer; // @global
	_printer.device     = 0;
	strcpy(_printer.port, "LPT1");
	_printer.prnport    = 0;
	_printer.fd         = -1;
	_printer.pgl        = 0;	// 66;
	_printer.pgw        = 255;
	_printer.leftMarg   = 0;
	_printer.rightMarg  = 0;
	_printer.topMarg    = 0;
	_printer.bottomMarg = 0;
	_printer.options    = SPRN_EJECTAFTER;
	return &_printer;
}

SLAPI SPrinter::SPrinter()
{
	THISZERO();
	setupCmdSet(SPCMDSET_DEFAULT, 0);
	device     = 0;
	strcpy(port, "LPT1");
	prnport    = 0;
	captured   = 0;
	pgl        = 0;		// 66;
	pgw        = 255;
	leftMarg   = 0;
	rightMarg  = 0;
	topMarg    = 0;
	bottomMarg = 0;
	options    = SPRN_EJECTAFTER;
	fd         = -1;
}

SLAPI SPrinter::~SPrinter()
{
	delete device;
	if(fd >= 0)
		close(fd);
	delete cmdSet;
}

int SLAPI SPrinter::setupCmdSet(long scmdsetID, long extra)
{
	if(cmdSet)
		delete cmdSet;
	cmdSet = SPrnCmdSet::CreateInstance(scmdsetID, extra);
	return 1;
}

int SLAPI SPrinter::setPort(char *aPort)
{
	return (strcpy(port, aPort), 1);
}

int SLAPI SPrinter::checkPort()
{
	ushort st;
	int err = 0;
	if(fd < 0)
		err = (SLibError = SLERR_WRITEFAULT, 1);
	else if(prnport && !captured)
		do {
			st = biosprint(2, 0, prnport - 1);
			err = 1;
			if((st & 0x08) || !(st & 0x10))
				SLibError = SLERR_PRTNOTREADY;
			else if(st & 0x01)
				SLibError = SLERR_PRTBUSY;
			else if(st & 0x20)
				SLibError = SLERR_PRTOUTOFPAPER;
			else
				err = 0;
		} while(err && HandlePrintError && HandlePrintError(SLibError));
	return !err;
}

int SLAPI SPrinter::printChar(int c)
{
	if(checkPort())
		return (write(fd, &c, 1) < 0) ? (SLibError = SLERR_WRITEFAULT, 0) : 1;
	return 0;
}

int SLAPI SPrinter::printLine(const char * b, size_t maxLen)
{
	size_t p = 0;
	while(b[p] && (!maxLen || p < maxLen))
		if(!printChar(b[p++]))
			return 0;
	return 1;
}

int SLAPI SPrinter::escape(int c, char * s)
{
	for(int i = 0; i < c; i++)
		if(!checkPort())
			return 0;
		else if(_write(fd, s + i, 1) < 0)
			return (SLibError = SLERR_WRITEFAULT, 0);
	return 1;
}

int SLAPI SPrinter::setEffect(int s)
{
	int  i = 0;
	char b[128];
	if(s != 0) {
		i += cmdSet->SetFontStyle(SPFS_BOLD, (s & FONT_BOLD) ? 1 : 0, b + i);
		i += cmdSet->SetFontStyle(SPFS_ITALIC, (s & FONT_ITALIC) ? 1 : 0, b + i);
		i += cmdSet->SetFontStyle(SPFS_UNDERLINE, (s & FONT_UNDERLINE) ? 1 : 0, b + i);
	}
	return escape(i, b);
}

int SLAPI SPrinter::initDevice()
{
	int  i = 0;
	char b[128];
	i += cmdSet->InitPrinter(b + i);
	i += cmdSet->SetLinesPerInch(6/*8*/, b + i);
	i += cmdSet->SetMargin(SPMRG_LEFT, leftMarg, b + i);
	i += cmdSet->SetMargin(SPMRG_TOP, 1, b + i);
	//i += cmdSet->SetPageLength(pgl, b + i);
	if(options & SPRN_CPI10)
		i += cmdSet->SetCPI(SPCPI_10, b + i);
	else
		i += cmdSet->SetCPI(SPCPI_12, b + i);
	if(options & SPRN_CONDS)
		i += cmdSet->SetCPI(SPCPI_COND, b + i);
	i += cmdSet->SetQuality((options & SPRN_NLQ) ? SPQLTY_NLQ : SPQLTY_DRAFT, b + i);
	i += cmdSet->SetOrientation((options & SPRN_LANDSCAPE) ? 1 : 0, b + i);
	return escape(i, b);
}

int SLAPI SPrinter::resetDevice()
{
	int i = 0;
	char b[32];
	i += cmdSet->ResetPrinter(b + i);
	return escape(i, b);
}

int SLAPI SPrinter::startDoc()
{
	if(*strip(port) == 0) {
		char   port_buf[32];
		setPort(GetComDvcSymb(comdvcsLpt, 1, 0, port_buf, sizeof(port_buf)));
	}
	if(fd >= 0) {
		close(fd);
		fd = -1;
	}
	prnport = 0;
	if(IsComDvcSymb(port, &prnport) == comdvcsPrn)
		prnport = 1;
	else if(IsComDvcSymb(port, &prnport) != comdvcsLpt)
		prnport = 0;
	if(prnport > 0 && prnport < 4) {
		options |= SPRN_TRUEPRINTER;
		captured = NWGetCaptureStatus(prnport-1);
	}
	else {
		options &= ~SPRN_TRUEPRINTER;
		captured = 0;
	}
	fd = open(port, O_CREAT | O_TRUNC | O_TEXT | O_WRONLY, S_IWRITE);
	if(checkPort()) {
		initDevice();
		if(options & SPRN_EJECTBEFORE)
			endPage();
		return 1;
	}
	return 0;
}

int SLAPI SPrinter::endDoc()
{
	if(fd >= 0) {
		if(options & SPRN_EJECTAFTER)
			endPage();
		resetDevice();
		close(fd);
		fd = -1;
	}
	return 1;
}

int SLAPI SPrinter::abortDoc()
{
	if(fd >= 0) {
		//resetDevice();
		close(fd);
		fd = -1;
	}
	return 1;
}

int SLAPI SPrinter::startPage()
{
	return checkPort();
}

int SLAPI SPrinter::endPage()
{
	return printChar('\x0C');
}

#endif // } __WIN32__
