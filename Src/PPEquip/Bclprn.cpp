// BCLPRN.CPP
// Copyright (c) A.Sobolev 1999, 2000, 2001, 2002, 2003, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017
//
#include <pp.h>
#pragma hdrstop
//
// @ModuleDef(PPObjBarcodePrinter)
//
SLAPI PPBarcodePrinter2::PPBarcodePrinter2()
{
	THISZERO();
}

void SLAPI PPBarcodePrinter2::Normalyze()
{
	PortEx.CopyTo(Port, sizeof(Port));
	SETFLAG(Flags, fPortEx, (PortEx.Len() >= sizeof(Port)));
}

SLAPI PPObjBarcodePrinter::PPObjBarcodePrinter(void * extraPtr) : PPObjReference(PPOBJ_BCODEPRINTER, extraPtr)
{
}

int SLAPI PPObjBarcodePrinter::GetPacket(PPID id, PPBarcodePrinter * pPack)
{
	PPBarcodePrinter rec;
	int    ok = Search(id, &rec);
	if(ok > 0) {
		if(rec.Flags & PPBarcodePrinter::fPortEx)
			ref->GetPropVlrString(Obj, id, BCPPRP_PORTEX, rec.PortEx);
		else
			rec.PortEx = rec.Port;
		ASSIGN_PTR(pPack, rec);
	}
	return ok;
}

int SLAPI PPObjBarcodePrinter::PutPacket(PPID * pID, PPBarcodePrinter * pPack, int use_ta)
{
	int    ok = 1;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID) {
			if(pPack) {
				THROW(CheckDupName(*pID, pPack->Name));
				THROW(ref->UpdateItem(Obj, *pID, pPack, 1, 0));
			}
			else {
				THROW(ref->RemoveItem(Obj, *pID, 0));
				DS.LogAction(PPACN_OBJRMV, Obj, *pID, 0, 0);
			}
		}
		else {
			*pID = pPack->ID;
			THROW(ref->AddItem(Obj, pID, pPack, 0));
		}
		if(*pID) {
			const char * p = 0;
			if(pPack && pPack->Flags & PPBarcodePrinter::fPortEx)
				p = pPack->PortEx;
			THROW(ref->PutPropVlrString(Obj, *pID, BCPPRP_PORTEX, p));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjBarcodePrinter::Edit(PPID * pID, void * extraPtr)
{
	class BarcodePrinterDialog : public TDialog {
	public:
		BarcodePrinterDialog() : TDialog(DLG_BCPRT)
		{
			PPSetupCtrlMenu(this, CTL_BCPRT_PORT, CTLMNU_BCPRT_PORT, CTRLMENU_SELPRINTER);
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(TVKEYDOWN) {
				if(TVKEY == kbF2) {
					SString prn_port;
					if(SelectPrinterFromWinPool(prn_port) > 0)
						setCtrlString(CTL_BCPRT_PORT, prn_port);
					clearEvent(event);
				}
			}
		}
	};
	int    ok = 1, r = cmCancel, valid_data = 0, is_new = 0;
	BarcodePrinterDialog * dlg = 0;
	PPBarcodePrinter rec;
	THROW(CheckDialogPtr(&(dlg = new BarcodePrinterDialog())));
	THROW(EditPrereq(pID, dlg, &is_new));
	if(!is_new)
		THROW(GetPacket(*pID, &rec) > 0);
	dlg->SetCtrlBitmap(CTL_BCPRT_BMP, BM_BARCODEPRINTER);
	dlg->setCtrlData(CTL_BCPRT_NAME, rec.Name);
	dlg->setCtrlString(CTL_BCPRT_PORT, rec.PortEx);
	dlg->setCtrlData(CTL_BCPRT_FORM, rec.LabelName);
	SetupStringCombo(dlg, CTLSEL_BCPRT_DEVICE, PPTXT_BCPT, rec.PrinterType);
	dlg->AddClusterAssocDef(CTL_BCPRT_BAUD, 0, cbr9600);
	dlg->AddClusterAssoc(CTL_BCPRT_BAUD,    1, cbr19200);
	dlg->SetClusterData (CTL_BCPRT_BAUD,    rec.Cpp.Cbr);
	dlg->AddClusterAssocDef(CTL_BCPRT_PARITY, 0, 0);
	dlg->AddClusterAssoc(CTL_BCPRT_PARITY,  1, 1);
	dlg->AddClusterAssoc(CTL_BCPRT_PARITY,  2, 2);
	dlg->SetClusterData (CTL_BCPRT_PARITY,  rec.Cpp.Parity);
	dlg->AddClusterAssocDef(CTL_BCPRT_DATA, 0, 8);
	dlg->AddClusterAssoc(CTL_BCPRT_DATA,    1, 7);
	dlg->SetClusterData (CTL_BCPRT_DATA,    rec.Cpp.ByteSize);
	dlg->setCtrlData(CTL_BCPRT_NARROWPT, &rec.BcNarrowPt); // @v8.0.9
	dlg->setCtrlData(CTL_BCPRT_WIDEPT,   &rec.BcWidePt);   // @v8.0.9
	// @v9.2.7 {
	{
		ushort cpv = 0;
		if(rec.Cp == cpOEM)
			cpv = 1;
		dlg->setCtrlUInt16(CTL_BCPRT_CP, cpv);
	}
	// } @v9.2.7
	while(!valid_data && (r = ExecView(dlg)) == cmOK) {
		long   temp_long;
		dlg->getCtrlData(CTL_BCPRT_NAME,   rec.Name);
		dlg->getCtrlString(CTL_BCPRT_PORT, rec.PortEx);
		dlg->getCtrlData(CTL_BCPRT_FORM,   rec.LabelName);
		dlg->getCtrlData(CTLSEL_BCPRT_DEVICE, &rec.PrinterType);
		dlg->GetClusterData(CTL_BCPRT_BAUD,   &rec.Cpp.Cbr);
		dlg->GetClusterData(CTL_BCPRT_PARITY, &temp_long);
		rec.Cpp.Parity = (int8)temp_long;
		dlg->GetClusterData(CTL_BCPRT_DATA,   &temp_long);
		rec.Cpp.ByteSize = (int8)temp_long;
		dlg->getCtrlData(CTL_BCPRT_NARROWPT, &rec.BcNarrowPt); // @v8.0.9
		dlg->getCtrlData(CTL_BCPRT_WIDEPT,   &rec.BcWidePt);   // @v8.0.9
		// @v9.2.7 {
		{
			ushort cpv = dlg->getCtrlUInt16(CTL_BCPRT_CP);
			if(cpv == 1)
				rec.Cp = cpOEM;
			else
				rec.Cp = cpANSI;
		}
		// } @v9.2.7
		if(*pID)
			*pID = rec.ID;
		rec.Normalyze();
		if(PutPacket(pID, &rec, 1))
			valid_data = 1;
		else
			PPError();
	}
	CATCHZOKPPERR
	delete dlg;
	return ok ? r : 0;
}
/*
	Label format definition

@gname    - Goods Name
@price    - Price
@revalprice - Цена до переоценки // @v6.9.0
@expiry   - Expiration
@manuf    - Manufacturer
@country  - Manufacturer country
@brand    - Торговая марка
@date     - Print date
@time     - Print time
@user     - User name

@serial   - Серийный номер лота
@billdate - Дата документа прихода лота
@billno   - Номер документа прихода лота
@qtty     - Оприходованное количество в лоте
@phqtty   - Оприходованное количество в лоте в физических единицах
@pack     - Емкость упаковки
@brutto   - Масса брутто оприходованного количества
@serxnom  - Серийный номер со следующим за ним через разделитель 'X' номенклатурынм кодом товара (Code39)

@ext_a - дополнительное поле товара A
@ext_b - дополнительное поле товара B
@ext_c - дополнительное поле товара C
@ext_d - дополнительное поле товара D
@ext_e - дополнительное поле товара E

@gcdimx     - //
@gcdimy     - //
@gcdimz     - //
@gcdimw     - //

@gckind     - //
@gcgrade    - //
@gcaddprop  - //
@gcaddprop2 - //

Default filename {PPY_BIN}\\BARLABEL.LBL

 ********* Formal barcode label syntax definition

label_file = unit_list

unit_list = unit | unit unit_list

unit = label | load

label = LABEL label_name '{' label_param_list '.' label_format '}'
label_param_list = label_param | label_param_list
label_param = MEMORY memory_module_name | codepage page_name | SIZE x_pos y_pos | HOME  x_pos y_pos | GAP y_size
label_format = label_format_statement | label_format
label_format_statement = text_statement | barcode_statement
text_statement = TEXT rotation x_pos y_pos font_name x_multiplier y_multiplier "text"
barcode_statement = BARCODE rotation x_pos y_pos height x_multiplier y_multiplier
barcode_statement = SERIALCODE  rotation x_pos y_pos height x_multiplier y_multiplier
barcode_statement = SERXNOM rotation x_pos y_pos height x_multiplier y_multiplier

page_name = windows-1251 | cp866

load = LOAD load_name '{' load_op_list '}'
load_op_list = load_op | load_op load_op_list
load_op = "text" | file "file_name"

 ********* Example

label Label1 {
memory A
size x y
home x y
.
text    0 20  10 A1 1 1 "@gname"
barcode 0 20  30 70 1 1
text    0 20 100 A1 1 1 "–Ґ­ : @price"
text    0 20 120 A1 1 1 "„ в  ¬ аЄЁа®ўЄЁ: @date"
text    0 20 140 A1 1 1 "Њ аЄЁа®ўйЁЄ: @user"
}

load FONTS {
	file "HELVDL12.SFP"
	"\x1B*c130D\xD\xA    "
	file "NTCOUR12.SFP"
	"\x1B*c170D\xD\xA    "
	file "ARIALC24.SFP"
	"\x1B*c114D\xD\xA    "
}
*/

// @vmiller
struct UsbOpt {
	char * P_Vid;
	char * P_Pid;
};

// @vmiller
UsbOpt EltronUsbOpt[] = {
	{"Vid_1203",	"Pid_0160"} // TSC TDP-225
};

struct BarcodeLabelParam {
	char   Name[64];
	int    SizeX;         // Ширина этикетки
	int    SizeY;         // Высота этикетки
	int    Gap;           // Размер зазора между этикетками
	int    HomeX;         // Начальная позиция этикетки по ширине
	int    HomeY;         // Начальная позиция этикетки по высоте
	char   MemModule[32]; // Наименование модуля памяти
	char   Codepage[32];  // Кодовая страница (windows-1251 | cp866)
	//
	int    BcNarrowPt;    // @v8.0.9
	int    BcWidePt;      // @v8.0.9
};

struct BarcodeLabelEntry {
	enum EntryType {
		 etText = 1, // LABEL & LOAD
		 etBarcode,  // LABEL
		 etFile,     // LOAD
		 etWrap      // LABEL Перенос предыдущей строки текста
	};
	EntryType Type;
	int    Rotation; // Degree (0, 90, 180, 270)
	int    XMult;
	int    YMult;
	int    XPos;
	int    YPos;
	int    FontHeight;
	int    FontWidth;
	int    BarcodeHeight;
	int    BarcodeStd;
	char   Font[32];
	uint   TextIdx;
	int    TextMaxLen; // Максимальная длина строки (символов)
	const  char * Text;
};

enum BarcodeFormatToken {
	tokEOL = 0,
	tokIdent,
	tokDot,
	tokLBrace,
	tokRBrace,
	tokLabel,
	tokSize,
	tokHome,
	tokText,
	tokBarcode,
	tokNumber,
	tokString,
	tokGap,
	tokSerial,
	tokMemory,
	tokCodepage,
	tokLoad,
	tokFile,
	tokSerXNom, // (Серийный номер) 'X' (номенклатурный номер)
	tokWrap
};

class BarcodeLabel : private SArray {
public:
	SLAPI  BarcodeLabel();
	SLAPI ~BarcodeLabel();
	const  BarcodeLabelParam * SLAPI GetParam() const;
	uint   SLAPI GetEntryCount() const;
	const  BarcodeLabelEntry * SLAPI GetEntry(uint);
	int    SLAPI AddEntry(BarcodeLabelEntry *);
	int    SLAPI ParseFormat(const RetailGoodsInfo *, const char * pFileName, const char * pFormatName);
	void   SLAPI SetBarcodeWidth(int narrowPt, int widePt)
	{
		BLP.BcNarrowPt = narrowPt;
		BLP.BcWidePt = widePt;
	}
private:
	BarcodeFormatToken SLAPI NextToken(char ** ppLine, char * pBuf, size_t buflen);
	int    SLAPI GetNumber(char ** ppLine, int * pNumber);
	int    SLAPI GetPoint(char ** ppLine, int * pX, int * pY);
	int    SLAPI GetText(int wrap, char ** ppLine);
	int    SLAPI GetBarcode(int serial, char ** ppLine);
	int    SLAPI SubstVar(char ** ppSrc, char ** ppDest);
	int    SLAPI WrapText(const char * pText, uint maxLen, SString & rHead, SString & rTail);
	void   SLAPI UpdateText(uint entryPos, const SString & rText);

	StringSet StrBuf;
	BarcodeLabelParam BLP;
	RetailGoodsInfo   RGI;
	char   TextBuf[256];
	SString VarString;
	PPGoodsPacket * P_GPack;
	PPGdsClsPacket * P_GcPack;
};

class DatamaxLabelPrinter : public BarcodeLabelPrinter {
public:
	SLAPI  DatamaxLabelPrinter(const PPBarcodePrinter & rPrnPack) : BarcodeLabelPrinter(rPrnPack), NumCopies(1)
	{
	}
	virtual int SLAPI StartLabel(const BarcodeLabelParam *, int numCopies);
	virtual int SLAPI EndLabel();
	virtual int SLAPI PutDataEntry(const BarcodeLabelEntry *);
private:
	int    NumCopies;
};

class ZebraLabelPrinter : public BarcodeLabelPrinter {
public:
	SLAPI  ZebraLabelPrinter(const PPBarcodePrinter & rPrnPack) : BarcodeLabelPrinter(rPrnPack), NumCopies(1)
	{
	}
	virtual int SLAPI StartLabel(const BarcodeLabelParam *, int numCopies);
	virtual int SLAPI EndLabel();
	virtual int SLAPI PutDataEntry(const BarcodeLabelEntry *);
private:
	int    SLAPI PutCtrl(uint16);
	int    SLAPI PutPosition(uint16 ctrl, int x, int y);
	int    NumCopies;
};

class EltronLabelPrinter : public BarcodeLabelPrinter {
public:
	SLAPI  EltronLabelPrinter(const PPBarcodePrinter & rPrnPack) : BarcodeLabelPrinter(rPrnPack),
		NumCopies(1), BcNarrowPt(0), BcWidePt(0)
	{
	}
	virtual int SLAPI StartLabel(const BarcodeLabelParam *, int numCopies);
	virtual int SLAPI EndLabel();
	virtual int SLAPI PutDataEntry(const BarcodeLabelEntry *);
private:
	int    SLAPI PutDataEntryPrefix(char letter, const BarcodeLabelEntry *);
	int    NumCopies;
	int    BcNarrowPt;
	int    BcWidePt;
};

class WindowsLabelPrinter : public BarcodeLabelPrinter {
public:
	SLAPI  WindowsLabelPrinter(const PPBarcodePrinter & rPrnPack) : BarcodeLabelPrinter(rPrnPack)
	{
	}
	virtual int SLAPI StartLabel(const BarcodeLabelParam *, int numCopies)
	{
		return -1;
	}
	virtual int SLAPI EndLabel()
	{
		return -1;
	}
	virtual int SLAPI PutDataEntry(const BarcodeLabelEntry *)
	{
		return -1;
	}
private:
};
//
// BarcodeLabel
//
enum BarcodeVarStr {
	bcvsLabel = 0,
	bcvsSize,
	bcvsHome,
	bcvsText,
	bcvsBarcode,
	bcvsSerialCode,
	bcvsGap,

	bcvsGoodsName,
	bcvsPrice,
	bcvsRevalPrice,
	bcvsExpiry,
	// @v7.5.1 bcvsManuf,
	bcvsCountry,
	bcvsDate,
	bcvsTime,
	bcvsUser,
	bcvsSerial,
	bcvsBillDate,
	bcvsBillNo,
	bcvsQtty,
	bcvsPhQtty,
	bcvsPack,
	bcvsBrutto,
	bcvsObjName,      // objname
	bcvsObj2Name,     // obj2name
	bcvsExtA,
	bcvsExtB,
	bcvsExtC,
	bcvsExtD,
	bcvsExtE,
	bcvsPrcName,      // prcname
	bcvsGoodsGrpName, // goodsgrpname Наименование группы товара

	bcvsBrand,        //
	bcvsGcDimX,       //
	bcvsGcDimY,       //
	bcvsGcDimZ,       //
	bcvsGcDimW,       //

	bcvsGcKind,       //
	bcvsGcGrade,      //
	bcvsGcAddProp2,   //
	bcvsGcAddProp,    //
	bcvsLineCost,     //
	bcvsLinePrice,    //
	bcvsManufDate,    // @v7.5.1 Дата/время производства
	bcvsManuf,        // @v7.5.1 Перенесено сверху-вниз (во избежании коллизии с bcvsManufDate)
	bcvsQuot,         // @v7.6.10 quot:
	bcvsGoodsCode,    // @v8.1.3 Текстовое представление кода товара

	bcvsMemory,
	bcvsCodepage,
	bcvsLoad,         // Начало формата загрузки данных на принтер
	bcvsFile,         // file
	bcvsSerXNom,      //
	bcvsWrap          // Перенос предыдущей строки
};

#define FIRSTSUBSTVAR bcvsGoodsName
#define NUMSUBSTVARS  39 // 24-->33, @v7.2.12 34->36, @v7.5.1 36-->37 @v7.6.10 37-->38 @v8.1.3 38-->39

SLAPI BarcodeLabel::BarcodeLabel() : SArray(sizeof(BarcodeLabelEntry)), P_GPack(0), P_GcPack(0)
{
	MEMSZERO(BLP);
	uint   pos = 0;
	StrBuf.add("!", &pos);
}

SLAPI BarcodeLabel::~BarcodeLabel()
{
	delete P_GPack;
	delete P_GcPack;
}

const BarcodeLabelParam * SLAPI BarcodeLabel::GetParam() const
{
	return &BLP;
}

uint SLAPI BarcodeLabel::GetEntryCount() const
{
	return getCount();
}

const BarcodeLabelEntry * SLAPI BarcodeLabel::GetEntry(uint pos)
{
	BarcodeLabelEntry * p_entry = (BarcodeLabelEntry *)at(pos);
	if(p_entry->TextIdx) {
		uint   pos = p_entry->TextIdx;
		StrBuf.get(&pos, TextBuf, sizeof(TextBuf));
		p_entry->Text = TextBuf;
	}
	else
		p_entry->Text = 0;
	return (const BarcodeLabelEntry *)p_entry;
}

int SLAPI BarcodeLabel::AddEntry(BarcodeLabelEntry * pEntry)
{
	if(pEntry->Text) {
		uint   pos = 0;
		StrBuf.add(pEntry->Text, &pos);
		pEntry->Text = 0;
		pEntry->TextIdx = pos;
	}
	else
		pEntry->TextIdx = 0;
	return insert(pEntry) ? 1 : PPSetErrorSLib();
}

void SLAPI BarcodeLabel::UpdateText(uint entryPos, const SString & rText)
{
	BarcodeLabelEntry * p_entry = (BarcodeLabelEntry *)at(entryPos);
	if(rText.NotEmpty()) {
		uint   pos = 0;
		StrBuf.add(rText, &pos);
		p_entry->TextIdx = pos;
	}
	else
		p_entry->TextIdx = 0;
}

BarcodeFormatToken SLAPI BarcodeLabel::NextToken(char ** ppLine, char * pBuf, size_t buflen)
{
	BarcodeFormatToken tok;
	char * s = *ppLine;
	if(*s == 0) {
		tok = tokEOL;
		ASSIGN_PTR(pBuf, 0);
	}
	else {
		const  size_t tbs = 512;
		char   word[tbs];
		size_t dp = 0;
		while(oneof2(*s, ' ', '\t'))
			s++;
		if(*s == '\"') {
			s++;
			while(!oneof2(*s, '\"', 0) && dp < tbs-2) {
				if(*s == '\\') {
					s++;
					if(toupper(*s) == 'X') {
						*s++;
						int    hc = 0;
						if(isxdigit(*s)) {
							int    h = toupper(*s);
							if(isdec(h))
								hc += (h-'0');
							else if(h >= 'A' && h <= 'F')
								hc += 10 + (h-'A');
							s++;
							if(isxdigit(*s)) {
								hc *= 16;
								h = toupper(*s);
								if(isdec(h))
									hc += (h-'0');
								else if(h >= 'A' && h <= 'F')
									hc += 10 + (h-'A');
								s++;
							}
						}
						word[dp++] = hc;
					}
					else if(*s)
						word[dp++] = *s++;
				}
				else
					word[dp++] = *s++;
			}
			s++;
			word[dp] = 0;
			tok = tokString;
		}
		else  {
			while(!oneof5(*s, ' ', '\t', '\"', ':', 0) && dp < tbs-2) {
				word[dp++] = *s++;
			}
			word[dp] = 0;
			if(word[0] == '{')
				tok = tokLBrace;
			else if(word[0] == '}')
				tok = tokRBrace;
			else {
				int var_idx = -1;
				if(!PPSearchSubStr(VarString, &var_idx, word, 1))
					var_idx = -1;
				switch(var_idx) {
					case bcvsLabel:      tok = tokLabel;   break;
					case bcvsSize:       tok = tokSize;    break;
					case bcvsHome:       tok = tokHome;    break;
					case bcvsGap:        tok = tokGap;     break;
					case bcvsMemory:     tok = tokMemory;  break;
					case bcvsCodepage:   tok = tokCodepage; break;
					case bcvsText:       tok = tokText;    break;
					case bcvsWrap:       tok = tokWrap;    break;
					case bcvsBarcode:    tok = tokBarcode; break;
					case bcvsSerialCode: tok = tokSerial;  break;
					case bcvsLoad:       tok = tokLoad;    break;
					case bcvsFile:       tok = tokFile;    break;
					case bcvsSerXNom:    tok = tokSerXNom; break;
					default:
						{
							size_t len = strlen(word);
							tok = tokNumber;
							for(size_t i = 0; tok != tokIdent && i < len; i++)
								if(!isdec(word[i]))
									tok = tokIdent;
						}
						break;
				}
			}
		}
		strnzcpy(pBuf, word, buflen);
		*ppLine = s;
	}
	return tok;
}

int SLAPI BarcodeLabel::GetNumber(char ** ppLine, int * pNumber)
{
	char   temp[32];
	const char * p_org = *ppLine;
	BarcodeFormatToken tok = NextToken(ppLine, temp, sizeof(temp));
	if(tok == tokNumber) {
		*pNumber = atoi(temp);
		return 1;
	}
	else
		return PPSetError(PPERR_BARLAB_NMBEXP, p_org);
}

int SLAPI BarcodeLabel::GetPoint(char ** ppLine, int * pX, int * pY)
{
	const char * p_org = *ppLine;
	return (GetNumber(ppLine, pX) && GetNumber(ppLine, pY)) ? 1 : PPSetError(PPERR_BARLAB_POINTEXP, p_org);
}

int SLAPI BarcodeLabel::SubstVar(char ** ppSrc, char ** ppDest)
{
	PPObjGoods goods_obj;
	char * s = *ppSrc;
	char * d = *ppDest;
	char   temp[256];
	SString temp_str;
	temp_str.Space() = 0; // Для уверенности, что буфер не нулевой
	if(*s == '@')
		s++;
	for(int i = FIRSTSUBSTVAR; i < FIRSTSUBSTVAR + NUMSUBSTVARS; i++) {
		if(PPGetSubStr(VarString, i, temp, sizeof(temp)) && strnicmp(s, temp, strlen(temp)) == 0) {
			size_t var_len = strlen(temp);
			LDATE  curdate;
			LTIME  curtime;
			double rval = 0.0;
			long   lval = 0;
			PPSecur secur;
			switch(i) {
				case bcvsGoodsName:
					d = stpcpy(d, RGI.Name);
					break;
				case bcvsGoodsGrpName:
					{
						Goods2Tbl::Rec goods_rec;
						if(goods_obj.Fetch(RGI.ID, &goods_rec) > 0 && goods_rec.ParentID) {
							if(GetGoodsNameR(goods_rec.ParentID, temp_str) > 0)
								d = stpcpy(d, temp_str);
						}
					}
					break;
				case bcvsPrice:
					d = stpcpy(d, realfmt(RGI.Price, MKSFMTD(0, 2, NMBF_NOZERO), temp));
					break;
				case bcvsRevalPrice:
					d = stpcpy(d, realfmt(RGI.RevalPrice, MKSFMTD(0, 2, NMBF_NOZERO), temp));
					break;
				case bcvsLineCost:
					d = stpcpy(d, realfmt(NZOR(RGI.LineCost, RGI.Cost), MKSFMTD(0, 2, NMBF_NOZERO), temp));
					break;
				case bcvsLinePrice:
					d = stpcpy(d, realfmt(NZOR(RGI.LinePrice, RGI.Price), MKSFMTD(0, 2, NMBF_NOZERO), temp));
					break;
				case bcvsExpiry:
					d = stpcpy(d, datefmt(&RGI.Expiry, DATF_DMY, temp));
					break;
				case bcvsManufDate:
					temp_str.Z().Cat(RGI.ManufDtm, DATF_DMY|DATF_NOZERO, TIMF_HM|TIMF_NOZERO);
					d = stpcpy(d, temp_str);
					break;
				case bcvsManuf:
					d = stpcpy(d, RGI.Manuf);
					break;
				case bcvsCountry:
					d = stpcpy(d, RGI.ManufCountry);
					break;
				case bcvsDate:
					curdate = getcurdate_();
					d = stpcpy(d, datefmt(&curdate, DATF_DMY, temp));
					break;
				case bcvsTime:
					curtime = getcurtime_();
					d = stpcpy(d, timefmt(curtime, TIMF_HM, temp));
					break;
				case bcvsUser:
					if(SearchObject(PPOBJ_USR, LConfig.User, &secur) > 0)
						if(GetPersonName(secur.PersonID, temp_str) > 0)
							d = stpcpy(d, temp_str);
						else
							d = stpcpy(d, secur.Name);
					break;
				case bcvsSerial:
					d = stpcpy(d, RGI.Serial);
					break;
				case bcvsBillDate:
					d = stpcpy(d, datefmt(&RGI.BillDate, DATF_DMY, temp));
					break;
				case bcvsBillNo:
					d = stpcpy(d, RGI.BillCode);
					break;
				case bcvsQtty:
					d = stpcpy(d, realfmt(RGI.Qtty, MKSFMTD(0, 6, NMBF_NOZERO | NMBF_NOTRAILZ), temp));
					break;
				case bcvsPhQtty:
					d = stpcpy(d, realfmt(RGI.PhQtty, MKSFMTD(0, 6, NMBF_NOZERO | NMBF_NOTRAILZ), temp));
					break;
				case bcvsPack:
					d = stpcpy(d, realfmt(RGI.UnitPerPack, MKSFMTD(0, 6, NMBF_NOZERO | NMBF_NOTRAILZ), temp));
					break;
				case bcvsBrutto:
					d = stpcpy(d, realfmt(RGI.Brutto, MKSFMTD(0, 6, NMBF_NOZERO | NMBF_NOTRAILZ), temp));
					break;
				case bcvsObjName:
					d = stpcpy(d, RGI.ArName);
					break;
				case bcvsObj2Name:
					d = stpcpy(d, RGI.Ar2Name);
					break;
				case bcvsPrcName:
					d = stpcpy(d, RGI.PrcName);
					break;
				case bcvsGcDimX:
				case bcvsGcDimY:
				case bcvsGcDimZ:
				case bcvsGcDimW:
				case bcvsGcKind:
				case bcvsGcGrade:
				case bcvsGcAddProp:
				case bcvsGcAddProp2:
				case bcvsBrand:
				case bcvsExtA:
				case bcvsExtB:
				case bcvsExtC:
				case bcvsExtD:
				case bcvsExtE:
					temp_str = 0;
					if(P_GPack == 0) {
						P_GPack = new PPGoodsPacket;
						goods_obj.GetPacket(RGI.ID, P_GPack, PPObjGoods::gpoSkipQuot); // @v8.3.7 PPObjGoods::gpoSkipQuot
					}
					if(i >= bcvsGcDimX && i <= bcvsGcAddProp) {
						if(P_GcPack == 0 && P_GPack->Rec.GdsClsID) {
							PPObjGoodsClass gc_obj;
							P_GcPack = new PPGdsClsPacket;
							gc_obj.Fetch(P_GPack->Rec.GdsClsID, P_GcPack);
						}
					}
					if(i == bcvsExtA)
						P_GPack->GetExtStrData(GDSEXSTR_STORAGE, temp_str);
					else if(i == bcvsExtB)
						P_GPack->GetExtStrData(GDSEXSTR_STANDARD, temp_str);
					else if(i == bcvsExtC)
						P_GPack->GetExtStrData(GDSEXSTR_INGRED, temp_str);
					else if(i == bcvsExtD)
						P_GPack->GetExtStrData(GDSEXSTR_ENERGY, temp_str);
					else if(i == bcvsExtE)
						P_GPack->GetExtStrData(GDSEXSTR_USAGE, temp_str);
					else if(i == bcvsBrand) {
						// @v9.5.5 GetGoodsName(P_GPack->Rec.BrandID, temp_str);
						goods_obj.FetchNameR(P_GPack->Rec.BrandID, temp_str); // @v9.5.5
						temp_str.CopyTo(temp, sizeof(temp));
					}
					else if(i == bcvsGcDimX) {
						if(P_GcPack) {
							P_GcPack->GetExtDim(&P_GPack->ExtRec, PPGdsCls::eX, &rval);
							temp_str.Cat(rval, MKSFMTD(0, 6, NMBF_NOZERO | NMBF_NOTRAILZ));
						}
					}
					else if(i == bcvsGcDimY) {
						if(P_GcPack) {
							P_GcPack->GetExtDim(&P_GPack->ExtRec, PPGdsCls::eY, &rval);
							temp_str.Cat(rval, MKSFMTD(0, 6, NMBF_NOZERO | NMBF_NOTRAILZ));
						}
					}
					else if(i == bcvsGcDimZ) {
						if(P_GcPack) {
							P_GcPack->GetExtDim(&P_GPack->ExtRec, PPGdsCls::eZ, &rval);
							temp_str.Cat(rval, MKSFMTD(0, 6, NMBF_NOZERO | NMBF_NOTRAILZ));
						}
					}
					else if(i == bcvsGcDimW) {
						if(P_GcPack) {
							P_GcPack->GetExtDim(&P_GPack->ExtRec, PPGdsCls::eW, &rval);
							temp_str.Cat(rval, MKSFMTD(0, 6, NMBF_NOZERO | NMBF_NOTRAILZ));
						}
					}
					else if(i == bcvsGcKind) {
						if(P_GcPack)
							P_GcPack->GetExtProp(&P_GPack->ExtRec, PPGdsCls::eKind, &lval, temp_str);
					}
					else if(i == bcvsGcGrade) {
						if(P_GcPack)
							P_GcPack->GetExtProp(&P_GPack->ExtRec, PPGdsCls::eGrade, &lval, temp_str);
					}
					else if(i == bcvsGcAddProp) {
						if(P_GcPack)
							P_GcPack->GetExtProp(&P_GPack->ExtRec, PPGdsCls::eAdd, &lval, temp_str);
					}
					else if(i == bcvsGcAddProp2) {
						if(P_GcPack)
							P_GcPack->GetExtProp(&P_GPack->ExtRec, PPGdsCls::eAdd2, &lval, temp_str);
					}
					d = stpcpy(d, temp_str);
					break;
				case bcvsQuot:
					{
						const char * p_symb = s + var_len;
						temp_str = 0;
						PPObjQuotKind qk_obj;
						PPQuotKind qk_rec;
						for(size_t ss_ = 0; p_symb[ss_] && p_symb[ss_] != ' ' && p_symb[ss_] != '\t' && ss_ < (sizeof(qk_rec.Symb)-1); ss_++) {
							temp_str.CatChar(p_symb[ss_]);
							PPID   qk_id = 0;
							if(qk_obj.FetchBySymb(temp_str, &qk_id) > 0) {
								double quot = 0.0;
								double cost_ = NZOR(RGI.LineCost, RGI.Cost);
								double price_ = NZOR(RGI.LinePrice, RGI.Price);
								QuotIdent qi(QIDATE(getcurdate_()), RGI.LocID, qk_id, 0L /* @curID */);
								goods_obj.GetQuotExt(RGI.ID, qi, cost_, price_, &quot, 1);
								d = stpcpy(d, realfmt(quot, MKSFMTD(0, 2, NMBF_NOZERO), temp));
								var_len += (ss_+1);
								break;
							}
						}
					}
					break;
				// @v8.1.3 {
				case bcvsGoodsCode:
					(temp_str = RGI.BarCode).Strip();
					d = stpcpy(d, temp_str);
					break;
				// } @v8.1.3
			}
			s += var_len;
			break;
		}
	}
	*ppSrc = s;
	*ppDest = d;
	return 1;
}

// @todo Заменить использование этой функции методом SString::Wrap
int SLAPI BarcodeLabel::WrapText(const char * pText, uint maxLen, SString & rHead, SString & rTail)
{
	int    ok = 1;
	const  size_t len = sstrlen(pText);
	size_t p = maxLen;
	if(p > 0 && p < len) {
		size_t temp_pos = p;
		size_t next_pos = p;
		while(pText[temp_pos] != ' ')
			if(temp_pos)
				temp_pos--;
			else
				break;
		if(temp_pos) {
			p = temp_pos;
			next_pos = temp_pos+1;
		}
		rHead.CopyFromN(pText, p);
		rTail.CopyFrom(pText+next_pos);
	}
	else {
		rHead = pText;
		rTail.Z();
		ok = -1;
	}
	return ok;
}

int SLAPI BarcodeLabel::GetText(int wrap, char ** ppLine)
{
	int    ok = 1;
	BarcodeLabelEntry entry;
	char   temp[256], str[512];
	SString head, tail;
	BarcodeFormatToken tok;
	MEMSZERO(entry);
	entry.Type = BarcodeLabelEntry::etText;
	//
	// Извлекаем опциональную максимальную длину строки {
	//
	char * s = *ppLine;
	while(oneof2(*s, ' ', '\t'))
		s++;
	if(*s == ':') {
		*ppLine = s+1;
		THROW(GetNumber(ppLine, &entry.TextMaxLen));
	}
	// }
	if(wrap) {
		THROW_PP_S(getCount(), PPERR_BARLAB_NOWRAPTEXT, BLP.Name);
		{
			uint   pos = getCount()-1;
			const  BarcodeLabelEntry * p_entry = GetEntry(pos);
			THROW_PP_S(p_entry->Type == BarcodeLabelEntry::etText, PPERR_BARLAB_NOWRAPTEXT, BLP.Name);
			THROW_PP_S(p_entry->TextMaxLen, PPERR_BARLAB_NOWRAPTEXT, BLP.Name);
			WrapText(p_entry->Text, p_entry->TextMaxLen, head, tail);
			UpdateText(pos, head);
			//
			// Если для этой точки переноса не назначена макс длина строки,
			// то она наследуется от предыдущей точки переноса или текста.
			//
			SETIFZ(entry.TextMaxLen, p_entry->TextMaxLen);
			entry.Text = tail;
		}
	}
	THROW(GetNumber(ppLine, &entry.Rotation));
	THROW(GetPoint(ppLine, &entry.XPos, &entry.YPos));
	tok = NextToken(ppLine, temp, sizeof(temp));
	if(tok == tokIdent) {
		uint   pos = 0;
		StringSet ss(',', temp);
		if(ss.get(&pos, str, sizeof(str))) {
			STRNSCPY(entry.Font, str);
			if(ss.get(&pos, str, sizeof(str))) {
				entry.FontWidth = atoi(str);
				if(ss.get(&pos, str, sizeof(str)))
					entry.FontHeight = atoi(str);
			}
		}
	}
	else {
		THROW(tok == tokNumber);
		STRNSCPY(entry.Font, temp);
	}
	THROW(GetPoint(ppLine, &entry.XMult, &entry.YMult));
	if(!wrap) {
		tok = NextToken(ppLine, temp, sizeof(temp));
		THROW(tok == tokString);
		char * s = temp;
		char * d = str;
		while(*s != 0)
			if(*s != '@')
				*d++ = *s++;
			else
				SubstVar(&s, &d);
		*d = 0;
		if(sstreqi_ascii(BLP.Codepage, "windows-1251"))
			SOemToChar(str);
		entry.Text = str;
	}
	AddEntry(&entry);
	CATCHZOK
	return ok;
}

int SLAPI BarcodeLabel::GetBarcode(int serial, char ** ppLine)
{
	int    ok = 1;
	size_t len = 0;
	PPObjGoods goods_obj;
	size_t check_dig = BIN(goods_obj.GetConfig().Flags & GCF_BCCHKDIG);
	BarcodeLabelEntry entry;
	SString temp_buf;
	MEMSZERO(entry);
	entry.Type = BarcodeLabelEntry::etBarcode;
	THROW(GetNumber(ppLine, &entry.Rotation));
	THROW(GetPoint(ppLine, &entry.XPos, &entry.YPos));
	THROW(GetNumber(ppLine, &entry.BarcodeHeight));
	THROW(GetPoint(ppLine, &entry.XMult, &entry.YMult));
	if(serial == 1) {
		(temp_buf = RGI.Serial).Strip();
		entry.BarcodeStd = BARCSTD_CODE39;
	}
	else if(serial == 0) {
		(temp_buf = RGI.BarCode).Strip();
		if(temp_buf.Len() == (12+check_dig))
			entry.BarcodeStd = BARCSTD_EAN13;
		else if(temp_buf.Len() == (7+check_dig))
			entry.BarcodeStd = (temp_buf.C(0) == '0') ? BARCSTD_UPCE : BARCSTD_EAN8;
		else if(temp_buf.Len() == (11+check_dig))
			entry.BarcodeStd = BARCSTD_UPCA;
		else if(temp_buf.Len() <= 7) {
			temp_buf.PadLeft(12-temp_buf.Len(), '0');
			entry.BarcodeStd = BARCSTD_EAN13;
		}
		else
			entry.BarcodeStd = BARCSTD_EAN13;
	}
	else if(serial == 2) { // SerXNom
		(temp_buf = RGI.Serial).Strip().CatChar('X').Cat(RGI.BarCode);
		entry.BarcodeStd = BARCSTD_CODE39;
	}
	entry.Text = temp_buf;
	THROW(AddEntry(&entry));
	CATCHZOK
	return ok;
}

int SLAPI BarcodeLabel::ParseFormat(const RetailGoodsInfo * pRgi, const char * pFileName, const char * pFormatName)
{
	int    ok = -1;
	char   buf[512];
	int    skip_label = 0;
	int    zone = 0; // 1 - label param, 2 - label format, 11 - load
	int    line_no = 0;
	if(!RVALUEPTR(RGI, pRgi))
		RGI.Init();
	MEMSZERO(BLP);
	FILE * f = 0;
	if(VarString.Empty())
		PPLoadText(PPTXT_BCLABEL_VARS, VarString);
	ZDELETE(P_GPack);
	THROW_PP_S(f = fopen(pFileName, "rt"), PPERR_BARLABELFILENEXISTS, pFileName);
	while(fgets(buf, sizeof(buf), f)) {
		char   temp[512];
		BarcodeFormatToken tok;
		line_no++;
		char * s = strip(chomp(buf));
		//
		// Skip comments (//)
		//
		for(char * p_c = s; p_c[0] != 0; p_c++)
			if(p_c[0] == '/' && p_c[1] == '/') {
				p_c[0] = 0;
				break;
			}
		if(*s == '.') {
			THROW(zone == 1);
			zone = 2;
		}
		else {
			tok = NextToken(&s, temp, sizeof(temp));
			if(tok == tokLabel) {  // "LABEL name {"
				tok = NextToken(&s, temp, sizeof(temp));
				THROW(tok == tokIdent);
				skip_label = (stricmp(temp, pFormatName) == 0) ? 0 : 1;
				if(!skip_label) {
					STRNSCPY(BLP.Name, temp);
					ok = 1;
				}
   	            THROW(NextToken(&s, temp, sizeof(temp)) == tokLBrace);
				zone = 1;
				continue;
			}
			else if(tok == tokLoad) {  // "LOAD name {"
				tok = NextToken(&s, temp, sizeof(temp));
				THROW(tok == tokIdent);
				skip_label = (stricmp(temp, pFormatName) == 0) ? 0 : 1;
				if(!skip_label) {
					STRNSCPY(BLP.Name, temp);
					ok = 1;
				}
   	            THROW(NextToken(&s, temp, sizeof(temp)) == tokLBrace);
				zone = 11;
				continue;
			}
			else if(tok == tokRBrace) {
				THROW(zone == 2 || zone == 11);
				zone = 0;
				if(!skip_label)
					break;
			}
			else if(!skip_label) {
				if(tok == tokHome) {
					THROW(zone == 1 && GetPoint(&s, &BLP.HomeX, &BLP.HomeY));
				}
				else if(tok == tokSize) {
					THROW(zone == 1 && GetPoint(&s, &BLP.SizeX, &BLP.SizeY));
				}
				else if(tok == tokGap) {
					THROW(zone == 1 && GetNumber(&s, &BLP.Gap));
				}
				else if(tok == tokMemory) {
					THROW(zone == 1);
					BarcodeFormatToken tok2 = NextToken(&s, temp, sizeof(temp));
					THROW(tok2 == tokIdent);
					STRNSCPY(BLP.MemModule, temp);
				}
				else if(tok == tokCodepage) {
					THROW(zone == 1);
					BarcodeFormatToken tok2 = NextToken(&s, temp, sizeof(temp));
					THROW(tok2 == tokIdent);
					STRNSCPY(BLP.Codepage, temp);
				}
				else if(tok == tokText) {
					THROW(zone == 2 && GetText(0, &s));
				}
				else if(tok == tokWrap) {
					THROW(zone == 2 && GetText(1, &s));
				}
				else if(tok == tokBarcode) {
			   		THROW(zone == 2 && GetBarcode(0, &s));
				}
				else if(tok == tokSerial) {
				   	THROW(zone == 2 && GetBarcode(1, &s));
				}
				else if(tok == tokSerXNom) {
					THROW(zone == 2 && GetBarcode(2, &s));
				}
				else if(tok == tokString) {
					BarcodeLabelEntry ent;
					THROW(zone == 11);
					MEMSZERO(ent);
					ent.Type = BarcodeLabelEntry::etText;
					ent.Text = temp;
					AddEntry(&ent);
				}
				else if(tok == tokFile) {
					BarcodeLabelEntry ent;
					THROW(zone == 11);
					MEMSZERO(ent);
					ent.Type = BarcodeLabelEntry::etFile;
					THROW(NextToken(&s, temp, sizeof(temp)) == tokString);
					ent.Text = temp;
					AddEntry(&ent);
				}
			}
		}
	}
	CATCHZOK
	SFile::ZClose(&f);
	return ok;
}
//
// BarcodeLabelPrinter
//

//static
BarcodeLabelPrinter * SLAPI BarcodeLabelPrinter::CreateInstance(/*PPID printerTypeID*/const PPBarcodePrinter & rPrnPack)
{
	if(rPrnPack.PrinterType == PPBCPT_ZEBRA)
		return new ZebraLabelPrinter(rPrnPack);
	else if(rPrnPack.PrinterType == PPBCPT_DATAMAX)
		return new DatamaxLabelPrinter(rPrnPack);
	else if(rPrnPack.PrinterType == PPBCPT_ELTRON)
		return new EltronLabelPrinter(rPrnPack);
	else if(rPrnPack.PrinterType == PPBCPT_WINDOWS)
		return new WindowsLabelPrinter(rPrnPack);
	else
		return (PPSetError(PPERR_INVBCPTYPE), (BarcodeLabelPrinter*)0);
}

//#define BLPF_PRINTALL   0x0001L // Печать всей выборки

SLAPI BarcodeLabelPrinter::BarcodeLabelPrintParam::BarcodeLabelPrintParam() : PrinterID(0), NumCopies(0), Pad(0), LocID(0), Flags(0)
{
}

static int SLAPI EditBarcodeLabelPrintParam(BarcodeLabelPrinter::BarcodeLabelPrintParam * pParam, int isExtDlg)
{
	class PrintBarcodeLabelDialog : public TDialog {
	public:
		PrintBarcodeLabelDialog(int isExtDlg) : TDialog(isExtDlg ? DLG_BCPLABEL_EXT : DLG_BCPLABEL)
		{
			SetCtrlBitmap(CTL_BCPLABEL_BMP, BM_BARCODEPRINTER);
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCbSelected(CTLSEL_BCPLABEL_PRINTER)) {
				PPObjBarcodePrinter bcp_obj;
				PPBarcodePrinter rec;
				PPID   printer_id = getCtrlLong(CTLSEL_BCPLABEL_PRINTER);
				if(bcp_obj.GetPacket(printer_id, &rec) > 0)
					if(rec.PortEx.NotEmptyS())
						setCtrlString(CTL_BCPLABEL_PORT, rec.PortEx);
			}
			else if(event.isClusterClk(CTL_BCPLABEL_HOW)) {
				int16  num_copies;
				ushort v = getCtrlUInt16(CTL_BCPLABEL_HOW);
				setCtrlData(CTL_BCPLABEL_COUNT, &(num_copies = v ? 0 : 1));
				disableCtrl(CTL_BCPLABEL_COUNT, v);
			}
			else
				return;
			clearEvent(event);
		}
	};
	int    ok = -1, valid_data = 0;
	PrintBarcodeLabelDialog * dlg = new PrintBarcodeLabelDialog(isExtDlg);
	if(CheckDialogPtrErr(&dlg)) {
		SetupPPObjCombo(dlg, CTLSEL_BCPLABEL_PRINTER, PPOBJ_BCODEPRINTER, pParam->PrinterID, 0);
		dlg->setCtrlString(CTL_BCPLABEL_PORT, pParam->Port);
		dlg->setCtrlData(CTL_BCPLABEL_COUNT, &pParam->NumCopies);
		SetupPPObjCombo(dlg, CTLSEL_BCPLABEL_LOC, PPOBJ_LOCATION, pParam->LocID, 0);
		dlg->setCtrlUInt16(CTL_BCPLABEL_HOW, BIN(pParam->Flags & BarcodeLabelPrinter::BarcodeLabelPrintParam::fPrintAll));
		dlg->AddClusterAssoc(CTL_BCPLABEL_FLAGS, 0, BarcodeLabelPrinter::BarcodeLabelPrintParam::fQttyAsPack);
		dlg->AddClusterAssoc(CTL_BCPLABEL_FLAGS, 1, BarcodeLabelPrinter::BarcodeLabelPrintParam::fInteractive); // @9.6.6
		dlg->SetClusterData(CTL_BCPLABEL_FLAGS, pParam->Flags);
		dlg->selectCtrl(pParam->PrinterID ? CTL_BCPLABEL_COUNT : CTL_BCPLABEL_PRINTER);
		while(!valid_data && ExecView(dlg) == cmOK) {
			dlg->getCtrlData(CTLSEL_BCPLABEL_PRINTER, &pParam->PrinterID);
			dlg->getCtrlData(CTLSEL_BCPLABEL_LOC,     &pParam->LocID);
			dlg->getCtrlString(CTL_BCPLABEL_PORT, pParam->Port);
			dlg->getCtrlData(CTL_BCPLABEL_COUNT, &pParam->NumCopies);
			SETFLAG(pParam->Flags, BarcodeLabelPrinter::BarcodeLabelPrintParam::fPrintAll, dlg->getCtrlUInt16(CTL_BCPLABEL_HOW));
			dlg->GetClusterData(CTL_BCPLABEL_FLAGS, &pParam->Flags);
			if(pParam->PrinterID == 0)
				PPErrorByDialog(dlg, CTLSEL_BCPLABEL_PRINTER, PPERR_LABELPRNIDNEEDED);
			else
				valid_data = 1;
			ok = 1;
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

//static
int SLAPI BarcodeLabelPrinter::UpLoad(PPID prnID, const char * pLoadName, int silent)
{
	int    ok = 1;
	SFile  f;
	PPBarcodePrinter rec;
	BarcodeLabelPrinter * p_prn = 0;
	PPObjBarcodePrinter bcpobj;
	BarcodeLabelPrintParam bclpp;
	GetComDvcSymb(comdvcsCom, 1, 0, bclpp.Port);
	bclpp.PrinterID = NZOR(prnID, bcpobj.GetSingle());
	if(bclpp.PrinterID) {
		if(bcpobj.GetPacket(bclpp.PrinterID, &rec) > 0)
			bclpp.Port = rec.PortEx;
		else
			bclpp.PrinterID = 0;
		uint   i;
		SString file_name, file_path;
		PPIniFile ini_file;
		BarcodeLabel label;
		THROW(bcpobj.GetPacket(bclpp.PrinterID, &rec) > 0);
		ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_LABELFILE, file_name);
		if(file_name.Empty())
			PPGetFileName(PPFILNAM_BARLABEL_LBL, file_name);
		PPGetFilePath(PPPATH_BIN, file_name, file_path);
		THROW_PP(fileExists(file_path), PPERR_BARLABELFILENEXISTS);
		THROW_PP(label.ParseFormat(0, file_path, pLoadName), PPERR_LABELSYNTX);
		THROW(p_prn = BarcodeLabelPrinter::CreateInstance(rec/*.PrinterType*/));
		for(i = 0; i < label.GetEntryCount(); i++) {
			const BarcodeLabelEntry * p_entry = label.GetEntry(i);
			if(p_entry->Type == BarcodeLabelEntry::etText)
				p_prn->PutStr(p_entry->Text);
			else if(p_entry->Type == BarcodeLabelEntry::etFile) {
				STempBuffer buf(0);
				uint32 sz = 0;
				THROW_SL(fileExists(p_entry->Text));
				THROW_SL(f.Open(p_entry->Text, SFile::mRead));
				f.Seek(0, SEEK_END);
				sz = (uint32)f.Tell();
				f.Seek(0, SEEK_SET);
				THROW_MEM(buf.Alloc(sz));
				THROW_SL(f.Read(buf, buf.GetSize()));
				p_prn->Buf.Write(buf, buf.GetSize());
			}
		}
		THROW(p_prn->PrintLabel(bclpp.Port, &rec.Cpp));
	}
	else
		ok = -1;
	CATCH
		if(!silent)
			PPError();
		ok = 0;
	ENDCATCH
	delete p_prn;
	return ok;
}

// static
int BarcodeLabelPrinter::LoadFonts(PPID prnID, int silent)
{
	int    ok = -1;
	PPID   printer_id = prnID;
	if(!printer_id)
		ok = ListBoxSelDialog(PPOBJ_BCODEPRINTER, &printer_id, 0);
	if(ok > 0 && printer_id) {
		PPWait(1);
		ok = BarcodeLabelPrinter::UpLoad(printer_id, "FONTS", silent);
		PPWait(0);
	}
	return ok;
}

// static
int SLAPI BarcodeLabelPrinter::PrintGoodsLabel(PPID goodsID)
{
	int    ok = -1;
	RetailGoodsInfo rgi;
	PPObjGoods gobj;
	if(gobj.GetRetailGoodsInfo(goodsID, 0, &rgi))
		ok = BarcodeLabelPrinter::PrintGoodsLabel2(&rgi, 0, 0);
	return ok;
}

// static
int SLAPI BarcodeLabelPrinter::PrintLotLabel(PPID lotID)
{
	int    ok = -1;
	RetailGoodsInfo rgi;
	if(BillObj->GetLabelLotInfo(lotID, &rgi) > 0)
		ok = BarcodeLabelPrinter::PrintGoodsLabel2(&rgi, 0, 0);
	return ok;
}

// static
int SLAPI BarcodeLabelPrinter::PrintGoodsLabel__(RetailGoodsInfo * pRgi, PPID prnID, int silent)
{
	int    ok = 1;
	PPBarcodePrinter rec;
	BarcodeLabelPrinter * p_prn = 0;
	PPObjBarcodePrinter bcpobj;
	BarcodeLabelPrintParam bclpp;
	GetComDvcSymb(comdvcsCom, 1, 0, bclpp.Port);
	bclpp.PrinterID = NZOR(prnID, bcpobj.GetSingle());
	bclpp.LocID = pRgi->LocID;
	if(bclpp.PrinterID)
		if(bcpobj.GetPacket(bclpp.PrinterID, &rec) > 0)
			bclpp.Port = rec.PortEx;
		else
			bclpp.PrinterID = 0;
	bclpp.NumCopies = 1;
	if(pRgi->LabelCount > 0 && pRgi->LabelCount < 1000)
		bclpp.NumCopies = pRgi->LabelCount;
	if(silent || EditBarcodeLabelPrintParam(&bclpp, 0) > 0) {
		uint   i;
		SString file_name, file_path;
		PPIniFile ini_file;
		BarcodeLabel label;
		THROW(bcpobj.GetPacket(bclpp.PrinterID, &rec) > 0);
		if(pRgi->LocID != bclpp.LocID) {
			RetailGoodsInfo temp_rgi;
			PPObjGoods gobj;
			if(gobj.GetRetailGoodsInfo(pRgi->ID, bclpp.LocID, &temp_rgi) > 0) {
				pRgi->Price = temp_rgi.Price;
				pRgi->Expiry = temp_rgi.Expiry;
			}
		}
		ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_LABELFILE, file_name);
		if(file_name.Empty())
			PPGetFileName(PPFILNAM_BARLABEL_LBL, file_name);
		PPGetFilePath(PPPATH_BIN, file_name, file_path);
		THROW_PP_S(fileExists(file_path), PPERR_BARLABELFILENEXISTS, file_path);
		THROW_PP(label.ParseFormat(pRgi, file_path, rec.LabelName), PPERR_LABELSYNTX);
		THROW(p_prn = BarcodeLabelPrinter::CreateInstance(rec/*.PrinterType*/));
		label.SetBarcodeWidth(rec.BcNarrowPt, rec.BcWidePt); // @v8.0.9
		THROW(p_prn->StartLabel(label.GetParam(), bclpp.NumCopies));
		for(i = 0; i < label.GetEntryCount(); i++)
			THROW(p_prn->PutDataEntry(label.GetEntry(i)));
		THROW(p_prn->EndLabel());
		// @vmiller {
		SString str = bclpp.Port;
		if(!str.CmpPrefix("usb", 1))
			THROW(p_prn->PrintLabelUsb(rec.PrinterType))
		else
		// } @vmiller
			THROW(p_prn->PrintLabel(bclpp.Port, &rec.Cpp));
	}
	else
		ok = -1;
	CATCH
		if(!silent)
			PPError();
		ok = 0;
	ENDCATCH
	delete p_prn;
	return ok;
}

// static
int SLAPI BarcodeLabelPrinter::PrintGoodsLabel2(RetailGoodsInfo * pRgi, PPID prnID, int silent)
{
	int    ok = 1;
	PPBarcodePrinter rec;
	BarcodeLabelPrinter * p_prn = 0;
	PPObjBarcodePrinter bcpobj;
	BarcodeLabelPrintParam bclpp;
	GetComDvcSymb(comdvcsCom, 1, 0, bclpp.Port);
	bclpp.PrinterID = NZOR(prnID, bcpobj.GetSingle());
	bclpp.LocID = pRgi->LocID;
	if(bclpp.PrinterID)
		if(bcpobj.GetPacket(bclpp.PrinterID, &rec) > 0)
			bclpp.Port = rec.PortEx;
		else
			bclpp.PrinterID = 0;
	bclpp.NumCopies = 1;
	if(pRgi->LabelCount > 0 && pRgi->LabelCount < 1000)
		bclpp.NumCopies = pRgi->LabelCount;
	if(silent || EditBarcodeLabelPrintParam(&bclpp, 0) > 0) {
		//uint   i;
		//SString file_name, file_path;
		//PPIniFile ini_file;
		//BarcodeLabel label;
		THROW(bcpobj.GetPacket(bclpp.PrinterID, &rec) > 0);
		if(pRgi->LocID != bclpp.LocID) {
			RetailGoodsInfo temp_rgi;
			PPObjGoods gobj;
			if(gobj.GetRetailGoodsInfo(pRgi->ID, bclpp.LocID, &temp_rgi) > 0) {
				pRgi->Price = temp_rgi.Price;
				pRgi->Expiry = temp_rgi.Expiry;
			}
		}

		{
			TSCollection <RetailGoodsInfo> rgi_list;
			RetailGoodsInfo * p_rgi = rgi_list.CreateNewItem();
			THROW_SL(p_rgi);
			*p_rgi = *pRgi;
			p_rgi->LabelCount = bclpp.NumCopies;
			{
				THROW(p_prn = BarcodeLabelPrinter::CreateInstance(rec/*.PrinterType*/));
				THROW(p_prn->Helper_PrintRgiCollection(bclpp, rgi_list));
			}
		}
		//ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_LABELFILE, file_name);
		//if(file_name.Empty())
		//	PPGetFileName(PPFILNAM_BARLABEL_LBL, file_name);
		//PPGetFilePath(PPPATH_BIN, file_name, file_path);
		//THROW_PP_S(fileExists(file_path), PPERR_BARLABELFILENEXISTS, file_path);
		//THROW_PP(label.ParseFormat(pRgi, file_path, rec.LabelName), PPERR_LABELSYNTX);
		//THROW(p_prn = BarcodeLabelPrinter::CreateInstance(rec/*.PrinterType*/));
		//label.SetBarcodeWidth(rec.BcNarrowPt, rec.BcWidePt); // @v8.0.9
		//THROW(p_prn->StartLabel(label.GetParam(), bclpp.NumCopies));
		//for(i = 0; i < label.GetEntryCount(); i++)
		//	THROW(p_prn->PutDataEntry(label.GetEntry(i)));
		//THROW(p_prn->EndLabel());
		// @vmiller {
		//SString str = bclpp.Port;
		//if(!str.CmpPrefix("usb", 1))
		//	THROW(p_prn->PrintLabelUsb(rec.PrinterType))
		//else
		// } @vmiller
		//	THROW(p_prn->PrintLabel(bclpp.Port, &rec.Cpp));
	}
	else
		ok = -1;
	CATCH
		if(!silent)
			PPError();
		ok = 0;
	ENDCATCH
	delete p_prn;
	return ok;
}

int SLAPI BarcodeLabelPrinter::Helper_PrintRgiCollection(const BarcodeLabelPrintParam & rBclpp, TSCollection <RetailGoodsInfo> & rList)
{
	int    ok = 1;
	PPObjGoods gobj;
	SString temp_buf;
	{
		if(PrnPack.PrinterType == PPBCPT_WINDOWS) {
			uint   rpt_id = REPORT_BARCODELABELLIST;
			SString loc_prn_port = PrnPack.PortEx.NotEmptyS() ? PrnPack.PortEx.cptr() : 0;
			loc_prn_port.Strip();
			PView  pv(&rList);
			PPReportEnv env;
			if(!(rBclpp.Flags & rBclpp.fInteractive))
				env.PrnFlags = SReport::PrintingNoAsk;
			if(loc_prn_port.NotEmpty()) {
				DS.GetTLA().PrintDevice = loc_prn_port;
			}
			PPAlddPrint(rpt_id, &pv, &env);
		}
		else {
			SString file_name, file_path;
			uint   part_count = 0;
			int    row_delay = 0;
			PPIniFile ini_file;
			ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_LABELFILE, file_name);
			ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_BARLABELPRINT_DELAY, &row_delay);
			if(row_delay <= 0 || row_delay > 10000)
				row_delay = 250;
			if(file_name.Empty())
				PPGetFileName(PPFILNAM_BARLABEL_LBL, file_name);
			PPGetFilePath(PPPATH_BIN, file_name, file_path);
			THROW_PP(fileExists(file_path), PPERR_BARLABELFILENEXISTS);
			for(uint i = 0; i < rList.getCount(); i++) {
				const RetailGoodsInfo * p_rgi = rList.at(i);
				if(p_rgi) {
					int16  num_copies = p_rgi->LabelCount;
					if(num_copies > 0) {
						BarcodeLabel label__;
						THROW_PP(label__.ParseFormat(p_rgi, file_path, PrnPack.LabelName), PPERR_LABELSYNTX);
						//THROW(p_prn = BarcodeLabelPrinter::CreateInstance(rec/*.PrinterType*/));
						label__.SetBarcodeWidth(PrnPack.BcNarrowPt, PrnPack.BcWidePt);
						{
							THROW(StartLabel(label__.GetParam(), num_copies));
							for(uint i = 0; i < label__.GetEntryCount(); i++) {
								THROW(PutDataEntry(label__.GetEntry(i)));
							}
							THROW(EndLabel());
							// @vmiller {
							temp_buf = rBclpp.Port;
							if(!temp_buf.CmpPrefix("usb", 1))
								THROW(PrintLabelUsb(PrnPack.PrinterType))
							else
							// } @vmiller
								THROW(PrintLabel(rBclpp.Port, &PrnPack.Cpp));
						}
						//
						// При печати большого количества этикеток принтер не успевает обрабатывать запросы
						// Из-за этого делаем задержку между отдельными строками (между каждыми 10-ю строками
						// задержка больше)
						//
						if(++part_count >= 10) {
							SDelay(2000);
							part_count = 0;
						}
						else
							SDelay(row_delay);
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

// static
int SLAPI BarcodeLabelPrinter::PrintLabelByBill2(const PPBillPacket * pPack, uint pos)
{
	int    ok = 1;
	PPObjBill * p_bobj = BillObj;
	TSCollection <RetailGoodsInfo> rgi_list;
	PPID   cor_loc_id = 0;
	PPBarcodePrinter rec;
	BarcodeLabelPrinter * p_prn = 0;
	PPObjBarcodePrinter bcpobj;
	BarcodeLabelPrintParam bclpp;
	GetComDvcSymb(comdvcsCom, 1, 0, bclpp.Port);
	bclpp.PrinterID = bcpobj.GetSingle();
	if(IsIntrExpndOp(pPack->Rec.OpID))
		bclpp.LocID = cor_loc_id = PPObjLocation::ObjToWarehouse(pPack->Rec.Object);
	SETIFZ(bclpp.LocID, pPack->Rec.LocID);
	if(bclpp.PrinterID && bcpobj.GetPacket(bclpp.PrinterID, &rec) > 0)
		bclpp.Port = rec.PortEx;
	bclpp.NumCopies = 1;
	if(EditBarcodeLabelPrintParam(&bclpp, 1) > 0 && bcpobj.GetPacket(bclpp.PrinterID, &rec) > 0) {
		uint   cur_pos, end_pos;
		uint   part_count = 0;
		SString serial;
		PPObjGoods gobj;
		if(bclpp.Flags & BarcodeLabelPrintParam::fPrintAll) {
			cur_pos = 0;
			end_pos = pPack->GetTCount() - 1;
		}
		else
			cur_pos = end_pos = pos;
		for(; cur_pos <= end_pos; cur_pos++) {
			int16  num_copies = 1;
			RetailGoodsInfo * p_rgi = rgi_list.CreateNewItem();
			THROW_SL(p_rgi);
			const PPTransferItem * p_ti = &pPack->ConstTI(cur_pos);
			int    r = 0;
			if(p_ti->LotID) {
				PPID   lot_id = p_ti->LotID;
				if(cor_loc_id && p_ti->BillID && p_ti->RByBill) {
					TransferTbl::Rec trfr_rec;
					if(p_bobj->trfr->SearchByBill(p_ti->BillID, 1, p_ti->RByBill, &trfr_rec) > 0)
						lot_id = trfr_rec.LotID;
				}
				THROW(p_bobj->GetLabelLotInfo(lot_id, p_rgi) > 0);
				if(pPack->OpTypeID == PPOPT_GOODSREVAL)
					p_rgi->RevalPrice = p_ti->Discount;
				r = 1;
			}
			else {
				r = gobj.GetRetailGoodsInfo(p_ti->GoodsID, p_ti->LocID, p_rgi);
				SETIFZ(p_rgi->Cost, p_ti->Cost);
				SETIFZ(p_rgi->Price, p_ti->Price);
				p_rgi->RevalPrice = p_rgi->Price;
				if(p_rgi->Serial[0] == 0) {
					pPack->SnL.GetNumber(cur_pos, &serial);
					serial.CopyTo(p_rgi->Serial, sizeof(p_rgi->Serial));
				}
			}
			//
			// Цены поступления и реализации из строки документа
			//
			p_rgi->LineCost = p_ti->Cost;
			p_rgi->LinePrice = p_ti->Price;
			if(r > 0) {
				const double qtty = fabs(p_ti->SQtty(pPack->Rec.OpID));
				if(bclpp.Flags & BarcodeLabelPrintParam::fPrintAll) {
					GoodsStockExt gse;
					num_copies = (int16)R0i(qtty + 0.49);
					if(bclpp.Flags & BarcodeLabelPrintParam::fQttyAsPack) {
						if(p_ti->UnitPerPack > 0)
							num_copies = (int16)R0i(qtty / p_ti->UnitPerPack);
						else if(gobj.GetStockExt(p_ti->GoodsID, &gse) > 0 && gse.Package > 0)
							num_copies = (int16)R0i(qtty / gse.Package);
					}
					if(num_copies <= 0)
						num_copies = 1;
				}
				else
					num_copies = bclpp.NumCopies;
				p_rgi->LabelCount = num_copies;
				if(p_rgi->LocID != bclpp.LocID) {
					RetailGoodsInfo temp_rgi;
					if(gobj.GetRetailGoodsInfo(p_rgi->ID, bclpp.LocID, &temp_rgi) > 0) {
						p_rgi->Price  = temp_rgi.Price;
						p_rgi->Expiry = temp_rgi.Expiry;
					}
				}
			}
		}
		{
			THROW(p_prn = BarcodeLabelPrinter::CreateInstance(rec/*.PrinterType*/));
			THROW(p_prn->Helper_PrintRgiCollection(bclpp, rgi_list));
		}
	}
	CATCHZOKPPERR
	delete p_prn;
	return ok;
}

// static
int SLAPI BarcodeLabelPrinter::PrintLabelByBill__(PPBillPacket * pPack, uint pos)
{
	int    ok = 1;
	PPObjBill * p_bobj = BillObj;
	PPID   cor_loc_id = 0;
	PPBarcodePrinter rec;
	BarcodeLabelPrinter * p_prn = 0;
	PPObjBarcodePrinter bcpobj;
	BarcodeLabelPrintParam bclpp;
	GetComDvcSymb(comdvcsCom, 1, 0, bclpp.Port);
	bclpp.PrinterID = bcpobj.GetSingle();
	if(IsIntrExpndOp(pPack->Rec.OpID))
		bclpp.LocID = cor_loc_id = PPObjLocation::ObjToWarehouse(pPack->Rec.Object);
	SETIFZ(bclpp.LocID, pPack->Rec.LocID);
	if(bclpp.PrinterID && bcpobj.GetPacket(bclpp.PrinterID, &rec) > 0)
		bclpp.Port = rec.PortEx;
	bclpp.NumCopies = 1;
	if(EditBarcodeLabelPrintParam(&bclpp, 1) > 0 && bcpobj.GetPacket(bclpp.PrinterID, &rec) > 0) {
		uint   cur_pos, end_pos;
		uint   part_count = 0;
		int    row_delay = 0;
		SString file_name, file_path, serial;
		PPObjGoods gobj;
		PPIniFile ini_file;
		ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_LABELFILE, file_name);
		ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_BARLABELPRINT_DELAY, &row_delay);
		if(row_delay <= 0 || row_delay > 10000)
			row_delay = 250;
		if(file_name.Empty())
			PPGetFileName(PPFILNAM_BARLABEL_LBL, file_name);
		PPGetFilePath(PPPATH_BIN, file_name, file_path);
		THROW_PP(fileExists(file_path), PPERR_BARLABELFILENEXISTS);
		if(bclpp.Flags & BarcodeLabelPrintParam::fPrintAll) {
			cur_pos = 0;
			end_pos = pPack->GetTCount() - 1;
		}
		else
			cur_pos = end_pos = pos;
		for(; cur_pos <= end_pos; cur_pos++) {
			int16  num_copies = 1;
			RetailGoodsInfo rgi;
			const PPTransferItem * p_ti = &pPack->ConstTI(cur_pos);
			int    r = 0;
			if(p_ti->LotID) {
				PPID   lot_id = p_ti->LotID;
				if(cor_loc_id && p_ti->BillID && p_ti->RByBill) {
					TransferTbl::Rec trfr_rec;
					if(p_bobj->trfr->SearchByBill(p_ti->BillID, 1, p_ti->RByBill, &trfr_rec) > 0)
						lot_id = trfr_rec.LotID;
				}
				THROW(p_bobj->GetLabelLotInfo(lot_id, &rgi) > 0);
				if(pPack->OpTypeID == PPOPT_GOODSREVAL)
					rgi.RevalPrice = p_ti->Discount;
				r = 1;
			}
			else {
				r = gobj.GetRetailGoodsInfo(p_ti->GoodsID, p_ti->LocID, &rgi);
				SETIFZ(rgi.Cost, p_ti->Cost);
				SETIFZ(rgi.Price, p_ti->Price);
				rgi.RevalPrice = rgi.Price;
				if(rgi.Serial[0] == 0) {
					pPack->SnL.GetNumber(cur_pos, &serial); // @v8.7.12 @fix pos-->cur_pos
					serial.CopyTo(rgi.Serial, sizeof(rgi.Serial));
				}
			}
			//
			// Цены поступления и реализации из строки документа
			//
			rgi.LineCost = p_ti->Cost;
			rgi.LinePrice = p_ti->Price;
			if(r > 0) {
				const double qtty = fabs(p_ti->SQtty(pPack->Rec.OpID));
				if(bclpp.Flags & BarcodeLabelPrintParam::fPrintAll) {
					GoodsStockExt gse;
					num_copies = (int16)R0i(qtty + 0.49);
					if(bclpp.Flags & BarcodeLabelPrintParam::fQttyAsPack) {
						if(p_ti->UnitPerPack > 0)
							num_copies = (int16)R0i(qtty / p_ti->UnitPerPack);
						else if(gobj.GetStockExt(p_ti->GoodsID, &gse) > 0 && gse.Package > 0)
							num_copies = (int16)R0i(qtty / gse.Package);
					}
					if(num_copies <= 0)
						num_copies = 1;
				}
				else
					num_copies = bclpp.NumCopies;
				if(num_copies > 0) {
					BarcodeLabel label;
					if(rgi.LocID != bclpp.LocID) {
						RetailGoodsInfo temp_rgi;
						if(gobj.GetRetailGoodsInfo(rgi.ID, bclpp.LocID, &temp_rgi) > 0) {
							rgi.Price  = temp_rgi.Price;
							rgi.Expiry = temp_rgi.Expiry;
						}
					}
					THROW_PP(label.ParseFormat(&rgi, file_path, rec.LabelName), PPERR_LABELSYNTX);
					THROW(p_prn = BarcodeLabelPrinter::CreateInstance(rec/*.PrinterType*/));
					label.SetBarcodeWidth(rec.BcNarrowPt, rec.BcWidePt); // @v8.0.9
					THROW(p_prn->StartLabel(label.GetParam(), num_copies));
					for(uint i = 0; i < label.GetEntryCount(); i++)
						THROW(p_prn->PutDataEntry(label.GetEntry(i)));
					THROW(p_prn->EndLabel());

					// @vmiller {
					SString str = bclpp.Port;
					if(!str.CmpPrefix("usb", 1))
						THROW(p_prn->PrintLabelUsb(rec.PrinterType))
					else
					// } @vmiller
						THROW(p_prn->PrintLabel(bclpp.Port, &rec.Cpp));
					ZDELETE(p_prn);
					//
					// При печати большого количества этикеток принтер не успевает обрабатывать запросы
					// Из-за этого делаем задержку между отдельными строками (между каждыми 10-ю строками
					// задержка больше)
					//
					if(++part_count >= 10) {
						SDelay(2000);
						part_count = 0;
					}
					else
						SDelay(row_delay);
				}
			}
		}
	}
	CATCHZOKPPERR
	delete p_prn;
	return ok;
}

SLAPI BarcodeLabelPrinter::BarcodeLabelPrinter(const PPBarcodePrinter & rPrnPack) : PrnPack(rPrnPack)
{
}

SLAPI BarcodeLabelPrinter::~BarcodeLabelPrinter()
{
}

int FASTCALL BarcodeLabelPrinter::PutStr(const char * pStr)
{
	return Buf.Write(pStr, strlen(pStr)) ? 1 : PPSetErrorSLib();
}

int FASTCALL BarcodeLabelPrinter::PutChr(char c)
{
	return Buf.Write(&c, 1) ? 1 : PPSetErrorSLib();
}

static char * SLAPI PutIntToBuf(char * pBuf, int n, int numDigits)
{
	return longfmtz(n, numDigits, pBuf, 0);
}

int SLAPI BarcodeLabelPrinter::PutInt(int n, int numDigits)
{
	char   buf[32];
	return PutStr(PutIntToBuf(buf, n, numDigits));
}

int SLAPI BarcodeLabelPrinter::PrintLabel(const char * pPort, const CommPortParams * pCpp)
{
	int    c = 0;
	int    comdvcs = IsComDvcSymb(pPort, &c);
	int    ok = 1;
	ulong  written_bytes = 0;
	HANDLE hdl = INVALID_HANDLE_VALUE;
	if(comdvcs) {
		SString name;
		GetComDvcSymb(comdvcs, c, 1, name);
		hdl = ::CreateFile(name, GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0); // @unicodeproblem
		PPSetAddedMsgString(name);
		SLS.SetAddedMsgString(name);
	}
	else {
		hdl = ::CreateFile(pPort, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0); // @unicodeproblem
		PPSetAddedMsgString(pPort);
		SLS.SetAddedMsgString(pPort);
	}
	SLibError = SLERR_OPENFAULT;
	THROW_SL(hdl != INVALID_HANDLE_VALUE);
	if(comdvcs == comdvcsCom) {
		DCB    dcb;
		GetCommState(hdl, &dcb);
		if(pCpp->Cbr == cbr19200)
			dcb.BaudRate = CBR_19200;
		else
			dcb.BaudRate = CBR_9600;
		if(pCpp->Parity == 1)
			dcb.Parity = ODDPARITY;
		else if(pCpp->Parity == 2)
			dcb.Parity = EVENPARITY;
		else
			dcb.Parity   = NOPARITY;
		if(pCpp->ByteSize == 7)
			dcb.ByteSize = 7;
		else
			dcb.ByteSize = 8;
		dcb.StopBits = ONESTOPBIT;
		SetCommState(hdl, &dcb);
	}
	else if(oneof2(comdvcs, comdvcsPrn, comdvcsLpt)) {
		COMMTIMEOUTS cto;
		if(GetCommTimeouts(hdl, &cto)) {
			cto.WriteTotalTimeoutConstant = 2000;
			SetCommTimeouts(hdl, &cto);
		}
		SLS.SetOsError();
	}
	{
		size_t offs = Buf.GetRdOffs();
		size_t size = Buf.GetAvailableSize();
		WriteFile(hdl, Buf.GetBuf(offs), (DWORD)size, &written_bytes, 0);
		Buf.Clear();
		THROW_PP(written_bytes, PPERR_BARLABELPRINTFAULT);
	}
	CATCHZOK
	if(hdl != INVALID_HANDLE_VALUE)
		CloseHandle(hdl);
	return ok;
}

// @vmiller
int SLAPI BarcodeLabelPrinter::PrintLabelUsb(PPID devType)
{
	int    ok = 1, got_dev = 0;
	uint   i = 0;
	SUsbDevice usb;
	TSCollection <SUsbDevice> usb_list;
	THROW(SUsbDevice::GetDeviceList(usb_list) != -1);
	for(i = 0; i < usb_list.getCount(); i ++) {
		if(devType == PPBCPT_ZEBRA)
			continue;
		else if(devType == PPBCPT_DATAMAX)
			continue;
		else if(devType == PPBCPT_ELTRON) {
			for(uint j = 0; j < SIZEOFARRAY(EltronUsbOpt); j++) {
				if(usb_list.at(i)->IsDev(EltronUsbOpt[j].P_Vid, EltronUsbOpt[j].P_Pid)) {
					usb = *usb_list.at(i);
					got_dev = 1;
					break;
				}
			}
		}
	}
    //SLibError = SLERR_OPENFAULT;
	if(got_dev) {
		THROW(usb.Open());
		{
			size_t offs = Buf.GetRdOffs();
			size_t size = Buf.GetAvailableSize();
			THROW_PP(usb.Write(Buf.GetBuf(offs), size), PPERR_BARLABELPRINTFAULT);
		}
	}
	CATCHZOK
	usb.Close();
	return ok;
}

//
// DatamaxLabelPrinter
//
struct BarCStdToDatamaxEntry {
	int8   Std;
	int8   Chr;
};

static BarCStdToDatamaxEntry _BarCStdTab[] = {
	{BARCSTD_CODE11,        0},
	{BARCSTD_INTRLVD2OF5, 'D'},
	{BARCSTD_CODE39,      'A'},
	{BARCSTD_CODE49,        0},
	{BARCSTD_PDF417,      'z'},
	{BARCSTD_EAN8,        'G'},
	{BARCSTD_UPCE,        'C'},
	{BARCSTD_CODE93,      'O'},
	{BARCSTD_CODE128,     'E'},
	{BARCSTD_EAN13,       'F'},
	{BARCSTD_IND2OF5,     'L'},
	{BARCSTD_STD2OF5,     'J'},
	{BARCSTD_ANSI,         0},
	{BARCSTD_LOGMARS,      0},
	{BARCSTD_MSI,          0},
	{BARCSTD_PLESSEY,     'K'},
	{BARCSTD_UPCEAN2EXT,  'M'},
	{BARCSTD_UPCEAN5EXT,  'N'},
	{BARCSTD_UPCA,        'B'},
	{BARCSTD_POSTNET,     'p'}
};

int SLAPI DatamaxLabelPrinter::PutDataEntry(const BarcodeLabelEntry * pEntry)
{
	char   buf[512];
	size_t i = 0, j;
	char   c;
	int    rotation = pEntry->Rotation % 360;
	int    font_id = atoi(pEntry->Font);
	if((rotation > 315 && rotation < 360) || (rotation >= 0 && rotation <= 45))
		c = '1';
	else if(rotation > 45 && rotation <= 135)
		c = '2';
	else if(rotation > 135 && rotation <= 225)
		c = '3';
	else if(rotation > 225 && rotation <= 315)
		c = '4';
	else
		c = '1';
	buf[i++] = c;
	if(pEntry->Type == BarcodeLabelEntry::etText)
		buf[i++] = (font_id >= 1 && font_id <= 8) ? ('0' + font_id) : '9';
	else {
		c = 0;
		for(j = 0; j < SIZEOFARRAY(_BarCStdTab); j++)
			if(_BarCStdTab[j].Std == pEntry->BarcodeStd) {
				c = (char)_BarCStdTab[j].Chr;
				break;
			}
		if(c == 0)
			return PPSetError(PPERR_BARCSTDNSUPPORT);
		buf[i++] = c;
	}
	if(pEntry->XMult >= 0 && pEntry->XMult < 10)
		buf[i++] = '0' + pEntry->XMult; // Width multiplier
	else
		buf[i++] = '1';
	if(pEntry->YMult >= 0 && pEntry->YMult < 10)
		buf[i++] = '0' + pEntry->YMult; // Height multiplier
	else
		buf[i++] = '1';
	if(pEntry->Type == BarcodeLabelEntry::etText) {
		if(font_id >= 1 && font_id <= 8)
			PutIntToBuf(buf+i, (int)0, 3);
		else {
			if(font_id == 0) {
				char   font_buf[8];
				memzero(font_buf, sizeof(font_buf));
				strncpy(font_buf, pEntry->Font, 3);
				size_t j = strlen(font_buf);
				if(j < 3)
					padright(font_buf, '0', 3-j);
				memcpy(buf+i, font_buf, 3);
			}
			else
				PutIntToBuf(buf+i, font_id, 3);
		}
	}
	else
		PutIntToBuf(buf+i, pEntry->BarcodeHeight, 3);
	i += 3;
	PutIntToBuf(buf+i, pEntry->YPos, 4);
	i += 4;
	PutIntToBuf(buf+i, pEntry->XPos, 4);
	i += 4;
	if(pEntry->Type == BarcodeLabelEntry::etText) {
		if(pEntry->FontHeight > 0 || pEntry->FontWidth > 0) {
			buf[i++] = 'P';
			PutIntToBuf(buf+i, pEntry->FontHeight, 3);
			i += 3;
			buf[i++] = 'P';
			PutIntToBuf(buf+i, pEntry->FontWidth, 3);
			i += 3;
		}
	}
	strnzcpy(buf+i, pEntry->Text, sizeof(buf)-i);
	i += strlen(buf+i);
	buf[i++] = '\r';
	buf[i++] = 0;
	return PutStr(buf);
}

int SLAPI DatamaxLabelPrinter::StartLabel(const BarcodeLabelParam * pParam, int numCopies)
{
	char   buf[256];
	if(pParam->MemModule[0]) {
		size_t p = 0;
		buf[p++] = 2;
		buf[p++] = 'X';
		p += strlen(strcpy(buf+p, pParam->MemModule));
		buf[p++] = '\r';
		buf[p] = 0;
		PutStr(buf);
	}
	NumCopies = numCopies;
	buf[0] = 2;
	buf[1] = 'L';
	buf[2] = '\r';
	buf[3] = 0;
	PutStr(buf);
	PutChr('D');
	PutInt(1, 0);
	PutInt(1, 0);
	PutChr('\r');
	// Select single-byte codepage ('CP' - Cyr) "ySCP"
	buf[0] = 'y';
	buf[1] = 'S';
	buf[2] = 'C';
	buf[3] = 'P';
	buf[4] = '\r';
	buf[5] = 0;
	PutStr(buf);
	return 1;
}

int SLAPI DatamaxLabelPrinter::EndLabel()
{
	char   buf[256];
	if(NumCopies > 1) {
		char * p = buf;
		*p++ = 'Q';
		p += strlen(PutIntToBuf(p, NumCopies, 4));
		*p++ = '\r';
		*p++ = 0;
		PutStr(buf);
	}
	buf[0] = 'E';
	buf[1] = '\r';
	buf[2] = 0;
	return PutStr(buf);
}
//
// ZebraLabelPrinter
//
#define ZPL_XA 0x4158U
#define ZPL_XZ 0x5A58U
#define ZPL_LH 0x484CU
#define ZPL_FO 0x4F46U
#define ZPL_FS 0x5346U
#define ZPL_FD 0x4446U
#define ZPL_B  0x0042U
#define ZPL_PQ 0x5150U

int SLAPI ZebraLabelPrinter::PutCtrl(uint16 code)
{
	int    i = 0;
	char   buf[32];
	char * cc = (char*)&code;
	if(*cc) {
		buf[i++] = '^';
		buf[i++] = *cc++;
		if(*cc)
			buf[i++] = *cc++;
		buf[i++] = 0;
		PutStr(buf);
	}
	return 1;
}

int SLAPI ZebraLabelPrinter::PutPosition(uint16 ctrl, int x, int y)
{
	PutCtrl(ctrl);
	PutInt(x, 0);
	PutChr(',');
	PutInt(y, 0);
	return 1;
}

int SLAPI ZebraLabelPrinter::StartLabel(const BarcodeLabelParam * param, int numCopies)
{
	NumCopies = numCopies;
	PutCtrl(ZPL_XA);   // ^XA
	PutChr('\n');
	PutPosition(ZPL_LH, param->HomeX, param->HomeY);
	PutChr('\n');
	return 1;
}

int SLAPI ZebraLabelPrinter::EndLabel()
{
	if(NumCopies > 1) {
		PutCtrl(ZPL_PQ);
		PutInt(NumCopies, 0);
		PutChr('\n');
	}
	PutCtrl(ZPL_XZ);
	return 1;
}

struct BarCStdToZebraEntry {
	int8   Std;
	int8   Chr;
};

static BarCStdToZebraEntry _Z_BarCStdTab[] = {
	{BARCSTD_CODE11,      '1'},
	{BARCSTD_INTRLVD2OF5, '2'},
	{BARCSTD_CODE39,      '3'},
	{BARCSTD_CODE49,      '4'},
	{BARCSTD_PDF417,      '7'},
	{BARCSTD_EAN8,        '8'},
	{BARCSTD_UPCE,        '9'},
	{BARCSTD_CODE93,      'A'},
	{BARCSTD_CODE128,     'C'},
	{BARCSTD_EAN13,       'E'},
	{BARCSTD_IND2OF5,     'I'},
	{BARCSTD_STD2OF5,     'J'},
	{BARCSTD_ANSI,        'K'},
	{BARCSTD_LOGMARS,     'L'},
	{BARCSTD_MSI,         'M'},
	{BARCSTD_PLESSEY,     'P'},
	{BARCSTD_UPCEAN2EXT,  'S'},
	{BARCSTD_UPCEAN5EXT,  'S'},
	{BARCSTD_UPCA,        'U'},
	{BARCSTD_POSTNET,     'Z'}
};

int SLAPI ZebraLabelPrinter::PutDataEntry(const BarcodeLabelEntry * pEntry)
{
	int    c = 0;
	PutPosition(ZPL_FO, pEntry->XPos, pEntry->YPos);
	if(pEntry->Type == BarcodeLabelEntry::etText) {
		if(pEntry->Font[0]) {
			PutChr('^');
			PutStr(pEntry->Font);
			int rot = pEntry->Rotation % 360;
			c = 0;
			if(rot == 1)
				c = 0;
			else if((rot > 315 && rot < 360) || (rot >= 0 && rot <= 45))
				c = 'N';
			else if(rot > 45 && rot <= 135)
				c = 'R';
			else if(rot > 135 && rot <= 225)
				c = 'I';
			else if(rot > 225 && rot <= 315)
				c = 'R';
			else if(rot == 0)
				c = 'N';
			if(c)
				PutChr(c);
			if(pEntry->XMult > 1) {
				PutChr(',');
				PutInt(pEntry->XMult, 0);
			}
			if(pEntry->YMult > 1) {
				PutChr(',');
				PutInt(pEntry->YMult, 0);
			}
		}
		else
			PutCtrl(0x3141); // ^A1 (font 1)
	}
	else if(pEntry->Type == BarcodeLabelEntry::etBarcode) {
		PutCtrl(ZPL_B);
		for(int j = 0; j < sizeof(_Z_BarCStdTab) / sizeof(BarCStdToZebraEntry); j++)
			if(_Z_BarCStdTab[j].Std == pEntry->BarcodeStd) {
				c = _Z_BarCStdTab[j].Chr;
				break;
			}
		if(c == 0)
			return PPSetError(PPERR_BARCSTDNSUPPORT);
		PutChr(c);
		PutChr(',');
		PutInt(pEntry->BarcodeHeight, 0);
	}
	{
		SString text = pEntry->Text;
		// @v9.2.7 {
		if(PrnPack.Cp != cpOEM)
			text.Transf(CTRANSF_INNER_TO_OUTER);
		// } @v9.2.7
		PutCtrl(ZPL_FD);
		PutStr(text/*, maxlen*/);
		PutCtrl(ZPL_FS);
		PutChr('\n');
	}
	return 1;
}
//
// EltronLabelPrinter
//
#define ZPL_XA 0x4158U
#define ZPL_XZ 0x5A58U
#define ZPL_LH 0x484CU
#define ZPL_FO 0x4F46U
#define ZPL_FS 0x5346U
#define ZPL_FD 0x4446U
#define ZPL_B  0x0042U
#define ZPL_PQ 0x5150U

int SLAPI EltronLabelPrinter::StartLabel(const BarcodeLabelParam * param, int numCopies)
{
	NumCopies = numCopies;
	BcNarrowPt = param->BcNarrowPt; // @v8.0.9
	BcWidePt = param->BcWidePt;     // @v8.0.9
	PutChr('N');   // Clear buffer
	PutChr('\n');

	PutChr('I');   // Set codepage
	PutInt(8, 0);
	PutChr(',');
	PutInt(10, 0); // Codepage 10 (Cyrillic 866)
	PutChr(',');
	PutInt(1, 3);  // Country 001 (USA)
	PutChr('\n');

	if(param->SizeX > 0) {
		PutChr('q');           // Set Label Width (dots)
		PutInt(param->SizeX, 0);
		PutChr('\n');
	}
	if(param->SizeY > 0) {
		PutChr('Q');           // Set Form Length (dots)
		PutInt(param->SizeY, 0);
		PutChr(',');
		PutInt((param->Gap > 0) ? param->Gap : 0, 0);
		PutChr('\n');
	}
	if(param->HomeX > 0 || param->HomeY > 0) {
		PutChr('R');           // Set reference point (home position)
		PutInt(param->HomeX, 0);
		PutChr(',');
		PutInt(param->HomeY, 0);
		PutChr('\n');
	}
	return 1;
}

int SLAPI EltronLabelPrinter::EndLabel()
{
	PutChr('P');   // Print
	//PutInt(1, 0);  // Number of label sets
	if(NumCopies > 1) {
		//PutChr(',');
		PutInt(NumCopies, 0);
	}
	else
		PutInt(1, 0);
	PutChr('\n');
	return 1;
}

struct BarCStdToEltronEntry {
	int8   Std;
	char * P_Str;
};

static BarCStdToEltronEntry _E_BarCStdTab[] = {
	{BARCSTD_INTRLVD2OF5, "2"},
	{BARCSTD_CODE39,      "3"},
	{BARCSTD_EAN8,        "E80"},
	{BARCSTD_UPCE,        "UE0"},
	{BARCSTD_CODE93,      "9"},
	{BARCSTD_CODE128,     "1"},
	{BARCSTD_EAN13,       "E30"},
	{BARCSTD_MSI,         "M"},
	{BARCSTD_PLESSEY,     "L"},
	{BARCSTD_UPCEAN2EXT,  "UE2"},
	{BARCSTD_UPCEAN5EXT,  "UE5"},
	{BARCSTD_UPCA,        "UA0"},
	{BARCSTD_POSTNET,     "P"}
};

int SLAPI EltronLabelPrinter::PutDataEntryPrefix(char letter, const BarcodeLabelEntry * pEntry)
{
	int    rot_c = '0';
	int    rot = pEntry->Rotation % 360;
	if(rot == 1)
		rot_c = '0';
	else if((rot > 315 && rot < 360) || (rot >= 0 && rot <= 45))
		rot_c = '0';
	else if(rot > 45 && rot <= 135)
		rot_c = '1';
	else if(rot > 135 && rot <= 225)
		rot_c = '2';
	else if(rot > 225 && rot <= 315)
		rot_c = '3';
	PutChr(letter);
	PutInt(pEntry->XPos, 0);
	PutChr(',');
	PutInt(pEntry->YPos, 0);
	PutChr(',');
	PutChr(rot_c); // Rotation
	PutChr(',');
	return 1;
}

int SLAPI EltronLabelPrinter::PutDataEntry(const BarcodeLabelEntry * pEntry)
{
	size_t buf_size = 256;
	char * p_temp_str = (char *)SAlloc::C(buf_size, 1);
	if(!p_temp_str)
		return PPSetErrorNoMem();
	if(pEntry->Type == BarcodeLabelEntry::etText) {
		const size_t len = sstrlen(pEntry->Text);
		size_t p = 0;
		for(size_t i = 0; i < len; i++) {
			if(p >= buf_size-1) {
				buf_size += 32;
				p_temp_str = (char *)SAlloc::R(p_temp_str, buf_size);
				if(!p_temp_str)
					return PPSetErrorNoMem();
			}
			if(pEntry->Text[i] == '\"')
				p_temp_str[p++] = '\\';
			p_temp_str[p++] = pEntry->Text[i];
		}
		p_temp_str[p] = 0;
		PutDataEntryPrefix('A', pEntry);
		if(pEntry->Font[0])
			PutStr(pEntry->Font);
		else
			PutChr('2'); // Default font
		PutChr(',');
		if(pEntry->XMult >= 1 && pEntry->XMult <= 8)
			PutInt(pEntry->XMult, 0);
		else
			PutInt(1, 0);
		PutChr(',');
		if(pEntry->YMult >= 1 && pEntry->YMult <= 9)
			PutInt(pEntry->YMult, 0);
		else
			PutInt(1, 0);
		PutChr(',');
		PutChr('N'); // Normal text ('R' - white on black)
	}
	else if(pEntry->Type == BarcodeLabelEntry::etBarcode) {
		strnzcpy(p_temp_str, pEntry->Text, buf_size);
		PutDataEntryPrefix('B', pEntry);
		char * p_std = 0;
		for(int j = 0; j < sizeof(_E_BarCStdTab) / sizeof(BarCStdToEltronEntry); j++)
			if(_E_BarCStdTab[j].Std == pEntry->BarcodeStd) {
				p_std = _E_BarCStdTab[j].P_Str;
				break;
			}
		if(p_std == 0)
			return PPSetError(PPERR_BARCSTDNSUPPORT);
		PutStr(p_std);
		PutChr(',');
		// Narrow bar width in dots {
		if(BcNarrowPt > 0 && BcNarrowPt <= 30)
			PutInt(BcNarrowPt, 0);
		else
			PutInt(2, 0);
		// }
		PutChr(',');
		// Wide bar width in dots (2..30) {
		if(BcWidePt > 1 && BcWidePt <= 30)
			PutInt(BcWidePt, 0);
		else
			PutInt(5, 0);
		// }
		PutChr(',');
		PutInt(pEntry->BarcodeHeight, 0);
		PutChr(',');
		PutChr('N'); // Print human readable code ('B' - yes; 'N' - no)
	}
	else {
		SAlloc::F(p_temp_str);
		return 0;
	}
	PutChr(',');
	PutChr('\"');
	PutStr(p_temp_str);
	PutChr('\"');
	PutChr('\n');
	SAlloc::F(p_temp_str);
	return 1;
}
//
//
//
//
// Implementation of PPALDD_BarcodeLabelList
//
struct DlBarcodeLabelListBlock {
	DlBarcodeLabelListBlock(const void * ptr) : N(0), ExemplarN(0), P_RgiList((const TSCollection <RetailGoodsInfo> *)ptr)
	{
	}
	uint   N; // Текущий номер позиции
	uint   ExemplarN; // Номер экземпляра для N [0..P_RgiList[N].LabelCount-1]
	const TSCollection <RetailGoodsInfo> * P_RgiList;
};

PPALDD_CONSTRUCTOR(BarcodeLabelList)
{
	InitFixData(rscDefHdr, &H, sizeof(H));
	InitFixData(rscDefIter, &I, sizeof(I));
}

PPALDD_DESTRUCTOR(BarcodeLabelList)
{
	DlBarcodeLabelListBlock * p_blk = (DlBarcodeLabelListBlock *)Extra[0].Ptr;
	delete p_blk;
	Destroy();
}

int PPALDD_BarcodeLabelList::InitData(PPFilt & rFilt, long rsrv)
{
	//SString temp_buf;

	DlBarcodeLabelListBlock * p_blk = new DlBarcodeLabelListBlock(rFilt.Ptr);
	Extra[0].Ptr = p_blk;

	//TSCollection <RetailGoodsInfo> * p_rgi_list = (TSCollection <RetailGoodsInfo> *)rFilt.Ptr;
	//Extra[1].Ptr = p_rgi_list;
	H.nn = 0;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_BarcodeLabelList::InitIteration(long iterId, int sortId, long rsrv)
{
	DlBarcodeLabelListBlock * p_blk = (DlBarcodeLabelListBlock *)Extra[0].Ptr;
	//TSCollection <RetailGoodsInfo> * p_rgi_list = (TSCollection <RetailGoodsInfo> *)NZOR(Extra[1].Ptr, Extra[0].Ptr);
	IterProlog(iterId, 1);
	if(p_blk && p_blk->P_RgiList) {
		if(sortId >= 0)
			SortIdx = sortId;
		H.nn = 0;
		p_blk->N = 0;
		p_blk->ExemplarN = 0;
		return 1;
	}
	else
		return 0;
}

int PPALDD_BarcodeLabelList::NextIteration(long iterId)
{
	int    ok = -1;
	IterProlog(iterId, 0);
	//TSCollection <RetailGoodsInfo> * p_rgi_list = (TSCollection <RetailGoodsInfo> *)NZOR(Extra[1].Ptr, Extra[0].Ptr);
	DlBarcodeLabelListBlock * p_blk = (DlBarcodeLabelListBlock *)Extra[0].Ptr;
	if(p_blk && p_blk->P_RgiList && p_blk->N < (int)p_blk->P_RgiList->getCount()) {
		const RetailGoodsInfo * p_rgi = p_blk->P_RgiList->at(p_blk->N);
        const uint num_copies = (p_rgi->LabelCount > 1 && p_rgi->LabelCount < 1000) ? p_rgi->LabelCount : 1;
		I.GoodsID = p_rgi->ID;
		I.BillDate = p_rgi->BillDate;
		STRNSCPY(I.BillNo, p_rgi->BillCode);
		I.Expiry = p_rgi->Expiry;
		I.Qtty = p_rgi->Qtty;
		I.PhQtty = p_rgi->PhQtty;
		I.Brutto = p_rgi->Brutto;
		I.Price = p_rgi->Price;
		I.RevalPrice = p_rgi->RevalPrice;
		I.UnitPerPack = p_rgi->UnitPerPack;
		STRNSCPY(I.Barcode, p_rgi->BarCode);
		STRNSCPY(I.Serial, p_rgi->Serial);
		p_blk->ExemplarN++;
		if(p_blk->ExemplarN >= num_copies) {
			p_blk->N++;
			p_blk->ExemplarN = 0;
			H.nn++;
		}
		ok = DlRtm::NextIteration(iterId);
	}
	return ok;
}
