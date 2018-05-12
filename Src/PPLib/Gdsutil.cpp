// GDSUTIL.CPP
// Copyright (c) A.Sobolev 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018
// @codepage windows-1251
// Утилиты для работы с товарами
//
#include <pp.h>
#pragma hdrstop

int FASTCALL CalcBarcodeCheckDigit(const char * pBarcode)
{
	size_t len = sstrlen(pBarcode);
	return SCalcBarcodeCheckDigitL(pBarcode, len);
}

char * FASTCALL AddBarcodeCheckDigit(char * pBarcode)
{
	size_t len = sstrlen(pBarcode);
	if(len) {
		int    cdig = SCalcBarcodeCheckDigitL(pBarcode, len);
		pBarcode[len++] = '0' + cdig;
		pBarcode[len] = 0;
	}
	return pBarcode;
}

SString & FASTCALL AddBarcodeCheckDigit(SString & rBarcode)
{
	if(rBarcode.Len()) {
		int    cdig = SCalcBarcodeCheckDigitL(rBarcode, rBarcode.Len());
		rBarcode.CatChar('0' + cdig);
	}
	return rBarcode;
}

static char * FASTCALL EncodeEAN8(const char * pCode, char * pBuf)
{
	size_t p = 0;
	pBuf[p++] = 'x';
	for(size_t i = 0; i < 8; i++) {
		if(i <= 3) {
			pBuf[p++] = pCode[i];
			if(i == 3)
				pBuf[p++] = 'X';
		}
		else if(i >= 4 && i <= 7)
			pBuf[p++] = 'a' + (pCode[i]-'0');
	}
	pBuf[p++] = 'x';
	pBuf[p] = 0;
	return pBuf;
}

static char * FASTCALL EncodeUPCE(const char * pCode, char * pBuf)
{
	static const char tab[10][5] = {
		{1, 1, 0, 0, 0}, // 0 0x18
		{1, 0, 1, 0, 0}, // 1 0x14
		{1, 0, 0, 1, 0}, // 2 0x12
		{1, 0, 0, 0, 1}, // 3 0x11
		{0, 1, 1, 0, 0}, // 4 0x0C
		{0, 0, 1, 1, 0}, // 5 0x06
		{0, 0, 0, 1, 1}, // 6 0x03
		{0, 1, 0, 1, 0}, // 7 0x0A
		{0, 1, 0, 0, 1}, // 8 0x09
		{0, 0, 1, 0, 1}  // 9 0x05
	};
	const size_t len = sstrlen(pCode);
	if(len != 8)
		return 0;
	else {
		size_t i, p = 0;
		int  row;
		int  chk_dig = pCode[7]-'0';
		int  leading = pCode[0]-'0';
		char pc[64];
		pc[p++] = 'K' + leading; // digit only
		pc[p++] = 'x'; // left-hand guard bar (101)
		for(i = 1; i < len-1; i++) {
			int c = pCode[i];
			if(i == 1) {
				row = (leading != 0) ? 0 : 1;
				if(row == 0)    // Row A
					pc[p++] = c;
				else            // Row B
					pc[p++] = 'A' + (c-'0');
			}
			else if(i >= 2 && i <= 6) {
				row = tab[chk_dig][i-2];
				if(leading != 0)
					row = (row == 1) ? 0 : 1;
				if(row == 0)    // Row A
					pc[p++] = c;
				else            // Row B
					pc[p++] = 'A' + (c-'0');
			}
		}
		pc[p++] = 'X'; // center-guard bar (01010)
		// Здесь нужно вставить терминальный guard bar (1), но я не нашел
		// в этих шрифтах такого знака.
		pc[p++] = 'K' + chk_dig; // digit only
		for(i = 0; i < p; i++)
			pBuf[i] = pc[i];
		pBuf[p] = 0;
		return pBuf;
	}
}

static char * FASTCALL EncodeEAN13(int upca, const char * pCode, char * pBuf)
{
	// 0 - A, 1 - B
	static const char tab[9][5] = {
		{0, 1, 0, 1, 1},
		{0, 1, 1, 0, 1},
		{0, 1, 1, 1, 0},
		{1, 0, 0, 1, 1},
		{1, 1, 0, 0, 1},
		{1, 1, 1, 0, 0},
		{1, 0, 1, 0, 1},
		{1, 0, 1, 1, 0},
		{1, 1, 0, 1, 1}
	};
	// Row A - '0'..'9'
	// Row B - 'A'..'J'
	// Row C - 'a'..'j'
	// Row D - 'K'..'T' (no bar - digit only)
	//
	// i   : 12 11 10 09 08 07 06 05 04 03 02 01 00
	// Row :  D  A -----tab------  C  C  C  C  C  C
	//
	//  tab = leading == 0 ? A : (tab[leading][i-6] ? B : A)
	//  leadig = pCode[0] - '0'
	//
	const size_t len = sstrlen(pCode);
	if(len != 13)
		return 0;
	else {
		size_t i, p = 0;
		int  leading = pCode[0]-'0';
		char pc[64];
		pc[p++] = 'x';
		for(i = 0; i < len; i++) {
			if(i == 6)
				pc[p++] = 'X';
			if(i == len-1)
				pc[p++] = 'x';
			int c = pCode[len-i-1];
			if(i < 6)           // Row C
				pc[p++] = 'a' + (c-'0');
			else if(i >= 6 && i <= 10) {
				int row = (leading == 0) ? 0 : tab[leading-1][10-i];
				if(row == 0)    // Row A
					pc[p++] = c;
				else            // Row B
					pc[p++] = 'A' + (c-'0');
			}
			else if(i == 11)    // Row A
				pc[p++] = c;
			else                // No bar (digit only)
				if(!upca)
					pc[p++] = 'K' + (c-'0');
		}
		for(i = 0; i < p; i++)
			pBuf[i] = pc[p-i-1];
		pBuf[p] = 0;
		return pBuf;
	}
}

int SLAPI CreatePrintableBarcode(const char * pBarcode, int codeType, char * pBuf, size_t bufLen)
{
	enum {
		undef = 0,
		ean8 = 1,
		ean13,
		upca,
		upce
	} code_type = undef;
	int    ok = 1;
	char   code[64], buf[64];
	int    chkdig;
	int    calc_check_dig = 0;
	size_t i;
	size_t len = sstrlen(strnzcpy(code, pBarcode, sizeof(code)));
	ASSIGN_PTR(pBuf, 0);
	buf[0] = 0;
	if(codeType == 39) {
		//
		// Code39
		// Проверить код: либо цифры, либо латинские буквы A..Z
		//
		for(i = 0; i < len; i++) {
			if(!isdec(code[i]))
				if(isalpha(code[i]))
					code[i] = toupper(code[i]);
				else
					return 0;
		}
		size_t j = 0;
		buf[j++] = '*';
		strnzcpy(buf+j, code, sizeof(buf)-j-1);
		j = sstrlen(buf);
		buf[j++] = '*';
		buf[j] = 0;
	}
	else {
		//
		// Проверить код: длина 1..13, все цифры
		//
		if(len > 13)
			return 0;
		else {
			for(i = 0; i < len; i++) {
				if(!isdec(code[i]))
					return 0;
			}
			if((len == 7 || len == 11) && code[0] == '0') { // UPC-A/UPC-E without check digit
				code_type = (len == 7) ? upce : upca;
				calc_check_dig = 1;
			}
			else if(len == 12 && code[0] != '0') { // EAN-13 without check digit
				code_type = ean13;
				calc_check_dig = 1;
			}
			else if(len == 7) { // EAN-8 without check digit OR wheighted code (XX99999)
				PPGoodsConfig goods_cfg;
				if(PPObjGoods::ReadConfig(&goods_cfg) > 0 && goods_cfg.IsWghtPrefix(code)) {
					code_type = ean13;
					calc_check_dig = 1;
					SString temp_buf = code;
					STRNSCPY(code, temp_buf.CatCharN('0', 5));
				}
				else {
					code_type = ean8;
					calc_check_dig = 1;
				}
			}
			else if(len < 7 || !oneof3(len, 8, 12, 13)) {
				len = sstrlen(padleft(code, '0', 12-len));
				code_type = ean13;
				calc_check_dig = 1;
			}
			else if(len == 8) {
				if(code[0] == '0')
					code_type = upce;
				else
					code_type = ean8;
			}
			else if(len == 12 && code[0] == '0')
				code_type = upca;
			else if(len == 13)
				code_type = ean13;
			else
				code_type = undef;
			if(calc_check_dig) {
				chkdig = CalcBarcodeCheckDigit(code);
				code[len++] = '0'+chkdig;
				code[len] = 0;
			}
			if(code_type == ean8)
				ok = EncodeEAN8(code, buf) ? 1 : 0;
			else if(code_type == ean13)
				ok = EncodeEAN13(0, code, buf) ? 1 : 0;
			else if(code_type == upca) {
				padleft(code, '0', 1);
				ok = EncodeEAN13(1, code, buf) ? 1 : 0;
			}
			else if(code_type == upce)
				ok = EncodeUPCE(code, buf) ? 1 : 0;
			else
				ok = 0;
		}
	}
	strnzcpy(pBuf, buf, bufLen);
	return ok;
}

int FASTCALL GetGoodsNameR(PPID goodsID, SString & rBuf)
{
	int    ok = -1;
	rBuf.Z();
	if(goodsID) {
		PPObjGoods goods_obj(SConstructorLite); // @v8.1.1 SConstructorLite
		Goods2Tbl::Rec goods_rec;
		if(goods_obj.Fetch(goodsID, &goods_rec) > 0) {
			rBuf = goods_rec.Name;
			ok = 1;
		}
		else
			ideqvalstr(goodsID, rBuf);
	}
	return ok;
}

SString & FASTCALL GetGoodsName(PPID goodsID, SString & rBuf)
{
	GetGoodsNameR(goodsID, rBuf);
	return rBuf;
}

static int SLAPI BarcodeList(BarcodeArray * pCodes, int * pSelection)
{
	class SelByBCListDlg : public PPListDialog {
	public:
		SelByBCListDlg(BarcodeArray * pBCList) : PPListDialog(DLG_SELBYBCODE, CTL_SELBYBCODE_LIST), P_BCodesList(pBCList)
		{
			updateList(-1);
		}
		int getDTS(int * pSel)
		{
			long   sel = 0;
			int    ok = getCurItem(0, &sel);
			ASSIGN_PTR(pSel, (int)sel);
			return ok;
		}
	private:
		virtual int setupList()
		{
			int     ok = -1;
			SString buf;
			StringSet ss(SLBColumnDelim);
			BarcodeTbl::Rec * p_rec = 0;
			if(P_BCodesList && P_BCodesList->getCount()) {
				for(uint i = 0; P_BCodesList->enumItems(&i, (void**)&p_rec);) {
					ss.clear();
					ss.add(buf.Z().Cat(p_rec->Code));
					GetGoodsName(p_rec->GoodsID, buf);
					ss.add(buf);
					THROW(addStringToList(i, ss.getBuf()));
				}
				ok = 1;
			}
			CATCHZOK
			return ok;
		}
		BarcodeArray * P_BCodesList;
	};
	int    ok = -1, sel = 0;
	SelByBCListDlg * dlg = new SelByBCListDlg(pCodes);
	if(CheckDialogPtrErr(&dlg)) {
		if(ExecView(dlg) == cmOK)
			ok = dlg->getDTS(&sel);
	}
	else
		ok = 0;
	ASSIGN_PTR(pSelection, sel - 1);
	delete dlg;
	return ok;
}

SLAPI GoodsCodeSrchBlock::GoodsCodeSrchBlock() : ArID(0), Flags(0), GoodsID(0), ScaleID(0), Qtty(0.0), P_List(0)
{
	Code[0] = 0;
	RetCode[0] = 0;
	MEMSZERO(Rec);
}

SLAPI GoodsCodeSrchBlock::~GoodsCodeSrchBlock()
{
	delete P_List;
}

int SLAPI PPObjGoods::GenerateScaleBarcode(PPID goodsID, PPID scaleID, SString & rCode)
{
	int    ok = -1;
	rCode.Z();
	InitConfig();
	if(GetConfig().Flags & GCF_USESCALEBCPREFIX) {
		PPObjScale sc_obj;
		PPScale sc_rec;
		if(sc_obj.Fetch(scaleID, &sc_rec) > 0 && sc_rec.AltGoodsGrp && sc_rec.IsValidBcPrefix()) {
			//ObjAssocTbl::Rec assc_rec;
			// @v9.2.2 if(PPRef->Assc.Search(PPASS_ALTGOODSGRP, sc_rec.AltGoodsGrp, goodsID, &assc_rec) > 0) {
			long    inner_num = 0;
			if(P_Tbl->GetGoodsCodeInAltGrp(goodsID, sc_rec.AltGoodsGrp, &inner_num) > 0) { // @v9.2.2
				rCode.Cat(sc_rec.BcPrefix);
				rCode.CatLongZ(/*assc_rec.InnerNum*/inner_num, (rCode.Len() == 3) ? 3 : 4);
				if(rCode.Len() == 6) {
					rCode.CatChar('0');
					ok = 1;
				}
				else {
					SString goods_name;
					GetGoodsName(goodsID, goods_name);
					ok = PPSetError(PPERR_GENSCALECODEFAULT, goods_name);
				}
			}
		}
	}
	return ok;
}

int SLAPI PPObjGoods::IsScaleBarcode(const char * pCode, PPID * pScaleID, PPID * pGoodsID, double * pQtty)
{
	int    ok = -1;
	PPID   goods_id = 0;
	PPID   scale_id = 0;
	double qtty = 1.0;
	char   code[64];
	int    is_wght_good = 0;
	Goods2Tbl::Rec goods_rec;
	BarcodeTbl::Rec bcr;
	STRNSCPY(code, pCode);
	size_t code_len = sstrlen(code);
	if(oneof2(code_len, 12, 13)) {
		InitConfig();
		const  int wp = GetConfig().IsWghtPrefix(code);
		if(wp) {
			code[12] = 0;
			if(wp == 2)
				qtty = atol(code+7); // Для штучного товара (по префиксу кода) количество не дробное.
			else
				qtty = fdiv1000i(atol(code+7));
			code[7] = 0;
			if(GetConfig().Flags & GCF_LOADTOSCALEGID) {
				strtolong(code+2, &goods_id);
				if(Fetch(goods_id, &goods_rec) > 0 && goods_rec.Kind == PPGDSK_GOODS)
					ok = 2;
				else
					ok = 1;
			}
			else if(SearchByBarcode(code, &bcr, &goods_rec, 1) > 0) {
				goods_id = goods_rec.ID;
				ok = 2;
			}
			else
				ok = 1;
		}
		else if(GetConfig().Flags & GCF_USESCALEBCPREFIX) {
			PPObjScale sc_obj;
			LAssocArray bcp_list;
			if(sc_obj.GetListWithBcPrefix(&bcp_list) > 0) {
				size_t max_len = 0;
				for(uint i = 0; i < bcp_list.getCount(); i++) {
					char   p[32];
					ltoa(bcp_list.at(i).Val, p, 10);
					size_t len = sstrlen(p);
					if(len && memcmp(code, p, len) == 0 && len > max_len) {
						max_len = len;
						scale_id = bcp_list.at(i).Key;
					}
				}
				if(scale_id) {
					PPScale sc_rec;
					ObjAssocTbl::Rec assc_rec;
					long   plu = 0;
					ok = 1;
					if(sc_obj.Fetch(scale_id, &sc_rec) > 0 && sc_rec.AltGoodsGrp) {
						code[12] = 0;
						qtty = fdiv1000i(atol(code+7));
						//
						// Пытаемся найти PLU, разрядность которого равна 3 или 4 (возможно, с ведущими нулями)
						//
						code[6] = 0;
						plu = atol(code+max_len);
						if(PPRef->Assc.SearchNum(PPASS_ALTGOODSGRP, sc_rec.AltGoodsGrp, plu, &assc_rec) > 0) {
							goods_id = assc_rec.ScndObjID;
							if(Fetch(goods_id, &goods_rec) > 0 && goods_rec.Kind == PPGDSK_GOODS)
								ok = 2;
						}
					}
				}
			}
		}
	}
	ASSIGN_PTR(pScaleID, scale_id);
	ASSIGN_PTR(pGoodsID, goods_id);
	ASSIGN_PTR(pQtty, qtty);
	return ok;
}

int SLAPI PPObjGoods::SearchByCodeExt(GoodsCodeSrchBlock * pBlk)
{
	int    ok = -1;
	if(pBlk) {
		pBlk->RetCode[0] = 0;
		pBlk->GoodsID = 0;
		pBlk->ScaleID = 0;
		pBlk->Qtty = 0.0;
		pBlk->Flags &= ~(GoodsCodeSrchBlock::fWeightCode | GoodsCodeSrchBlock::fArCode |
			GoodsCodeSrchBlock::fOwnArCode | GoodsCodeSrchBlock::fGoodsId | GoodsCodeSrchBlock::fList);
		MEMSZERO(pBlk->Rec);
		char   code[128];
		BarcodeTbl::Rec bcr;
		Goods2Tbl::Rec goods_rec;
		STRNSCPY(code, pBlk->Code);
		SOemToChar(code);
		if(oneof2(code[0], '#', '№') && code[1]) {
			PPID   goods_id = atol(code+1);
			if(Fetch(goods_id, &goods_rec) > 0) {
				pBlk->GoodsID = goods_rec.ID;
				pBlk->Qtty = 1.0;
				pBlk->RetCode[0] = 0;
				pBlk->Flags |= GoodsCodeSrchBlock::fGoodsId;
				pBlk->Rec = goods_rec;
				ok = 1;
			}
		}
		else if(code[0] == '*' && code[1]) {
			STRNSCPY(code, code+1);
			int    sel = -1;
			BarcodeArray bcary;
			ok = P_Tbl->SearchBarcodeSubstr(code, &bcary);
			if(ok) {
				PPID srch_ar_id = NZOR(pBlk->ArID, -1);
				int r = P_Tbl->SearchArCodeSubstr(srch_ar_id, code, &bcary);
				if(r > 0)
					ok = 1;
				if(srch_ar_id > 0) {
					r = P_Tbl->SearchArCodeSubstr(-1, code, &bcary);
					if(r > 0)
						ok = 1;
				}
			}
			if(ok > 0) {
				SETIFZ(pBlk->P_List, new BarcodeArray);
				*pBlk->P_List = bcary;
				pBlk->Flags  |= GoodsCodeSrchBlock::fList;
				ok = 1;
			}
			else if(ok)
				ok = -2;
			else
				ok = 0;
		}
		else if(pBlk->Flags & GoodsCodeSrchBlock::fUse2dTempl && SearchBy2dBarcode(pBlk->Code, &bcr, &goods_rec) > 0) {
			pBlk->GoodsID = goods_rec.ID;
			pBlk->Qtty    = bcr.Qtty;
			pBlk->Rec     = goods_rec;
			pBlk->Flags  |= GoodsCodeSrchBlock::fBc2d;
			strnzcpy(pBlk->RetCode, bcr.Code, 16);
			ok = 1;
		}
		else {
			int    is_wght_good = 0;
			PPID   goods_id = 0;
			PPID   scale_id = 0;
			double qtty   = 0.0;
			size_t code_len = sstrlen(code);
			strnzcpy(pBlk->RetCode, code, 16);
			int    r = IsScaleBarcode(code, &scale_id, &goods_id, &qtty);
			if(r > 0 && Fetch(goods_id, &goods_rec) > 0) {
				pBlk->GoodsID = goods_id;
				pBlk->ScaleID = scale_id;
				pBlk->Qtty    = qtty;
				pBlk->Flags  |= GoodsCodeSrchBlock::fWeightCode;
				pBlk->Rec = goods_rec;
				ok = 1;
			}
			else if(r == 1) { // Код весовой, но товар, ему соответствующий, не найден
				pBlk->GoodsID = goods_id;
				pBlk->ScaleID = scale_id;
				pBlk->Qtty    = qtty;
				pBlk->Flags  |= GoodsCodeSrchBlock::fWeightCode;
				ok = -1;
			}
			else if(r < 0) {
				ArGoodsCodeTbl::Rec arc_rec;
				if(SearchByBarcode(code, &bcr, &goods_rec, BIN(pBlk->Flags & GoodsCodeSrchBlock::fAdoptSearch)) > 0) {
					pBlk->GoodsID = goods_rec.ID;
					pBlk->Qtty = bcr.Qtty;
					pBlk->Rec = goods_rec;
					ok = 1;
				}
				else if(P_Tbl->SearchByArCode(pBlk->ArID, code, &arc_rec, &goods_rec) > 0) {
					pBlk->GoodsID = goods_rec.ID;
					pBlk->Qtty    = 1.0;
					pBlk->Flags  |= GoodsCodeSrchBlock::fArCode;
					pBlk->Rec = goods_rec;
					ok = 1;
				}
				else if(pBlk->ArID && P_Tbl->SearchByArCode(0, code, &arc_rec, &goods_rec) > 0) {
					pBlk->GoodsID = goods_rec.ID;
					pBlk->Qtty    = 1.0;
					pBlk->Flags  |= GoodsCodeSrchBlock::fOwnArCode;
					pBlk->Rec = goods_rec;
					ok = 1;
				}
				else
					ok = -2;
			}
		}
	}
	return ok;
}

int SLAPI PPObjGoods::GetGoodsByBarcode(const char * pBarcode, PPID arID, Goods2Tbl::Rec * pRec, double * pQtty, SString * pRetCode)
{
	int    ok = -1, r;
	GoodsCodeSrchBlock blk;
	STRNSCPY(blk.Code, pBarcode);
	blk.ArID = arID;
	blk.Flags |= GoodsCodeSrchBlock::fAdoptSearch;
	//
	// Сохранение последнего введенного штрихкода в глобальном буфере
	//
	STRNSCPY(DS.GetTLA().Lid.Barcode, blk.Code);
	if((r = SearchByCodeExt(&blk)) > 0) {
		if(blk.Flags & GoodsCodeSrchBlock::fList && blk.P_List) {
			int    sel = -1;
			if(BarcodeList(blk.P_List, &sel) > 0) {
				BarcodeTbl::Rec bcr = blk.P_List->at(sel);
				ASSIGN_PTR(pRetCode, bcr.Code);
				if(Fetch(bcr.GoodsID, pRec) > 0) {
					ASSIGN_PTR(pQtty, bcr.Qtty);
					ok = 1;
				}
				else
					ok = -2;
			}
		}
		else {
			ASSIGN_PTR(pRec, blk.Rec);
			ASSIGN_PTR(pQtty, blk.Qtty);
			ok = 1;
		}
	}
	else
		ok = r;
	ASSIGN_PTR(pRetCode, blk.RetCode);
	return ok;
}

int SLAPI PPObjGoods::SelectGoodsByBarcode(int initChar, PPID arID, Goods2Tbl::Rec * pRec, double * pQtty, SString * pRetCode)
{
	int    r = -1;
	SString code = DS.GetTLA().Lid.Barcode;
	if((r = BarcodeInputDialog(initChar, code)) > 0) {
		r = GetGoodsByBarcode(code, arID, pRec, pQtty, pRetCode);
		if(r <= 0)
			ASSIGN_PTR(pRetCode, code);
	}
	if(!r)
		PPError();
	return r;
}

int SLAPI LoadGoodsStruc(const PPGoodsStruc::Ident * pIdent, PPGoodsStruc * pGs)
{
	PPObjGoods gobj;
	return gobj.LoadGoodsStruc(pIdent, pGs);
}

int SLAPI PPObjGoods::GetAltGoodsStrucID(PPID goodsID, PPID * pDynGenID, PPID * pStrucID)
{
	PPID   gs_id = 0;
	PPID   dyn_gen_id = 0;
	if(P_Tbl->BelongToGen(goodsID, &dyn_gen_id) > 0) {
		Goods2Tbl::Rec gen_rec;
		if(Search(dyn_gen_id, &gen_rec) > 0)
			gs_id = gen_rec.StrucID;
	}
	ASSIGN_PTR(pDynGenID, dyn_gen_id);
	ASSIGN_PTR(pStrucID, gs_id);
	return gs_id ? 1 : -1;
}

int SLAPI PPObjGoods::LoadGoodsStruc(const PPGoodsStruc::Ident * pIdent, PPGoodsStruc * pGs)
{
	int    ok = -1;
	Goods2Tbl::Rec goods_rec;
	if(pIdent && pIdent->GoodsID && (ok = Fetch(pIdent->GoodsID, &goods_rec)) > 0) {
		PPID   gs_id = goods_rec.StrucID;
		if(!gs_id)
			GetAltGoodsStrucID(pIdent->GoodsID, 0, &gs_id);
		PPObjGoodsStruc gs_obj;
		PPGoodsStruc gs;
		if(gs_id > 0 && gs_obj.Get(gs_id, &gs) > 0 && gs.Select(pIdent, pGs))
			ok = 1;
		else
			ok = (PPErrCode = PPERR_UNDEFGOODSSTRUC, -1);
	}
	return ok;
}

int SLAPI PPObjGoods::LoadGoodsStruc(const PPGoodsStruc::Ident * pIdent, TSCollection <PPGoodsStruc> & rGsList)
{
	rGsList.freeAll();

	int    ok = -1;
	Goods2Tbl::Rec goods_rec;
	if(pIdent && pIdent->GoodsID && (ok = Fetch(pIdent->GoodsID, &goods_rec)) > 0) {
		PPID   gs_id = goods_rec.StrucID;
		if(!gs_id)
			GetAltGoodsStrucID(pIdent->GoodsID, 0, &gs_id);
		PPObjGoodsStruc gs_obj;
		PPGoodsStruc gs;
		if(gs_id > 0 && gs_obj.Get(gs_id, &gs) > 0 && gs.Select(pIdent, rGsList) > 0)
			ok = 1;
		else
			ok = (PPErrCode = PPERR_UNDEFGOODSSTRUC, -1);
	}
	return ok;
}

int SLAPI PPObjGoods::Helper_EditGoodsStruc(PPID goodsID, int isDynGen)
{
	int    ok = -1;
	PPGoodsPacket pack;
	if(goodsID && GetPacket(goodsID, &pack, 0) > 0) {
		PPID   gs_id = pack.Rec.StrucID;
		PPID   dyn_gen_id = 0;
		if(gs_id == 0 && GetAltGoodsStrucID(goodsID, &dyn_gen_id, &gs_id) > 0) {
			ok = Helper_EditGoodsStruc(dyn_gen_id, 1); // @recursion
		}
		else {
			if(isDynGen)
				pack.GS.Rec.Flags |= GSF_DYNGEN;
			pack.GS.GoodsID = goodsID;
			if(GSObj.EditDialog(&pack.GS) > 0) {
				pack.UpdFlags |= PPGoodsPacket::ufChgNamedStruc;
				if(PutPacket(&goodsID, &pack, 1))
					ok = 1;
				else
					ok = PPErrorZ();
			}
		}
	}
	return ok;
}

int SLAPI PPObjGoods::EditGoodsStruc(PPID goodsID)
{
	return Helper_EditGoodsStruc(goodsID, 0);
}

int SLAPI PPObjGoods::Helper_WriteConfig(const PPGoodsConfig * pCfg, const SString * pGoodsExTitles,
	PPOpCounterPacket * pOwnAcCntr, int rebuild, int use_ta)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	int    r, is_new = 0;
	PPGoodsConfig cfg = *pCfg;
	PPGoodsConfig prev_cfg;
	// @v9.7.2 assert(!cfg.Ver.IsLt(7, 7, 0)); // Жесткая проверка на предмет инициализации номера версии
	cfg.Ver__ = DS.GetVersion();
	{
		PPTransaction tra(use_ta); // @v8.7.0 @fix 1-->use_ta
		THROW(tra);
		cfg.Flags &= ~GCF_VALID;
		THROW(r = ReadConfig(&prev_cfg));
		is_new = BIN(r < 0);
		if(pOwnAcCntr) {
			PPObjOpCounter opc_obj;
			pOwnAcCntr->Head.ObjType = PPOBJ_GOODS;
			pOwnAcCntr->Head.OwnerObjID = -1;
			pOwnAcCntr->Flags |= PPOpCounterPacket::fDontUpdCounter;
			THROW(opc_obj.PutPacket(&cfg.OwnArCodeCntrID, pOwnAcCntr, 0));
		}
		{
			const  size_t offs = offsetof(PPGoodsConfig, TagIndFilt);
			size_t sz = offs;
			strip(cfg.WghtPrefix);
			if(!cfg.TagIndFilt.IsEmpty()) {
				SBuffer ser_buf;
				THROW(cfg.TagIndFilt.Write(ser_buf, 0));
				sz += ser_buf.GetAvailableSize();
				{
					STempBuffer temp_buf(sz);
					THROW_SL(temp_buf.IsValid());
					memcpy(temp_buf, &cfg, offs);
					THROW_SL(ser_buf.Read(((char *)temp_buf)+offs, sz - offs));
					THROW(p_ref->PutProp(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_GOODSCFG, (char *)temp_buf, sz));
				}
			}
			else {
				THROW(p_ref->PutProp(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_GOODSCFG, &cfg, sz));
			}
		}
		if(pGoodsExTitles) {
			THROW(p_ref->PutPropVlrString(PPOBJ_GOODSGROUP, 0, GGPRP_EXSTRTITLES, *pGoodsExTitles));
		}
		if(rebuild) {
			PPWait(1);
			THROW(P_Tbl->RemoveBarcodeLeadingZeros(0));
			PPWait(0);
		}
		DS.LogAction(is_new ? PPACN_CONFIGCREATED : PPACN_CONFIGUPDATED, PPCFGOBJ_GOODS, 0, 0, 0);
		THROW(tra.Commit());
	}
	P_Tbl->DirtyConfig();
	CATCHZOK
	return ok;
}

// static
int FASTCALL PPObjGoods::ReadConfig(PPGoodsConfig * pCfg)
{
	int    ok = -1;
	Reference * p_ref = PPRef;
	assert(pCfg);
	pCfg->Clear();
	pCfg->Ver__ = DS.GetVersion(); // @v9.7.2
	size_t sz = 0;
	if(p_ref->GetPropActualSize(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_GOODSCFG, &sz) > 0) {
		const size_t pre770_size = pCfg->GetSize_Pre770();
		assert(pre770_size == (offsetof(PPGoodsConfig, Ver__)));
		if(sz <= pre770_size) {
			ok = p_ref->GetPropMainConfig(PPPRP_GOODSCFG, pCfg, sz);
			assert(ok > 0); // Раз нам удалось считать размер буфера, то последующая ошибка чтения - критична
			THROW(ok > 0);
		}
		else {
			STempBuffer temp_buf(sz);
			ok = p_ref->GetPropMainConfig(PPPRP_GOODSCFG, (char *)temp_buf, sz);
			assert(ok > 0); // Раз нам удалось считать размер буфера, то последующая ошибка чтения - критична
			THROW(ok > 0);
			{
				const size_t offs = offsetof(PPGoodsConfig, TagIndFilt);
				const size_t after_ver_offs = offsetof(PPGoodsConfig, Ver__)+sizeof(pCfg->Ver__);
				//
				// Сначала считаем данные до номера версии включительно...
				//
				memcpy(pCfg, (const char *)temp_buf, after_ver_offs);
				if(!pCfg->Ver__.IsLt(7, 7, 2)) {
					//
					// ... и, если версия больше или равна 7.7.2, дочитаем оставшийся кусочек
					//
					memcpy(PTR8(pCfg)+after_ver_offs, ((const char *)temp_buf)+after_ver_offs, offs-after_ver_offs);
				}
				strip(pCfg->WghtPrefix);
				if(sz > offs) {
					SBuffer ser_buf;
					THROW_SL(ser_buf.Write(((const char *)temp_buf)+offs, sz - offs));
					if(!pCfg->TagIndFilt.Read(ser_buf, 0)) {
						pCfg->TagIndFilt.Init(1, 0);
						PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

//static
int SLAPI PPObjGoods::ReadGoodsExTitles(PPID grpID, SString & rBuf)
{
	Reference * p_ref = PPRef;
	SString temp_buf;
	if(p_ref->GetPropVlrString(PPOBJ_GOODSGROUP, grpID, GGPRP_EXSTRTITLES, temp_buf) <= 0) {
		if(p_ref->GetPropVlrString(PPOBJ_GOODSGROUP, 0, GGPRP_EXSTRTITLES, temp_buf) <= 0)
			PPLoadText(PPTXT_GOODSEXTITLES, temp_buf);
	}
	rBuf = temp_buf;
	return 1;
}

//static
GoodsPacketKind SLAPI PPObjGoods::GetRecKind(const Goods2Tbl::Rec * pRec)
{
	if(pRec->Kind == PPGDSK_GOODS)
		return gpkndGoods;
	else if(pRec->Kind == PPGDSK_GROUP)
		if(pRec->Flags & GF_ALTGROUP)
			return gpkndAltGroup;
		else if(pRec->Flags & GF_FOLDER)
			return gpkndFolderGroup;
		else
			return gpkndOrdinaryGroup;
	return gpkndUndef;
}

//static
int SLAPI PPObjGoods::GenerateOwnArCode(SString & rCode, int use_ta)
{
	int    ok = -1;
	PPObjGoods goods_obj;
	const  PPID cntr_id = goods_obj.GetConfig().OwnArCodeCntrID;
	if(cntr_id) {
		PPObjOpCounter opc_obj;
		long   counter = 0;
		char   temp_buf[64];
		opc_obj.GetCode(cntr_id, &counter, temp_buf, sizeof(temp_buf), 0, use_ta);
		rCode = temp_buf;
		ok = rCode.NotEmptyS() ? 1 : -1;
	}
	else
		rCode.Z();
	return ok;
}
//
//
//
SLAPI PPGoodsConfig::PPGoodsConfig()
{
	Clear();
}

PPGoodsConfig & PPGoodsConfig::Clear()
{
	memzero(this, (size_t)(PTR8(&TagIndFilt) - PTR8(this)));
	// @v9.7.2 (expensive call) Ver = DS.GetVersion();
	TagIndFilt.Init(1, 0);
	return *this;
}

int FASTCALL PPGoodsConfig::IsWghtPrefix(const char * pCode) const
{
	int    yes = 0;
	if(pCode) {
		if(WghtPrefix[0] && strnicmp(pCode, WghtPrefix, sstrlen(WghtPrefix)) == 0)
			yes = 1;
		else if(WghtCntPrefix[0] && strnicmp(pCode, WghtCntPrefix, sstrlen(WghtCntPrefix)) == 0)
			yes = 2;
	}
	return yes;
}

int SLAPI PPGoodsConfig::GetCodeLenList(PPIDArray * pList, int * pAllowEmpty) const
{
	int    allow_empty = 1;
	SString len_str = BarCodeLen;
	SString item_buf;
	StringSet ss(',', len_str.Strip());
	CALLPTRMEMB(pList, freeAll());
	for(uint i = 0; ss.get(&i, item_buf) > 0;) {
		item_buf.Strip();
		if(item_buf.C(0) == '+' && item_buf.Len() == 1)
			allow_empty = 0;
		else if(pList) {
			long len = item_buf.ToLong();
			if(len > 0)
				pList->addUnique(len);
		}
	}
	ASSIGN_PTR(pAllowEmpty, allow_empty);
	return 1;
}

// Prototype. Defined in PPGoods\GoodsDlg.cpp
int SLAPI EditGoodsExTitles(SString & rGoodsExTitles);

class GoodsCfgDialog : public TDialog {
public:
	GoodsCfgDialog() : TDialog(DLG_GDSCFG)
	{
	}
	int    setDTS(const PPGoodsConfig *, const SString & rGoodsExTitles, const PPOpCounterPacket & rOwnAcCntr);
	int    getDTS(PPGoodsConfig *, SString & rGoodsExTitles, PPOpCounterPacket & rOwnAcCntr);
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCmd(cmGoodsExTitles)) {
			EditGoodsExTitles(GoodsExTitles);
		}
		else if(event.isCmd(cmTagIndFilt)) {
			Data.TagIndFilt.Flags |= TagFilt::fColors; // @v9.8.6
			EditTagFilt(PPOBJ_GOODS, &Data.TagIndFilt);
		}
		else
			return;
		clearEvent(event);
	}
	PPGoodsConfig Data;
	SString GoodsExTitles;
};

int GoodsCfgDialog::setDTS(const PPGoodsConfig * pData, const SString & rGoodsExTitles, const PPOpCounterPacket & rOwnAcCntr)
{
	Data = *pData;
	GoodsExTitles = rGoodsExTitles;

	AddClusterAssoc(CTL_GDSCFG_FLAGS,  0, GCF_SUPPRLZERO);
	AddClusterAssoc(CTL_GDSCFG_FLAGS,  1, GCF_ENABLEWP);
	AddClusterAssoc(CTL_GDSCFG_FLAGS,  2, GCF_BCCHKDIG);
	AddClusterAssoc(CTL_GDSCFG_FLAGS,  3, GCF_BCNDIG);
	AddClusterAssoc(CTL_GDSCFG_FLAGS,  4, GCF_DISABLEWOTAXFLAG);
	AddClusterAssoc(CTL_GDSCFG_FLAGS,  5, GCF_LOADTOSCALEGID);
	AddClusterAssoc(CTL_GDSCFG_FLAGS,  6, GCF_RETAILPRICEBYMOSTRECENTLOT);
	AddClusterAssoc(CTL_GDSCFG_FLAGS,  7, GCF_DONTSELPASSIVE);
	AddClusterAssoc(CTL_GDSCFG_FLAGS,  8, GCF_ADDTOALTGRPWITHOUTMSG);
	AddClusterAssoc(CTL_GDSCFG_FLAGS,  9, GCF_ONUNITEMOVEBARCODE);
	AddClusterAssoc(CTL_GDSCFG_FLAGS, /*11*/10, GCF_USESCALEBCPREFIX);
	AddClusterAssoc(CTL_GDSCFG_FLAGS, 11, GCF_USEBRANDINGSELEXTDLG);
	AddClusterAssoc(CTL_GDSCFG_FLAGS, 12, GCF_AUTOPREFBARCODE);
	AddClusterAssoc(CTL_GDSCFG_FLAGS, 13, GCF_DONTDELFROMMTXGOODSINOPENORD);
	AddClusterAssoc(CTL_GDSCFG_FLAGS, 14, GCF_SHOWGSTRUCPRICE); // @v8.0.0

	SetClusterData(CTL_GDSCFG_FLAGS, Data.Flags);

	AddClusterAssoc(CTL_GDSCFG_XCHG_FLAGS, 0, GCF_XCHG_DONTRCVTAXGRPUPD);
	AddClusterAssoc(CTL_GDSCFG_XCHG_FLAGS, 1, GCF_XCHG_RCVSTRUCUPD);
	AddClusterAssoc(CTL_GDSCFG_XCHG_FLAGS, 2, GCF_XCHG_SENDGENGOODSCONTENT); // @v7.7.12
	AddClusterAssoc(CTL_GDSCFG_XCHG_FLAGS, 3, GCF_XCHG_SENDALTGROUP); // @v8.1.12
	AddClusterAssoc(CTL_GDSCFG_XCHG_FLAGS, 4, GCF_XCHG_SENDATTACHMENT); // @v8.2.3
	SetClusterData(CTL_GDSCFG_XCHG_FLAGS, Data.Flags);

	setCtrlData(CTL_GDSCFG_CODLEN, Data.BarCodeLen);
	setCtrlData(CTL_GDSCFG_MINUNIQBCLEN, &Data.MinUniqBcLen);
	setCtrlData(CTL_GDSCFG_WPRFX,  Data.WghtPrefix);
	setCtrlData(CTL_GDSCFG_CPRFX,  Data.WghtCntPrefix); // @v7.0.7
	setCtrlData(CTL_GDSCFG_ACGIT,  &Data.ACGI_Threshold);
	SetupPPObjCombo(this, CTLSEL_GDSCFG_DEFUNIT, PPOBJ_UNIT, Data.DefUnitID, OLW_CANINSERT, (void *)PPUnit::Trade);
	SetupPPObjCombo(this, CTLSEL_GDSCFG_DEFPCKGTYP, PPOBJ_PCKGTYPE, Data.DefPckgTypeID, 0);
	SetupPPObjCombo(this, CTLSEL_GDSCFG_DEFGRP,   PPOBJ_GOODSGROUP, Data.DefGroupID, OLW_CANSELUPLEVEL); // @v7.2.7
	SetupPPObjCombo(this, CTLSEL_GDSCFG_ASSETGRP, PPOBJ_GOODSGROUP, Data.AssetGrpID, OLW_CANSELUPLEVEL);
	SetupPPObjCombo(this, CTLSEL_GDSCFG_TAREGRP,  PPOBJ_GOODSGROUP, Data.TareGrpID,  OLW_CANSELUPLEVEL);
	SetupPPObjCombo(this, CTLSEL_GDSCFG_DEFGOODS, PPOBJ_GOODS,  Data.DefGoodsID, OLW_LOADDEFONOPEN, 0); // @v8.9.5
	SetupPPObjCombo(this, CTLSEL_GDSCFG_GDSMATRIX, PPOBJ_QUOTKIND,  Data.MtxQkID, 0, (void *)1);
	AddClusterAssoc(CTL_GDSCFG_IGNFLDMTX, 0, GCF_IGNOREFOLDERMATRIX); // @v8.3.4
	SetClusterData(CTL_GDSCFG_IGNFLDMTX, Data.Flags);                 // @v8.3.4
	SetupPPObjCombo(this, CTLSEL_GDSCFG_GMTXRESTR, PPOBJ_QUOTKIND,  Data.MtxRestrQkID, 0, (void *)1);
	{
		ObjTagFilt ot_filt;
		ot_filt.ObjTypeID = PPOBJ_GLOBALUSERACC;
		ot_filt.Flags |= ObjTagFilt::fOnlyTags;
		SetupObjTagCombo(this, CTLSEL_GDSCFG_BCPGUATAG, Data.BcPrefixGuaTagID, 0, &ot_filt);
	}
	setCtrlString(CTL_GDSCFG_OWNACTMPL, rOwnAcCntr.Head.CodeTemplate);
	setCtrlLong(CTL_GDSCFG_OWNACCNTR, rOwnAcCntr.Head.Counter);
	disableCtrl(CTL_GDSCFG_OWNACCNTR, 1);
	return 1;
}

int GoodsCfgDialog::getDTS(PPGoodsConfig * pData, SString & rGoodsExTitles, PPOpCounterPacket & rOwnAcCntr)
{
	int    ok = 1;
	uint   sel = 0;
	char * p;
	getCtrlData(CTL_GDSCFG_CODLEN, Data.BarCodeLen);
	getCtrlData(CTL_GDSCFG_MINUNIQBCLEN, &Data.MinUniqBcLen);
	getCtrlData(CTL_GDSCFG_WPRFX,  Data.WghtPrefix);
	getCtrlData(CTL_GDSCFG_CPRFX,  Data.WghtCntPrefix);
	getCtrlData(CTLSEL_GDSCFG_DEFUNIT, &Data.DefUnitID);
	getCtrlData(CTLSEL_GDSCFG_DEFPCKGTYP, &Data.DefPckgTypeID);
	getCtrlData(CTLSEL_GDSCFG_DEFGRP,   &Data.DefGroupID);
	getCtrlData(CTLSEL_GDSCFG_ASSETGRP, &Data.AssetGrpID);
	getCtrlData(CTLSEL_GDSCFG_TAREGRP,  &Data.TareGrpID);
	getCtrlData(CTLSEL_GDSCFG_DEFGOODS, &Data.DefGoodsID); // @v8.9.5
	getCtrlData(CTL_GDSCFG_ACGIT,  &Data.ACGI_Threshold);
	getCtrlData(CTLSEL_GDSCFG_GDSMATRIX, &Data.MtxQkID);
	GetClusterData(CTL_GDSCFG_IGNFLDMTX, &Data.Flags);        // @v8.3.4
	getCtrlData(CTLSEL_GDSCFG_GMTXRESTR, &Data.MtxRestrQkID);
	getCtrlData(CTLSEL_GDSCFG_BCPGUATAG, &Data.BcPrefixGuaTagID);
	GetClusterData(CTL_GDSCFG_FLAGS, &Data.Flags);
	GetClusterData(CTL_GDSCFG_XCHG_FLAGS, &Data.Flags);
	sel = CTL_GDSCFG_ACGIT;
	THROW_PP(Data.ACGI_Threshold >= 0 && Data.ACGI_Threshold <= 1461, PPERR_USERINPUT);
	sel = CTL_GDSCFG_CODLEN;
	for(p = strip(Data.BarCodeLen); *p; p++)
		THROW_PP_S(isdec(*p) || oneof3(*p, ' ', ',', '+'), PPERR_INVEXPR, Data.BarCodeLen);
	sel = CTL_GDSCFG_WPRFX;
	for(p = strip(Data.WghtPrefix); *p; p++)
		THROW_PP_S(isdec(*p), PPERR_INVEXPR, Data.WghtPrefix);
	ASSIGN_PTR(pData, Data);
	rGoodsExTitles = GoodsExTitles;
	getCtrlData(CTL_GDSCFG_OWNACTMPL, rOwnAcCntr.Head.CodeTemplate);
	getCtrlData(CTL_GDSCFG_OWNACCNTR, &rOwnAcCntr.Head.Counter);
	CATCH
		ok = PPErrorByDialog(this, sel);
	ENDCATCH
	return ok;
}

int SLAPI PPObjGoods::WriteConfig(const PPGoodsConfig * pCfg, const SString * pGoodsExTitles, int use_ta)
{
	return Helper_WriteConfig(pCfg, pGoodsExTitles, 0, 0, use_ta);
}

// static
int SLAPI PPObjGoods::EditConfig()
{
	int    ok = -1, ta = 0, is_new = 0;
	int    valid_data = 0;
	int    suppress_l_zero = 0;
	SString goods_ex_titles;
	PPObjOpCounter opc_obj;
	PPOpCounterPacket ownac_cntr;
	PPObjGoods goods_obj;
	PPGoodsConfig goods_cfg;
	GoodsCfgDialog * dlg = 0;
	THROW(CheckCfgRights(PPCFGOBJ_GOODS, PPR_READ, 0));
	THROW(is_new = ReadConfig(&goods_cfg));
	THROW(ReadGoodsExTitles(0, goods_ex_titles));
	opc_obj.GetPacket(goods_cfg.OwnArCodeCntrID, &ownac_cntr);
	suppress_l_zero = BIN(goods_cfg.Flags & GCF_SUPPRLZERO);
	THROW(CheckDialogPtr(&(dlg = new GoodsCfgDialog)));
	dlg->setDTS(&goods_cfg, goods_ex_titles, ownac_cntr);
	while(!valid_data && ExecView(dlg) == cmOK) {
		THROW(CheckCfgRights(PPCFGOBJ_GOODS, PPR_MOD, 0));
		if(dlg->getDTS(&goods_cfg, goods_ex_titles, ownac_cntr))
			ok = valid_data = 1;
	}
	if(ok > 0) {
		int    rebuild = 0;
		if(!suppress_l_zero && goods_cfg.Flags & GCF_SUPPRLZERO)
			if(PPMessage(mfConf|mfYes|mfNo, PPCFM_SUPPRBCLZERO) == cmYes)
				rebuild = 1;
		THROW(goods_obj.Helper_WriteConfig(&goods_cfg, &goods_ex_titles.Strip(), &ownac_cntr, rebuild, 1));
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

RetailGoodsInfo::RetailGoodsInfo()
{
	Init();
}

void RetailGoodsInfo::Init()
{
	THISZERO();
}

int SLAPI PPObjGoods::GetRetailGoodsInfo(PPID goodsID, PPID locID, RetailGoodsInfo * pInfo)
{
	return Helper_GetRetailGoodsInfo(goodsID, locID, 0, 0, ZERODATETIME, 0.0, pInfo, 0);
}

int SLAPI PPObjGoods::GetRetailGoodsInfo(PPID goodsID, PPID locID, const RetailPriceExtractor::ExtQuotBlock * pEqBlk,
	PPID arID, LDATETIME actualDtm, double qtty, RetailGoodsInfo * pInfo, long flags)
{
	return Helper_GetRetailGoodsInfo(goodsID, locID, pEqBlk, arID, actualDtm, qtty, pInfo, flags);
}

int SLAPI PPObjGoods::GetRetailGoodsInfo(PPID goodsID, PPID locID, const RetailPriceExtractor::ExtQuotBlock * pEqBlk,
	PPID arID, double qtty, RetailGoodsInfo * pInfo, long flags)
{
	return Helper_GetRetailGoodsInfo(goodsID, locID, pEqBlk, arID, ZERODATETIME, qtty, pInfo, flags);
}

int SLAPI PPObjGoods::Helper_GetRetailGoodsInfo(PPID goodsID, PPID locID, const RetailPriceExtractor::ExtQuotBlock * pEqBlk,
	PPID arID, LDATETIME actualDtm, double qtty, RetailGoodsInfo * pInfo, long flags)
{
	int    ok = 1;
	const  PPID loc_id = NZOR(locID, LConfig.Location);
	Goods2Tbl::Rec goods_rec;
	SString temp_buf;
	SString src_code = (flags & rgifUseInBarcode) ? pInfo->BarCode : 0;
	const double code_qtty = pInfo->Qtty;
	const double outer_price = pInfo->OuterPrice; // @v8.0.12
	pInfo->Init();
	pInfo->ID = goodsID;
	pInfo->LocID = loc_id;
	if(goodsID && Fetch(goodsID, &goods_rec) > 0) {
		if(!(flags & rgifPriceOnly)) {
			PPUnit unit_rec;
			PPCountryBlock country_blk;
			pInfo->ID = goodsID;
			STRNSCPY(pInfo->Name, goods_rec.Name);
			if(src_code.NotEmptyS())
				temp_buf = src_code;
			else
				FetchSingleBarcode(goodsID, temp_buf);
			if(flags & rgifConcatQttyToCode) {
				const int wp = GetConfig().IsWghtPrefix(temp_buf);
				if(wp && temp_buf.Len() == 7) {
					long   i_code_qtty = (wp == 2) ? (long)R0(fabs(code_qtty)) : (long)R0(fabs(code_qtty) * 1000.0);
					temp_buf.CatLongZ(i_code_qtty, 5);
					AddBarcodeCheckDigit(temp_buf);
				}
			}
			temp_buf.CopyTo(pInfo->BarCode, sizeof(pInfo->BarCode));
			// @v8.0.0 {
			if(goods_rec.ManufID) {
				if(SETIFZ(P_PsnObj, new PPObjPerson)) {
					GetPersonName(goods_rec.ManufID, temp_buf);
					STRNSCPY(pInfo->Manuf, temp_buf);
				}
			}
			// } @v8.0.0
			GetManufCountry(goodsID, &goods_rec, 0, &country_blk);
			country_blk.Name.CopyTo(pInfo->ManufCountry, sizeof(pInfo->ManufCountry));
			if(FetchUnit(goods_rec.UnitID, &unit_rec) > 0)
				STRNSCPY(pInfo->UnitName, unit_rec.Name);
			pInfo->PhUPerU = goods_rec.PhUPerU;
		}
		{
			//
			// @v7.8.1 Объединили расчет цены для нелимитированных и лимитированных товаров.
			// RetailPriceExtractor самостоятельно идентифицирует категорияю товара
			//
			RetailExtrItem  rtl_ext_item;
			long   rpe_flags = 0;
			SETFLAG(rpe_flags, RTLPF_USEQUOTWTIME, flags & rgifUseQuotWTimePeriod);
			SETFLAG(rpe_flags, RTLPF_PRICEBYQUOT,  flags & rgifUseBaseQuotAsPrice);
			// @v8.0.12 {
			if(flags & rgifUseOuterPrice && outer_price > 0.0) {
				rpe_flags |= RTLPF_USEOUTERPRICE;
				rtl_ext_item.OuterPrice = outer_price;
			}
			// } @v8.0.12
			RetailPriceExtractor rpe(loc_id, pEqBlk, arID, actualDtm, rpe_flags); // @v7.8.1 0-->pEqBlk
			if(rpe.GetPrice(goodsID, 0, qtty, &rtl_ext_item)) {
				pInfo->Cost     = rtl_ext_item.Cost;
				pInfo->Price    = rtl_ext_item.Price;
				pInfo->ExtPrice = rtl_ext_item.ExtPrice; // @v7.8.1
				pInfo->Expiry   = rtl_ext_item.Expiry;
				pInfo->ManufDtm = rtl_ext_item.ManufDtm; // @v7.5.1
				pInfo->QuotKindUsedForPrice    = rtl_ext_item.QuotKindUsedForPrice;
				pInfo->QuotKindUsedForExtPrice = rtl_ext_item.QuotKindUsedForExtPrice; // @v7.8.1
				SETFLAG(pInfo->Flags, pInfo->fDisabledQuot, rtl_ext_item.Flags & rtl_ext_item.fDisabledQuot);
			}
			else
				ok = -2;
		}
		pInfo->RevalPrice = pInfo->Price;
	}
	else
		ok = -1;
	return ok;
}
//
//
//
int SLAPI EditQuotVal(PPQuot * pQ, int quotCls)
{
	class SetQuotDialog : public TDialog {
	public:
		SetQuotDialog(int quotCls) : TDialog((quotCls == PPQuot::clsPredictCoeff) ? DLG_SETQUOTPC : DLG_SETQUOT), QuotCls(quotCls),
			UseQuot2(BIN(CConfig.Flags2 & CCFLG2_QUOT2))
		{
			PPObjQuotKind qk_obj;
			if(!qk_obj.CheckRights(QUOTRT_UPDQUOTS)) {
				SString temp_buf;
				PPLoadText(PPTXT_NOUPDRIGHTS, temp_buf);
				setStaticText(CTL_SETQUOT_ST_INFO, temp_buf);
			}
			if(UseQuot2)
				SetupCalPeriod(CTLCAL_SETQUOT_PERIOD, CTL_SETQUOT_PERIOD);
			else
				disableCtrl(CTL_SETQUOT_PERIOD, 1);
		}
		int    setDTS(const PPQuot * pData)
		{
			Data = *pData;
			ushort v = 0;
			if(Data.Flags & PPQuot::fPctOnCost)
				v = 1;
			else if(Data.Flags & PPQuot::fPctOnPrice)
				v = 2;
			else if(Data.Flags & PPQuot::fPctOnAddition)
				v = 3;
			else if(Data.Flags & PPQuot::fPctDisabled) {
				v = 4;
				Data.Quot = 0.0;
			}
			else if(Data.Flags & PPQuot::fPctOnBase)
				v = 5;
			else
				v = 0;
			setCtrlData(CTL_SETQUOT_HOW,  &v);
			AddClusterAssoc(CTL_SETQUOT_ZERO, 0, PPQuot::fZero);
			SetClusterData(CTL_SETQUOT_ZERO, Data.Flags);
			setCtrlReal(CTL_SETQUOT_VAL,  Data.Quot);
			setCtrlLong(CTL_SETQUOT_QTTY, Data.MinQtty);
			if(Data.Kind == PPQUOTK_BASE)
				DisableClusterItem(CTL_SETQUOT_HOW, 3, 1);
			if(UseQuot2)
				SetPeriodInput(this, CTL_SETQUOT_PERIOD, &Data.Period);
			SetupVal();
			return 1;
		}
		int    getDTS(PPQuot * pData)
		{
			int    ok = 1;
			ushort v = getCtrlUInt16(CTL_SETQUOT_HOW);
			Data.Flags = 0;
			if(v == 1)
				Data.Flags |= PPQuot::fPctOnCost;
			else if(v == 2)
				Data.Flags |= PPQuot::fPctOnPrice;
			else if(v == 3)
				Data.Flags |= PPQuot::fPctOnAddition;
			else if(v == 4)
				Data.Flags |= PPQuot::fPctDisabled;
			else if(v == 5)
				Data.Flags |= PPQuot::fPctOnBase;
			GetClusterData(CTL_SETQUOT_ZERO, &Data.Flags);
			if(Data.Flags & (PPQuot::fPctDisabled|PPQuot::fZero))
				Data.Quot = 0.0;
			else
				Data.Quot = getCtrlReal(CTL_SETQUOT_VAL);
			Data.MinQtty = getCtrlLong(CTL_SETQUOT_QTTY);
			if(Data.MinQtty < 0)
				Data.MinQtty = 0;
			if(UseQuot2)
				GetPeriodInput(this, CTL_SETQUOT_PERIOD, &Data.Period);
			if(v == 0)
				Data.Quot = fabs(Data.Quot);
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	private:
		void   SetupVal()
		{
			ushort v = getCtrlUInt16(CTL_SETQUOT_HOW);
			GetClusterData(CTL_SETQUOT_ZERO, &Data.Flags);
			if(Data.Flags & PPQuot::fZero || v == 4)
				disableCtrl(CTL_SETQUOT_VAL, 1);
			else
				disableCtrl(CTL_SETQUOT_VAL, 0);
		}
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isClusterClk(CTL_SETQUOT_ZERO) || event.isClusterClk(CTL_SETQUOT_HOW)) {
				SetupVal();
				clearEvent(event);
			}
		}
		const int QuotCls;
		const int UseQuot2;
		PPQuot Data;
	};
	DIALOG_PROC_BODY_P1(SetQuotDialog, quotCls, pQ);
}
//
// QuotListDialog
//
struct QuotListDlgParam {
	PPIDArray   * P_QuotKinds;
	PPQuotArray * P_QuotAry;
	QuotIdent   * P_Ident;
	double LastCost;
	double LastPrice;
	int    RightsForUpdate;
};

class QuotListDialog : public PPListDialog {
public:
	QuotListDialog(QuotListDlgParam & rParam) : PPListDialog(DLG_QUOTLIST, CTL_QUOTLIST_LIST), QuotKindsAry(*rParam.P_QuotKinds),
		P_Data(rParam.P_QuotAry), QIdent(*rParam.P_Ident), RightsForUpdate(rParam.RightsForUpdate), LastCost(rParam.LastCost), LastPrice(rParam.LastPrice)
	{
		enableCommand(cmaEdit,   rParam.RightsForUpdate);
		enableCommand(cmaDelete, rParam.RightsForUpdate);
		updateList(-1);
	}
	void   SetQuotKinds(const PPIDArray & rQuotKinds)
	{
		QuotKindsAry = rQuotKinds;
	}
	void   UpdateList(const QuotIdent & rIdent, double lastCost, double lastPrice);
private:
	virtual int  setupList();
	virtual int  editItem(long pos, long id);
	virtual int  delItem(long pos, long id);
	void      ReplyOnPressButton(HWND buttonWnd);

	PPObjGoods    GObj;
	PPQuotArray * P_Data;
	PPIDArray QuotKindsAry;
	QuotIdent QIdent;
	int    RightsForUpdate;
	double LastCost;
	double LastPrice;
};

void QuotListDialog::UpdateList(const QuotIdent & rIdent, double lastCost, double lastPrice)
{
	QIdent    = rIdent;
	LastCost  = lastCost;
	LastPrice = lastPrice;
	updateList(-1);
}

int QuotListDialog::setupList()
{
	int    ok = 1;
	StringSet  ss(SLBColumnDelim);
	PPQuot     quot;
	PPQuotKind qkr;
	PPObjQuotKind qk_obj;
	SString temp_buf, qk_text;
	for(uint i = 0; i < QuotKindsAry.getCount(); i++) {
		const   PPID qk_id = QuotKindsAry.get(i);
		double  price = 0.0;
		char    num_buf[32];
		if(qk_obj.Search(qk_id, &qkr) > 0)
			qk_text = qkr.Name;
		else
			ideqvalstr(qk_id, qk_text.Z());
		{
			QIdent.QuotKindID = qk_id;
			GObj.GetQuotExtByList(P_Data, QIdent, LastCost, LastPrice, &price);
			int    found = 0;
			LongArray period_list;
			LongArray qtty_list;
			for(uint j = 0; /*!found &&*/ j < P_Data->getCount(); j++) {
				const PPQuot & r_q = P_Data->at(j);
				if(r_q.Kind == qk_id && r_q.LocID == QIdent.LocID && r_q.ArID == QIdent.ArID && r_q.CurID == QIdent.CurID) {
					int32  period_idx = 0;
					Quotation2Core::PeriodToPeriodIdx(&r_q.Period, &period_idx);
					if(!period_list.lsearch(period_idx) || !qtty_list.lsearch(r_q.MinQtty)) {
						ss.clear();
						ss.add(qk_text);
						ss.add(r_q.PutValToStr(temp_buf.Z()));
						realfmt(price, MKSFMTD(10, 2, NMBF_NOZERO), num_buf);
						ss.add(num_buf);
						ss.add(temp_buf.Z().Cat(r_q.Period, 1));
						intfmt(r_q.MinQtty, MKSFMTD(0, 0, NMBF_NOZERO), num_buf);
						ss.add(num_buf);
						THROW(addStringToList(j+1, ss.getBuf()));
						found = 1;
					}
					period_list.add(period_idx);
					qtty_list.add(r_q.MinQtty);
				}
			}
			if(!found) {
				ss.clear();
				ss.add(qk_text);
				ss.add(temp_buf.Z());
				realfmt(price, MKSFMTD(10, 2, NMBF_NOZERO), num_buf);
				ss.add(num_buf);
				DateRange empty_period;
				ss.add(temp_buf.Z().Cat(empty_period.SetZero(), 1));
				intfmt(0, MKSFMTD(0, 0, NMBF_NOZERO), num_buf);
				ss.add(num_buf);
				THROW(addStringToList((1000000 + qk_id), ss.getBuf()));
			}
		}
		/*
		QIdent.QuotKindID = qk_id;
		P_Data->GetQuot(QIdent, &quot);
		ss.add(quot.PutValToStr(temp_buf));

		GObj.GetQuotExtByList(P_Data, QIdent, LastCost, LastPrice, &price);
		realfmt(price, MKSFMTD(10, 2, NMBF_NOZERO), num_buf);
		ss.add(num_buf);
		intfmt(quot.MinQtty, MKSFMTD(0, 0, NMBF_NOZERO), num_buf);
		ss.add(num_buf);
		if(!addStringToList(qk_id, ss.getBuf()))
			return 0;
		*/
	}
	CATCHZOK
	return ok;
}

void QuotListDialog::ReplyOnPressButton(HWND buttonWnd)
{
	SetFocus(GetDlgItem(H(), CTL_QUOTLIST_LIST));
	SendMessage(buttonWnd, BM_SETSTYLE, (WPARAM)(BS_TEXT|BS_PUSHBUTTON), MAKELPARAM(TRUE, 0));
}

int QuotListDialog::editItem(long pos, long id)
{
	int    ok = -1;
	HWND   b_wnd = GetFocus();
	PPID   qk_id = (id > 1000000) ? (id - 1000000) : 0;
	if(qk_id) {
		PPQuot quot;
		quot.Kind  = qk_id;
		quot.ArID  = QIdent.ArID;
		quot.LocID = QIdent.LocID;
		quot.CurID = QIdent.CurID;
		if(EditQuotVal(&quot, PPQuot::clsGeneral) > 0) {
			QuotIdent qi;
			qi = QIdent;
			qi.QuotKindID = quot.Kind;
			P_Data->SetQuot(qi, quot.Quot, quot.Flags, quot.MinQtty, &quot.Period);
			ok = 1;
		}
	}
	else if(id > 0 && id <= (long)P_Data->getCount()) {
		PPQuot quot = P_Data->at(id-1);
		if(EditQuotVal(&quot, PPQuot::clsGeneral) > 0) {
			QuotIdent qi;
			qi = QIdent;
			qi.QuotKindID = quot.Kind;
			P_Data->SetQuot(qi, quot.Quot, quot.Flags, quot.MinQtty, &quot.Period);
			ok = 1;
		}
	}
	ReplyOnPressButton(b_wnd);
	return ok;
}

int QuotListDialog::delItem(long pos, long id)
{
	int    ok = -1;
	HWND   b_wnd = GetFocus();
	if(id > 0 && id <= (long)P_Data->getCount()) {
		P_Data->atFree(id-1);
		ok = 1;
	}
	ReplyOnPressButton(b_wnd);
	return ok;
}
//
//
//
struct RankNNameEntry {
	int16 Rank;
	char  Name[48];
	PPID  ID;
};

IMPL_CMPFUNC(RankNName, i1, i2)
{
	int    cmp = 0;
	RankNNameEntry * k1 = (RankNNameEntry*)i1;
	RankNNameEntry * k2 = (RankNNameEntry*)i2;
	if(k1->ID == PPQUOTK_BASE && k2->ID != PPQUOTK_BASE)
		cmp = -1;
	else if(k1->ID != PPQUOTK_BASE && k2->ID == PPQUOTK_BASE)
		cmp = 1;
	else if(k1->Rank < k2->Rank)
		cmp = 1;
	else if(k1->Rank > k2->Rank)
		cmp = -1;
	else
		cmp = stricmp866(k1->Name, k2->Name);
	return cmp;
}

static const char * pViewQuotsAsListBox = "ViewQuotsAsListBox";

#define NUM_QUOTS_IN_DLG 10

class QuotationDialog : public EmbedDialog {
private:
	static int GetQuotDialogID(int quotCls)
	{
		if(quotCls == PPQuot::clsSupplDeal)
			return DLG_GQUOTSC;
		else if(quotCls == PPQuot::clsMtx)
			return DLG_GMQUOT;
		else if(quotCls == PPQuot::clsMtxRestr)
			return DLG_GMRQUOT;
		else if(quotCls == PPQuot::clsPredictCoeff)
			return DLG_GQUOTPC;
		else
			return DLG_GQUOT;
	}
	void SetupKinds(int onInit)
	{
		uint   i;
		SString temp_buf;
		memzero(Kinds, sizeof(Kinds));
		QuotKindsOrder.clear();
		if(Cls == PPQuot::clsSupplDeal) {
			Kinds[0] = Spc.SupplDealID;
			Kinds[1] = Spc.SupplDevUpID;
			Kinds[2] = Spc.SupplDevDnID;
			for(i = 0; i < 3; i++)
				disableCtrl(quotCtl(i), !RightsForUpdate || Kinds[i] == 0);
		}
		else if(Cls == PPQuot::clsMtx) {
			Kinds[0] = Spc.MtxID;
			disableCtrl(CTL_GQUOT_LOCLIST, /*!RightsForUpdate ||*/ !Spc.MtxID);
		}
		else if(Cls == PPQuot::clsMtxRestr) {
			Kinds[0] = Spc.MtxRestrID;
			disableCtrl(CTL_GQUOT_LOCLIST, /*!RightsForUpdate ||*/ !Spc.MtxRestrID);
		}
		else if(Cls == PPQuot::clsPredictCoeff) {
			Kinds[0] = Spc.PredictCoeffID;
			{
				SmartListBox * p_box = (SmartListBox *)getCtrlView(CTL_GQUOT_PRDLIST);
				SetupStrListBox(p_box);
			}
		}
		else { // PPQuot::clsGeneral
			Cls = PPQuot::clsGeneral; // @v9.3.10
			SArray qlist(sizeof(RankNNameEntry));
			PPQuotKind qkr;
			for(PPID qk = 0; QkObj.EnumItems(&qk, &qkr) > 0;) {
				if(qk != PPQUOTK_BASE && !Spc.IsSupplDealKind(qk) && !oneof3(qk, Spc.MtxID, Spc.MtxRestrID, Spc.PredictCoeffID) && oneof2(qkr.AccSheetID, 0, AccSheetID)) {
					RankNNameEntry e;
					e.Rank = qkr.Rank;
					STRNSCPY(e.Name, qkr.Name);
					e.ID = qkr.ID;
					qlist.ordInsert(&e, 0, PTR_CMPFUNC(RankNName));
				}
			}
			QuotKindsOrder.add(PPQUOTK_BASE);
			for(i = 0; i < qlist.getCount(); i++)
				QuotKindsOrder.add(((RankNNameEntry*)qlist.at(i))->ID);
			disableCtrl(CTL_GQUOT_BASE, !RightsForUpdate);
			for(i = 0; i < NUM_QUOTS_IN_DLG; i++) {
				int    disable_input = !RightsForUpdate;
				int    used_entry = 0;
				if(i < qlist.getCount()) {
					const RankNNameEntry * p_e = (RankNNameEntry*)qlist.at(i);
					if(p_e->ID != PPQUOTK_BASE) {
						Kinds[i] = p_e->ID;
						temp_buf = p_e->Name;
						size_t pad_size = (128-temp_buf.Len()-8) / 2 * 2;
						for(size_t j = 0; j < pad_size; j += 2)
							temp_buf.Space().Dot();
						used_entry = 1;
					}
				}
				if(!used_entry) {
					temp_buf.Z().Space();
					disable_input = 1;
				}
				setLabelText(quotCtl(i), temp_buf);
				disableCtrl(quotCtl(i), disable_input);
			}
			if(HasPeriodVal()) {
				ViewQuotsAsListBox = 1;
				disableCtrl(CTL_GQUOT_VIEW, 1);
			}
			else if(onInit) {
				WinRegKey reg_key(HKEY_CURRENT_USER, PPRegKeys::SysSettings, 1); // @v9.2.0 readonly 0-->1
				uint32 val = 0;
				if(reg_key.GetDWord(pViewQuotsAsListBox, &val) && val)
					ViewQuotsAsListBox = 1;
			}
			setCtrlData(CTL_GQUOT_VIEW, &ViewQuotsAsListBox);
		}
	}
public:
	//
	// ARG(category IN): 0 - обыкновенные котировки; 1 - контрактные цены; 2 - товарная матрица; 3 - ограничение по товарной матрице; 4 - поправочный коэффициент для прогноза продаж
	//
	QuotationDialog(PPID goodsID, PPID selLocID, PPID selCurID, PPID selArID, PPQuotArray * pAry, int quotCls, PPID accSheetID =0) :
		EmbedDialog(GetQuotDialogID(quotCls))
	{
		PPObjQuotKind::GetSpecialKinds(&Spc, 1);
		Cls = quotCls;
		GoodsID      = goodsID;
		SelLocID     = (selLocID > 0) ? selLocID : -1L;
		SelCurID     = (selCurID > 0) ? selCurID : 0L;
		SelArticleID = (selArID > 0 && oneof2(Cls, PPQuot::clsGeneral, PPQuot::clsSupplDeal)) ? selArID : 0L;
		LastCost = LastPrice = 0.0;
		getStaticText(CTL_GQUOT_LASTPRICES, FmtBuf);
		if(pAry) {
			Data.copy(*pAry);
			Data.GoodsID = pAry->GoodsID;
		}
		QuotKindsOrder.freeAll();
		ViewQuotsAsListBox = 0;
		RightsForUpdate = 1;
		AccSheetID = accSheetID;
		{
			//uint   i;
			//PPID   qk;
			//PPQuotKind    qkr;
			//PPObjQuotKind qk_obj;
			Goods2Tbl::Rec goods_rec;
			SString temp_buf;
			if(Cls == PPQuot::clsSupplDeal) {
				RightsForUpdate = QkObj.CheckRights(QUOTRT_UPDSUPPLCOST);
				SETIFZ(AccSheetID, GetSupplAccSheet());
			}
			else if(Cls == PPQuot::clsMtx)
				RightsForUpdate = QkObj.CheckRights(QUOTRT_UPDMTX);
			else if(Cls == PPQuot::clsMtxRestr)
				RightsForUpdate = QkObj.CheckRights(QUOTRT_UPDMTXRESTR);
			else if(Cls == PPQuot::clsPredictCoeff)
				RightsForUpdate = QkObj.CheckRights(QUOTRT_UPDQUOTS);
			else {
				RightsForUpdate = QkObj.CheckRights(QUOTRT_UPDQUOTS);
				SETIFZ(AccSheetID, GetSellAccSheet());
			}
			enableCommand(cmOK, RightsForUpdate);
			if(!RightsForUpdate) {
				PPLoadText(PPTXT_NOUPDRIGHTS, temp_buf);
				setStaticText(CTL_GQUOT_ST_INFO, temp_buf);
			}
			if(GObj.Fetch(GoodsID, &goods_rec) > 0) {
				setCtrlData(CTL_GQUOT_GOODS, goods_rec.Name);
				if(goods_rec.Kind == PPGDSK_GROUP) {
					enableCommand(cmaMore, 0);
					setLabelText(CTL_GQUOT_GOODS, PPGetWord(PPWORD_GROUP, 0, temp_buf));
				}
				if(!(goods_rec.Flags & GF_PRICEWOTAXES))
					setStaticText(CTL_GQUOT_GNOTE, 0);
			}
			disableCtrl(CTL_GQUOT_GOODS, 1);
			SetupPPObjCombo(this, CTLSEL_GQUOT_ACCSHEET, PPOBJ_ACCSHEET, AccSheetID, OLW_LOADDEFONOPEN, 0);
			disableCtrl(CTLSEL_GQUOT_ACCSHEET, (Cls == PPQuot::clsSupplDeal));
			SetupArCombo(this, CTLSEL_GQUOT_ARTICLE, SelArticleID, OLW_CANINSERT|OLW_LOADDEFONOPEN, AccSheetID, sacfDisableIfZeroSheet);
			disableCtrl(CTLSEL_GQUOT_ARTICLE, oneof3(Cls, PPQuot::clsMtx, PPQuot::clsMtxRestr, PPQuot::clsPredictCoeff));
			SetupCurrencyCombo(this, CTLSEL_GQUOT_CUR, (SelCurID >= 0) ? SelCurID : 0L, 0, 1, 0);
			//
			SetupKinds(1 /* onInit */);
			SetupLocList();
		}
	}
	int    getDTS(PPQuotArray *);
private:
	DECL_HANDLE_EVENT;
	void   getPage();
	void   setPage();
	void   updatePage();
	int    getQuotVal(uint ctlID, double *, long *);
	void   setupResultPrice(int idx);
	int    SetupLocList();
	int    SetupMatrixLocList(const StrAssocArray * pLocList, SmartListBox * pBox, long parentLocID, uint level);
	int    SetupMatrixLocEntry(const StrAssocArray::Item & rItem, SmartListBox * pBox, uint level);
	void   editMatrixItem(int autoToggle);
	virtual void setChildPos(uint neighbourCtl);
	void   showCtrlLabel(ushort ctlID, int s)
	{
		TLabel * p_label = getCtlLabel(ctlID);
		CALLPTRMEMB(p_label, Show(s));
	}
	int    HasPeriodVal() const
	{
		int    has_period_val = 0;
		for(uint i = 0; !has_period_val && i < Data.getCount(); i++) {
			if(!Data.at(i).Period.IsZero())
				has_period_val = 1;
		}
		return has_period_val;
	}
	uint   quotCtl(int i) const
	{
		if(Cls == PPQuot::clsSupplDeal)
			return (CTL_GQUOT_COST + i);
		else if(Cls == PPQuot::clsPredictCoeff)
			return CTL_GQUOT_COEFF;
		else if(oneof2(Cls, PPQuot::clsMtx, PPQuot::clsMtxRestr))
			return 0;
		else
			return (CTL_GQUOT_VAL1 + i * 3);
	}

	int    Cls;
	PPObjQuotKind::Special Spc;
	PPID   Kinds[NUM_QUOTS_IN_DLG];
	PPID   GoodsID;
	PPID   SelLocID;
	PPID   SelCurID;
	PPID   SelArticleID;
	PPID   AccSheetID;
	double LastCost;
	double LastPrice;
	PPQuotArray Data;
	PPObjQuotKind QkObj;
	PPObjGoods GObj;
	int    ViewQuotsAsListBox;
	int    RightsForUpdate;
	SString FmtBuf;
	PPIDArray  QuotKindsOrder;
	PPObjLocation LocObj;
};

int QuotationDialog::getDTS(PPQuotArray * pAry)
{
	int    ok = 1;
	if(ViewQuotsAsListBox)
		setPage();
	getPage();
	pAry->copy(Data);
	if(Cls == PPQuot::clsGeneral) {
		WinRegKey reg_key(HKEY_CURRENT_USER, PPRegKeys::SysSettings, 0);
		reg_key.PutDWord(pViewQuotsAsListBox, BIN(getCtrlLong(CTL_GQUOT_VIEW)));
	}
	return ok;
}

int QuotationDialog::getQuotVal(uint ctlID, double * pV, long * pF)
{
	int    ok = 0;
	if(ctlID) {
		char   temp_buf[128];
		temp_buf[0] = 0;
		if(getCtrlData(ctlID, temp_buf)) {
			PPQuot quot;
			quot.GetValFromStr(temp_buf);
			ASSIGN_PTR(pV, quot.Quot);
			ASSIGN_PTR(pF, quot.Flags);
			ok = quot.IsEmpty() ? -1 : 1;
		}
	}
	return ok;
}

int QuotationDialog::SetupLocList()
{
	// @v8.1.9 {
	PPIDArray ext_loc_list;
	LocationTbl::Rec loc_rec;
	for(uint i = 0; i < Data.getCount(); i++) {
		const PPQuot & r_item = Data.at(i);
		if(r_item.LocID && LocObj.Fetch(r_item.LocID, &loc_rec) > 0 && loc_rec.Type == LOCTYP_ADDRESS)
			ext_loc_list.add(r_item.LocID);
	}
	ext_loc_list.sortAndUndup();
	// } @v8.1.9
	LocationFilt loc_filt(LOCTYP_WAREHOUSE);
	if(ext_loc_list.getCount())
		loc_filt.ExtLocList.Set(&ext_loc_list);
	StrAssocArray * p_list = LocObj.MakeList_(&loc_filt, MAXLONG);
	SmartListBox * p_box = (SmartListBox *)getCtrlView(CTL_GQUOT_LOCLIST);
	long   cur_foc_pos = (p_box && p_box->def) ? p_box->def->_curItem() : -1;
	if(oneof2(Cls, PPQuot::clsMtx, PPQuot::clsMtxRestr)) {
		SetupStrListBox(p_box);
		SetupMatrixLocList(p_list, p_box, MAXLONG, 0);
	}
	else {
		SetupTreeListBox(this, CTL_GQUOT_LOCLIST, p_list, lbtDisposeData|lbtDblClkNotify|lbtFocNotify, 0);
	}
	if(SelLocID < 0)
		SelLocID = 0;
	setPage();
	if(cur_foc_pos < 0)
		p_box->TransmitData(+1, &SelLocID);
	else
		p_box->focusItem(cur_foc_pos);
	p_box->Draw_();
	selectCtrl(CTL_GQUOT_LOCLIST);
	return 1;
}

int QuotationDialog::SetupMatrixLocEntry(const StrAssocArray::Item & rItem, SmartListBox * pBox, uint level)
{
	int    ok = 1;
	const  PPID loc_id = (rItem.Id == MAXLONG) ? 0 : rItem.Id;
	StringSet ss(SLBColumnDelim);
	SString buf, buf2;
	buf.CatCharN('.', level * 4).Cat(rItem.Txt);
	ss.add(buf);

	PPQuot quot;
	const QuotIdent qi(loc_id, Kinds[0], SelCurID, SelArticleID);
	Data.GetQuot(qi, &quot);
	buf.Z();
	const double _q = round(quot.Quot, 0);
	if(_q != 0.0) {
		if(Cls == PPQuot::clsMtx) {
			PPLoadString((_q < 0.0) ? "no" : "yes", buf);
		}
		else
			buf.Cat((long)quot.Quot);
	}
	else if(Cls == PPQuot::clsMtx) {
		// @v9.4.9 QuotIdent qi(loc_id, Kinds[0], SelCurID, SelArticleID);
		int    r = 0;
		double result = 0.0;
		Goods2Tbl::Rec goods_rec;
		if(Data.GetResult(qi, 0.0, 0.0, &result) > 0) {
			r = 1;
		}
		else if(Data.GoodsID && GObj.Fetch(Data.GoodsID, &goods_rec) > 0 && goods_rec.ParentID) {
			if(GObj.GetQuotExt(goods_rec.ParentID, qi, &result, 1) > 0)
				r = 1;
		}
		if(r > 0 && result != 0.0) {
			PPLoadString((result < 0.0) ? "no" : "yes", buf2);
			buf.Space().CatParStr(buf2);
		}
	}
	ss.add(buf);
	{
		long   max_count = 0, count = 0;
		PPID   grp_id = 0;
		if(Cls == PPQuot::clsMtx)
			GObj.GetMatrixRestrict(Data.GoodsID, loc_id, 0, &grp_id, &max_count);
		else {
			max_count = (long)quot.Quot;
			grp_id = Data.GoodsID;
		}
		if(max_count) {
			PPIDArray goods_list;
			GoodsFilt gf(grp_id);
			gf.LocList.Add(loc_id);
			gf.Flags |= GoodsFilt::fRestrictByMatrix;
			GoodsIterator::GetListByFilt(&gf, &goods_list);
			count = max_count - goods_list.getCount();
			if(Cls == PPQuot::clsMtx) {
				buf.Z().Cat(max_count);
				ss.add(buf);
			}
		}
		if(count)
			buf.Cat(count);
		ss.add(buf);
	}
	pBox->addItem(loc_id, ss.getBuf());
	return ok;
}


int QuotationDialog::SetupMatrixLocList(const StrAssocArray * pLocList, SmartListBox * pBox, long parentLocID, uint level)
{
	int    ok = 1;
	if(pLocList && pBox) {
		uint pos = 0;
		if(level == 0 && pLocList->Search(parentLocID, &pos))
			THROW(SetupMatrixLocEntry(pLocList->Get(pos), pBox, level));
		for(uint i = 0; i < pLocList->getCount(); i++) {
			const StrAssocArray::Item & r_item = pLocList->Get(i);
			if(r_item.ParentId == parentLocID) {
				THROW(SetupMatrixLocEntry(r_item, pBox, level+1));
				THROW(SetupMatrixLocList(pLocList, pBox, r_item.Id, level+2)); // @recursion
			}
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

void QuotationDialog::editMatrixItem(int autoToggle)
{
	if(RightsForUpdate) {
		int    r = 0;
		SString title;
		PPQuot quot;
		QuotIdent qi(SelLocID, Kinds[0], SelCurID, SelArticleID);
		Data.GetQuot(qi, &quot);
		if(Cls == PPQuot::clsMtxRestr) {
			SString buf;
			PPLoadText(PPTXT_GOODSMATRIXRESTR, title);
			PPInputStringDialogParam isd_param(title, title);
			buf.Cat((long)quot.Quot);
			if((r = InputStringDialog(&isd_param, buf)) > 0) {
				quot.Quot = (buf.ToLong() >= 0) ? buf.ToLong() : 0;
				r = 1;
			}
		}
		else {
			PPLoadText(PPTXT_INCLUDEINGOODSMATRIX, title);
			const double _q = round(quot.Quot, 0);
			if(autoToggle) {
				quot.Quot = (_q < 0.0) ? 0.0 : ((_q > 0.0) ? -1.0 : 1.0);
				r = 1;
			}
			else {
				uint yes_no = (_q > 0.0) ? 2 : ((_q < 0.0) ? 1 : 0);
				if((r = SelectorDialog(DLG_YESNO, CTL_YESNO_YESNO, &yes_no, title)) > 0) {
					quot.Quot = (yes_no == 1) ? -1.0 : ((yes_no == 2) ? 1.0 : 0.0);
					r = 1;
				}
			}
		}
		if(r > 0) {
			Data.SetQuot(qi, quot.Quot, 0, 0, 0 /* period */);
			SetupLocList();
		}
	}
}

IMPL_HANDLE_EVENT(QuotationDialog)
{
	TDialog::handleEvent(event);
	if(event.isCmd(cmLBItemFocused))
		updatePage();
	else if(event.isCmd(cmaEdit) || event.isCmd(cmLBDblClk)) {
		if(oneof2(Cls, PPQuot::clsMtx, PPQuot::clsMtxRestr)) {
			if(event.isCmd(cmaEdit) || event.isCtlEvent(CTL_GQUOT_LOCLIST)) {
				editMatrixItem(0);
				clearEvent(event);
			}
		}
		else if(Cls == PPQuot::clsPredictCoeff) {
			if(event.isCmd(cmaEdit) || event.isCtlEvent(CTL_GQUOT_PRDLIST)) {
				long   i = 0;
				SmartListBox * p_box = (SmartListBox *)getCtrlView(CTL_GQUOT_PRDLIST);
				if(p_box && p_box->def && p_box->getCurID(&i) && i > 0 && i <= (long)Data.getCount()) {
					PPQuot quot = Data.at(i-1); // @copy
					if(quot.Kind == Spc.PredictCoeffID && EditQuotVal(&quot, Cls) > 0) {
						const QuotIdent qident(quot.LocID, Spc.PredictCoeffID, 0, quot.ArID);
						int    p = Data.SetQuot(qident, quot.Quot, quot.Flags, 0, &quot.Period);
						if(p > 0)
							updatePage();
					}
				}
				clearEvent(event);
			}
		}
	}
	if(event.isCmd(cmaInsert)) {
		getCtrlData(CTLSEL_GQUOT_CUR,     &SelCurID);
		getCtrlData(CTLSEL_GQUOT_ARTICLE, &SelArticleID);
		if(Cls == PPQuot::clsPredictCoeff) {
			PPQuot quot;
			quot.Kind  = Spc.PredictCoeffID;
			quot.ArID  = SelArticleID;
			quot.LocID = SelLocID;
			quot.CurID = 0;
			if(EditQuotVal(&quot, Cls) > 0) {
				const QuotIdent qident(SelLocID, Spc.PredictCoeffID, 0, SelArticleID);
				int    p = Data.SetQuot(qident, quot.Quot, quot.Flags, 0, &quot.Period);
				if(p > 0) {
					updatePage();
				}
			}
		}
	}
	else if(event.isCmd(cmaDelete)) {
		if(Cls == PPQuot::clsPredictCoeff) {
			long   i = 0;
			SmartListBox * p_box = (SmartListBox *)getCtrlView(CTL_GQUOT_PRDLIST);
			if(p_box && p_box->def && p_box->getCurID(&i) && i > 0 && i <= (long)Data.getCount()) {
				const PPQuot & r_q = Data.at(i-1);
				if(r_q.Kind == Spc.PredictCoeffID) {
					Data.atFree(i-1);
					updatePage();
				}
			}
		}
	}
	else if(event.isCmd(cmaMore)) {
		if(GoodsID)
			ViewLots(GoodsID, ((SelLocID >= 0) ? SelLocID : LConfig.Location), (Cls == PPQuot::clsSupplDeal) ? SelArticleID : 0, 0, 0);
	}
	else if(event.isCmd(cmGrpQuot)) {
		PPID   grp_id = 0;
		if(GObj.GetParentID(GoodsID, &grp_id) > 0 && grp_id) {
			if(GObj.EditQuotations(grp_id, SelLocID, SelCurID, SelArticleID, Cls, 1) > 0) {
				if(oneof2(Cls, PPQuot::clsMtx, PPQuot::clsMtxRestr))
					SetupLocList();
				else
					updatePage();
			}
		}
	}
	else if(event.isClusterClk(CTL_GQUOT_VIEW)) {
		const ushort quot_view_val = HasPeriodVal() ? 1 : getCtrlUInt16(CTL_GQUOT_VIEW);
		if(ViewQuotsAsListBox != quot_view_val) {
			ViewQuotsAsListBox = quot_view_val;
			updatePage();
		}
	}
	else if(event.isCbSelected(CTLSEL_GQUOT_ACCSHEET)) {
		const PPID acc_sheet_id = getCtrlLong(CTLSEL_GQUOT_ACCSHEET);
		if(Cls != PPQuot::clsSupplDeal && acc_sheet_id != AccSheetID) {
			AccSheetID = acc_sheet_id;
			SelArticleID = 0;
			SetupArCombo(this, CTLSEL_GQUOT_ARTICLE, SelArticleID, OLW_CANINSERT|OLW_LOADDEFONOPEN, AccSheetID, sacfDisableIfZeroSheet);
			// @v9.3.10 {
			SetupKinds(0);
			updatePage();
			// } @v9.3.10
		}
	}
	else if(event.isCbSelected(CTLSEL_GQUOT_ARTICLE))
		updatePage();
	else if(event.isCbSelected(CTLSEL_GQUOT_CUR)) {
		DS.GetTLA().Lid.QuotCurID = getCtrlLong(CTLSEL_GQUOT_CUR);
		if(oneof2(Cls, PPQuot::clsMtx, PPQuot::clsMtxRestr))
			SetupLocList();
		else
			updatePage();
	}
	else if(TVKEYDOWN) {
		if(TVKEY == kbF2 && P_Current && !oneof2(Cls, PPQuot::clsMtx, PPQuot::clsMtxRestr)) {
			uint   ctl_id = 0;
			int    idx = -1;
			if(isCurrCtlID(CTL_GQUOT_BASE)) {
				ctl_id = CTL_GQUOT_BASE;
				idx = -1;
			}
			else {
				for(int i = 0; !ctl_id && i < NUM_QUOTS_IN_DLG; i++)
					if(isCurrCtlID(quotCtl(i))) {
						ctl_id = quotCtl(i);
						idx = i;
					}
			}
			if(ctl_id) {
				PPQuot quot;
				if(getQuotVal(ctl_id, &quot.Quot, &quot.Flags) && EditQuotVal(&quot, Cls) > 0) {
					SString temp_buf;
					setCtrlString(ctl_id, quot.PutValToStr(temp_buf));
					QuotIdent qi(SelLocID, (idx < 0) ? PPQUOTK_BASE : Kinds[idx], SelCurID, SelArticleID);
					Data.SetQuot(qi, quot.Quot, quot.Flags, quot.MinQtty, &quot.Period);
					setupResultPrice(idx);
				}
			}
		}
		else if(TVKEY == kbF3 && P_Current) {
			PPID   qk_id = 0;
			if(Cls == PPQuot::clsMtx)
				qk_id = GObj.GetConfig().MtxQkID;
			else if(Cls == PPQuot::clsMtxRestr)
				qk_id = GObj.GetConfig().MtxRestrQkID;
			else if(isCurrCtlID(CTL_GQUOT_BASE))
				qk_id = PPQUOTK_BASE;
			else {
				for(int i = 0; !qk_id && i < NUM_QUOTS_IN_DLG; i++)
					if(isCurrCtlID(quotCtl(i)))
						qk_id = Kinds[i];
			}
			if(qk_id) {
				QkObj.Edit(&qk_id, 0);
			}
		}
		else if(TVKEY == kbSpace && oneof2(Cls, PPQuot::clsMtx, PPQuot::clsMtxRestr)) {
			if(isCurrCtlID(CTL_GQUOT_LOCLIST))
				editMatrixItem(1);
		}
		else
			return;
	}
	else
		return;
	clearEvent(event);
}

void QuotationDialog::getPage()
{
	if(SelLocID >= 0 && !ViewQuotsAsListBox && !oneof3(Cls, PPQuot::clsMtx, PPQuot::clsMtxRestr, PPQuot::clsPredictCoeff)) {
		double quot = 0.0;
		long   flags = 0;
		QuotIdent qi(SelLocID, PPQUOTK_BASE, SelCurID, SelArticleID);
		int    r = getQuotVal(CTL_GQUOT_BASE, &quot, &flags);
		if(r > 0) {
			Data.SetQuot(qi, quot, flags, 0, 0 /* period */); // @v7.9.0 minQuot -1-->0
		}
		// @v7.5.0 {
		else if(r < 0) {
			Data.SetQuot(qi, 0.0, 0, 0, 0 /* period */); // @v7.9.0 minQuot -1-->0
		}
		// } @v7.5.0
		for(int i = 0; i < NUM_QUOTS_IN_DLG; i++) {
			quot = 0.0;
			flags = 0;
			if(Kinds[i]) {
				r = getQuotVal(quotCtl(i), &quot, &flags);
				qi.QuotKindID = Kinds[i];
				if(r > 0) {
					Data.SetQuot(qi, quot, flags, 0 /* minQtty */, 0 /* period */); // @v7.9.0 minQuot -1-->0
				}
				// @v7.5.0 {
				else if(r < 0) {
					Data.SetQuot(qi, 0.0, 0, 0 /* minQtty */, 0 /* period */); // @v7.9.0 minQuot -1-->0
				}
				// } @v7.5.0
			}
		}
	}
}

void QuotationDialog::setupResultPrice(int i)
{
	if(Cls == PPQuot::clsGeneral) {
		double price = 0.0;
		const  QuotIdent qi(SelLocID, (i >= 0) ? Kinds[i] : PPQUOTK_BASE, SelCurID, SelArticleID);
		GObj.GetQuotExtByList(&Data, qi, LastCost, LastPrice, &price);
		if(i >= 0)
			setCtrlData(quotCtl(i)+2, &price);
		else
			setCtrlData(CTL_GQUOT_BASE+2, &price);
	}
}

void QuotationDialog::setPage()
{
	if(SelLocID >= 0 && !oneof3(Cls, PPQuot::clsMtx, PPQuot::clsMtxRestr, PPQuot::clsPredictCoeff)) {
		SString temp_buf;
		ReceiptTbl::Rec lot_rec;
		double price = 0.0;
		if(::GetCurGoodsPrice(GoodsID, SelLocID, GPRET_INDEF, &price, &lot_rec) > 0) {
			LastCost  = R5(lot_rec.Cost);
			LastPrice = R5(lot_rec.Price);
			const double disp_cost = BillObj->CheckRights(BILLRT_ACCSCOST) ? LastCost : 0.0;
			temp_buf.Printf(FmtBuf, disp_cost, LastPrice);
		}
		else
			LastCost = LastPrice = 0.0;
		setStaticText(CTL_GQUOT_LASTPRICES, temp_buf);
		if(!ViewQuotsAsListBox) {
			QuotIdent qi(SelLocID, PPQUOTK_BASE, SelCurID, SelArticleID);
			{
				PPQuot q;
				Data.GetQuot(qi, &q);
				setCtrlString(CTL_GQUOT_BASE, q.PutValToStr(temp_buf));
				setupResultPrice(-1);
			}
			for(int i = 0; i < NUM_QUOTS_IN_DLG; i++) {
				if(Kinds[i]) {
					PPQuot q;
					qi.QuotKindID = Kinds[i];
					Data.GetQuot(qi, &q);
					const uint ctl_id = quotCtl(i);
					if(ctl_id)
						setCtrlString(ctl_id, q.PutValToStr(temp_buf));
					setupResultPrice(i);
				}
			}
		}
	}
}

void QuotationDialog::setChildPos(uint neighbourCtl)
{
	if(P_ChildDlg && P_ChildDlg->H()) {
		RECT   child_rect, ctl_rect, dlg_rect;
		long   parent_style = TView::GetWindowStyle(H());
		GetWindowRect(GetDlgItem(H(), neighbourCtl), &ctl_rect);
		GetWindowRect(H(), &dlg_rect);
		child_rect.left   = ctl_rect.left - dlg_rect.left - GetSystemMetrics(SM_CXEDGE);
		child_rect.top    = ctl_rect.top - dlg_rect.top - GetSystemMetrics(SM_CYBORDER) -
			GetSystemMetrics(SM_CYEDGE) - (((parent_style & WS_CAPTION) == WS_CAPTION) ? GetSystemMetrics(SM_CYCAPTION) : 0);
		child_rect.bottom = ctl_rect.bottom - ctl_rect.top;
		child_rect.right  = ctl_rect.right - ctl_rect.left;
		MoveWindow(P_ChildDlg->H(), child_rect.left, child_rect.top, child_rect.right, child_rect.bottom, 1);
		APPL->SetWindowViewByKind(P_ChildDlg->H(), TProgram::wndtypChildDialog);
		ShowWindow(P_ChildDlg->H(), SW_SHOWNORMAL);
	}
	SetFocus(GetDlgItem(H(), MAKE_BUTTON_ID(CTL_GQUOT_VIEW, 1)));
}

void QuotationDialog::updatePage()
{
	if(!P_ChildDlg)
		getPage();
	SelLocID = getCtrlLong(CTL_GQUOT_LOCLIST);
	if(SelLocID == MAXLONG)
		SelLocID = 0;
	getCtrlData(CTLSEL_GQUOT_CUR,     &SelCurID);
	getCtrlData(CTLSEL_GQUOT_ARTICLE, &SelArticleID);
	setPage();
	if(Cls == PPQuot::clsPredictCoeff) {
		SmartListBox * p_box = (SmartListBox *)getCtrlView(CTL_GQUOT_PRDLIST);
		if(p_box) {
			p_box->freeAll();
			double effect_val = 0.0;
			QuotIdent qident(SelLocID, Spc.PredictCoeffID, 0, SelArticleID);
			qident.Dt = getcurdate_();
			GObj.GetQuotExtByList(&Data, qident, 0.0, 0.0, &effect_val);

			SString temp_buf;
			StringSet ss(SLBColumnDelim);
			for(uint i = 0; i < Data.getCount(); i++) {
				ss.clear();
				const PPQuot & r_q = Data.at(i);
				if(r_q.Kind == Spc.PredictCoeffID && r_q.LocID == SelLocID && !r_q.Period.IsZero()) {
					ss.add(temp_buf.Z().Cat(r_q.Period, 1));
					r_q.PutValToStr(temp_buf.Z());
					ss.add(temp_buf);
					ss.add(temp_buf.Z().Cat(effect_val, MKSFMTD(0, 5, NMBF_NOZERO|NMBF_NOTRAILZ)));
					p_box->addItem(i+1, ss.getBuf());
				}
			}
			p_box->Draw_();
		}
	}
	else {
		QuotIdent qident(SelLocID, PPQUOTK_BASE, SelCurID, SelArticleID);
		if((P_ChildDlg && ViewQuotsAsListBox) || (!P_ChildDlg && !ViewQuotsAsListBox)) {
			QuotListDialog * p_inner_dlg = (QuotListDialog *)P_ChildDlg;
			if(p_inner_dlg) {
				p_inner_dlg->SetQuotKinds(QuotKindsOrder); // @v9.3.10
				p_inner_dlg->UpdateList(qident, LastCost, LastPrice);
			}
		}
		else {
			TDialog * dlg = 0;
			if(ViewQuotsAsListBox) {
				QuotListDlgParam  qld_param;
				qld_param.P_QuotKinds = &QuotKindsOrder;
				qld_param.P_QuotAry   = &Data;
				qld_param.P_Ident     = &qident;
				qld_param.LastCost    = LastCost;
				qld_param.LastPrice   = LastPrice;
				qld_param.RightsForUpdate = RightsForUpdate;
				dlg = new QuotListDialog(qld_param);
			}
			Embed(dlg);
			setChildPos(CTL_GQUOT_EMBED_WINDOW);
			{
				const int show = !ViewQuotsAsListBox;
				for(uint ctrl_id = CTL_GQUOT_BASE, i = 0; i <= NUM_QUOTS_IN_DLG; i++) {
					showCtrl(ctrl_id, show);
					showCtrlLabel(ctrl_id++, show);
					showCtrl(ctrl_id++, show);
					showCtrl(ctrl_id++, show);
				}
			}
		}
	}
}

int SLAPI PPObjGoods::EditQuotations(PPID id, PPID initLocID, PPID initCurID, PPID initArID, int quotCls, int toCascade, PPID accSheetID /*=0*/)
{
	int    ok = -1, valid_data = 0;
	// @debug int    corrected = 0;
	QuotationDialog * dlg = 0;
	if(id) {
		PPQuotArray qary(id);
		THROW(GetQuotList(id, 0, qary));
		// @debug {
		/*
		if(qary.Correct721(0) > 0) {
			corrected = 1;
		}
		*/
		// } @debug
		if(initCurID < 0)
			initCurID = DS.GetTLA().Lid.QuotCurID;
		THROW(CheckDialogPtr(&(dlg = new QuotationDialog(id, initLocID, initCurID, initArID, &qary, quotCls, accSheetID))));
		if(toCascade)
			dlg->ToCascade();
		qary.DebugLog(); // @debug
		while(!valid_data && ExecView(dlg) == cmOK) {
			if(dlg->getDTS(&qary) > 0) {
				qary.DebugLog(); // @debug
				ok = valid_data = 1;
			}
			else
				PPError();
		}
		if(ok > 0)
			THROW(P_Tbl->SetQuotList(qary, 1));
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int SLAPI PPObjGoods::GetMatrixRestrict(PPID goodsID, PPID locID, int srchNearest, PPID * pGoodsGrpID, long * pResult)
{
	int    ok = -1, r;
	PPID   grp_id = goodsID;
	Goods2Tbl::Rec goods_rec;
	if((r = P_Tbl->GetMatrixRestrict(GetConfig().MtxRestrQkID, grp_id, locID, srchNearest, pResult)) > 0)
		ok = r;
	else if(Fetch(goodsID, &goods_rec) > 0 && goods_rec.ParentID) {
		if((r = GetMatrixRestrict(goods_rec.ParentID, locID, srchNearest, &grp_id, pResult)) > 0) // @recursion
			ok = r;
	}
	ASSIGN_PTR(pGoodsGrpID, grp_id);
	return ok;
}

int SLAPI PPObjGoods::CheckMatrixRestrict(PPID goodsID, PPID locID, long restrict)
{
	int    ok = 1;
	long   max_count = 0;
	PPID   grp_id = 0;
	if(restrict > 0 && GetMatrixRestrict(goodsID, locID, 1, &grp_id, &max_count) > 0 && grp_id) {
		uint   count = 0;
		PPIDArray goods_list;
		GoodsFilt gf(grp_id);
		gf.LocList.Add(locID);
		gf.Flags |= GoodsFilt::fRestrictByMatrix;
		GoodsIterator::GetListByFilt(&gf, &goods_list);
		count = goods_list.getCount();
		if(restrict > 0)
			count = BelongToMatrix(goodsID, locID) ? count : (count+1);
		else if(GetParentID(goodsID, &grp_id) > 0 && grp_id)
			count = BelongToMatrix(grp_id, locID) ? count : 0;
		else
			count = 0;
		if(count > (uint)max_count) {
			SString add_msg, goods_name, grp_name, loc_name, word_goods, word_grp, word_loc;
			GetLocationName(locID, loc_name);
			GetGoodsName(grp_id,  grp_name);
			GetGoodsName(goodsID, goods_name);
			// @v9.0.2 PPGetWord(PPWORD_GOODS,     0, word_goods);
			PPLoadString("ware", word_goods); // @v9.0.2
			PPGetWord(PPWORD_GROUP,     0, word_grp);
			// @v9.0.2 PPGetWord(PPWORD_WAREHOUSE, 0, word_loc);
			PPLoadString("warehouse", word_loc); // @v9.0.2
			add_msg.CatEq(word_goods, goods_name).CatDiv(',', 2).CatEq(word_grp, grp_name).CatDiv(',', 2).CatEq(word_loc, loc_name);
			CALLEXCEPT_PP_S(PPERR_MATRIXRESTRICTION, add_msg);
		}
	}
	CATCH
		ok = (PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_LASTERR|LOGMSGF_DBINFO), 0);
	ENDCATCH
	return ok;
}

int SLAPI PPSupplDeal::CheckCost(double c) const
{
	int    ok = 1;
	if(c > 0.0 && Cost > 0.0) {
		if(c > Cost) {
			const double p = 100.0 * (c - Cost) / Cost;
			if(p > UpDev)
				ok = 0;
		}
		else if(c < Cost) {
			const double p = 100.0 * (Cost - c) / Cost;
			if(p > DnDev)
				ok = 0;
		}
	}
	return ok;
}

SString & FASTCALL PPSupplDeal::Format(SString & rBuf) const
{
	rBuf.Z();
	if(IsDisabled) {
		rBuf.CatChar('X');
	}
	else {
		rBuf.Cat(Cost, SFMT_MONEY).Space().CatChar('(').
			CatChar('+').Cat(UpDev, MKSFMTD(0, 1, 0)).CatChar('%').Semicol().Space().
			CatChar('-').Cat(DnDev, MKSFMTD(0, 1, 0)).CatChar('%').CatChar(')');
	}
	return rBuf;
}

int SLAPI PPObjGoods::GetSupplDeal(PPID goodsID, const QuotIdent & rQi, PPSupplDeal * pResult, int useCache)
{
	int    ok = -1, r;
	const  PPThreadLocalArea & r_tla = DS.GetConstTLA();
	PPID   qk_id = r_tla.SupplDealQuotKindID;
	double val = 0.0;
	QuotIdent qi = rQi;
	memzero(pResult, sizeof(*pResult));
	if(qk_id) {
		qi.QuotKindID = qk_id;
		THROW(r = GetQuotExt(goodsID, qi, &val, useCache));
		if(r == 1)
			pResult->Cost = val;
		else if(r == 2)
			pResult->IsDisabled = 1;
		ok = r;
	}
	if(!pResult->IsDisabled) {
		qk_id = r_tla.SupplDevUpQuotKindID;
		if(qk_id) {
			qi.QuotKindID = qk_id;
			THROW(GetQuotExt(goodsID, qi, &(val = 0), useCache));
			pResult->UpDev = val;
		}
		qk_id = r_tla.SupplDevDnQuotKindID;
		if(qk_id) {
			qi.QuotKindID = qk_id;
			THROW(GetQuotExt(goodsID, qi, &(val = 0), useCache));
			pResult->DnDev = val;
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjGoods::SetSupplDeal(PPID goodsID, const QuotIdent & rQi, const PPSupplDeal * pDeal, int useTa)
{
	int    ok = 1;
	long   flags = 0;
	double cost = 0.0;
	PPID   quot_id = 0;
	const  PPThreadLocalArea & r_tla = DS.GetConstTLA();
	const  PPID deal_qk_id = r_tla.SupplDealQuotKindID;
	const  PPID dn_qk_id   = r_tla.SupplDevDnQuotKindID;
	const  PPID up_qk_id   = r_tla.SupplDevUpQuotKindID;
	QuotIdent qi;
	PPQuotArray qary;
	THROW_INVARG(pDeal);
	THROW_PP(deal_qk_id, PPERR_UNDEFSUPPLDEAL);
	THROW_PP(dn_qk_id, PPERR_UNDEFSUPPLDEALLOWDEV);
	THROW_PP(up_qk_id, PPERR_UNDEFSUPPLDEALUPPDEV);
	if(pDeal->IsDisabled) {
		cost = 0.0;
		flags |= PPQuot::fPctDisabled;
	}
	else
		cost = pDeal->Cost;
	THROW(GetQuotList(goodsID, 0, qary));
	qi = rQi;
	qi.QuotKindID = deal_qk_id;
	THROW(qary.SetQuot(qi, cost, flags, 0, 0 /* period */)); // @v7.9.0 minQuot -1-->0
	qi.QuotKindID = dn_qk_id;
	THROW(qary.SetQuot(qi, pDeal->DnDev, 0, 0, 0 /* period */)); // @v7.9.0 minQuot -1-->0
	qi.QuotKindID = up_qk_id;
	THROW(qary.SetQuot(qi, pDeal->UpDev, 0, 0, 0 /* period */)); // @v7.9.0 minQuot -1-->0
	THROW(P_Tbl->SetQuotList(qary, useTa));
	CATCHZOK
	return ok;
}

int SLAPI PPObjGoods::GetQuotExt(PPID goodsID, const QuotIdent & rQi, double cost, double price, double * pResult, int useCache)
{
	int    ok = 0;
	if(useCache == 1000 && pResult) {
		ok = Helper_GetQuotExt(goodsID, rQi, cost, price, pResult, 1);
		/* @v8.3.3
		if(ok > 0) {
			double uncached_result = 0.0;
			int    r = Helper_GetQuotExt(goodsID, rQi, cost, price, &uncached_result, 0);
			if(!feqeps(uncached_result, *pResult, 1E-6)) { // @v8.2.11 @fix feqeps-->!feqeps
				SString msg_buf;
				PPFormatT(PPTXT_CACHEDQUOTCONFLICT, &msg_buf, rQi.QuotKindID, goodsID, *pResult, uncached_result);
				PPLogMessage(PPFILNAM_INFO_LOG, msg_buf, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
				*pResult = uncached_result;
				ok = r;
			}
		}
		*/
	}
	else
		ok = Helper_GetQuotExt(goodsID, rQi, cost, price, pResult, useCache);
	return ok;
}

int SLAPI PPObjGoods::Helper_GetQuotExt(PPID goodsID, const QuotIdent & rQi, double cost, double price, double * pResult, int useCache)
{
	// @>>PPObjGoods::GetQuot >> GoodsCore::GetQuot >>
	//    QuotationCore::GetCurr >> QuotationCore::GetCurrList
	//
	//
	int    ok = -1;
	Goods2Tbl::Rec goods_rec;
	int    r = P_Tbl->GetQuot(goodsID, rQi, cost, price, pResult, useCache);
	if(r > 0)
		ok = r;
	else if(Fetch(goodsID, &goods_rec) > 0 && goods_rec.ParentID) {
		//
		// Рекурсивный поиск котировки.
		// Для случая определения значения котировки относительно базовой совершаем небольшой трюк:
		// находим базовую котировку для изначально заданной позиции и вставляем ее в QuotIdent.
		// Теперь, если относительная базовой котировка определена в вышестоящей группе, то
		// при расчете эффективного значения она будет использовать оригинальное значение базы (если таковое определено).
		//
		if(rQi.QuotKindID == PPQUOTK_BASE || rQi.GetPrevBase() > 0.0) {
			if((r = GetQuotExt(goods_rec.ParentID, rQi, cost, price, pResult, useCache)) > 0) // @recursion
				ok = r;
		}
		else {
			QuotIdent tmpi = rQi;
			QuotIdent qi = rQi;
			qi.QuotKindID = PPQUOTK_BASE;
			double base = 0.0;
			if(P_Tbl->GetQuot(goodsID, qi, cost, price, &base, useCache) > 0)
				tmpi.SetBase(base);
			if((r = GetQuotExt(goods_rec.ParentID, tmpi, cost, price, pResult, useCache)) > 0) // @recursion
				ok = r;
		}
	}
	else if(rQi.QuotKindID) {
		PPObjQuotKind qk_obj;
		if(rQi.CurID) {
			QuotIdent qi = rQi;
			qi.QuotKindID = PPQUOTK_BASE;
			if((r = P_Tbl->GetQuot(goodsID, qi, cost, price, &price, useCache)) > 0) {
				if(r == 2)
					ok = r;
				else if(qk_obj.GetCalculatedQuot(rQi.QuotKindID, 0, price, pResult, 0) > 0)
					ok = 1;
			}
		}
		else if(qk_obj.GetCalculatedQuot(rQi.QuotKindID, cost, price, pResult, 0) > 0)
			ok = 1;
	}
	return ok;
}

int SLAPI PPObjGoods::GetQuotExtByList(const PPQuotArray * pQList, const QuotIdent & rQi, double cost, double price, double * pResult)
{
	int    ok = -1, r;
	double result = 0.0;
	Goods2Tbl::Rec goods_rec;
	uint   pos = 0;
	if((r = pQList->GetResult(rQi, cost, price, &result)) > 0) {
		ok = r;
	}
	else {
		if(pQList->GoodsID && Fetch(pQList->GoodsID, &goods_rec) > 0 && goods_rec.ParentID) {
			QuotIdent tmpi = rQi;
			QuotIdent qi = rQi;
			qi.QuotKindID = PPQUOTK_BASE;
			double base = 0.0;
			if(P_Tbl->GetQuot(pQList->GoodsID, qi, cost, price, &base, 1) > 0)
				tmpi.SetBase(base);
			if((r = GetQuotExt(goods_rec.ParentID, tmpi, cost, price, &result, 1)) > 0)
				ok = r;
		}
		else if(rQi.QuotKindID) {
			PPObjQuotKind qk_obj;
			if(rQi.CurID) {
				QuotIdent qi = rQi;
				qi.QuotKindID = PPQUOTK_BASE;
				if((r = pQList->GetResult(qi, cost, price, &price)) > 0) {
					if(qk_obj.GetCalculatedQuot(rQi.QuotKindID, 0, price, &result, 0) > 0)
						ok = 1;
				}
			}
			else if(qk_obj.GetCalculatedQuot(rQi.QuotKindID, cost, price, &result, 0) > 0)
				ok = 1;
		}
	}
	ASSIGN_PTR(pResult, result);
	return ok;
}

int SLAPI PPObjGoods::CheckMatrix(PPID goodsID, PPID locID, PPID opID, PPID billArID)
{
	int    ok = 1;
	if(LConfig.Flags & CFGFLG_USEGOODSMATRIX && (!opID || CheckOpFlags(opID, OPKF_RESTRICTBYMTX))) {
		PPID   loc_id = locID;
		if(IsIntrExpndOp(opID) && billArID)
			loc_id = NZOR(PPObjLocation::ObjToWarehouse(billArID), locID);
		ok = BIN(BelongToMatrix(goodsID, loc_id));
	}
	return ok;
}

int SLAPI PPObjGoods::GetQuot(PPID goodsID, const QuotIdent & rQi, double cost, double price, double * pResult, int useCache)
	{ return P_Tbl->GetQuot(goodsID, rQi, cost, price, pResult, useCache); }
int SLAPI PPObjGoods::GetQuotExt(PPID goodsID, const QuotIdent & rQi, double * pResult, int useCache)
	{ return Helper_GetQuotExt(goodsID, rQi, 0.0, 0.0, pResult, (useCache == 1000 && pResult) ? 1 : useCache); }
int SLAPI PPObjGoods::BelongToMatrix(PPID goodsID, PPID locID)
	{ return P_Tbl->BelongToMatrix(goodsID, locID); }
int SLAPI PPObjGoods::GetQuotList(PPID goodsID, PPID locID, PPQuotArray & rList)
	{ return P_Tbl->GetQuotList(goodsID, locID, rList); }

int SLAPI PPObjGoods::PutQuotList(PPID goodsID, PPQuotArray * pList, int use_ta)
{
	int    ok = 1;
	if(pList)
		ok = (pList->GoodsID == goodsID) ? P_Tbl->SetQuotList(*pList, use_ta) : PPSetError(PPERR_INVQUOTLIST);
	else {
		PPQuotArray zero_list(goodsID);
		ok = P_Tbl->SetQuotList(zero_list, use_ta);
	}
	return ok;
}

int SLAPI PPObjGoods::UpdateFlags(PPID goodsID, long setF, long resetF, int use_ta)
{
	int    ok = -1;
	Goods2Tbl::Rec rec;
	if(setF || resetF) {
		PPTransaction tra(use_ta);
		THROW(tra);
		if(P_Tbl->Search(goodsID, &rec) > 0) {
			long   old_f = rec.Flags;
			if(setF & GF_NODISCOUNT) rec.Flags |= GF_NODISCOUNT;
			if(resetF & GF_NODISCOUNT) rec.Flags &= ~GF_NODISCOUNT;
			if(setF & GF_PASSIV) rec.Flags |= GF_PASSIV;
			if(resetF & GF_PASSIV) rec.Flags &= ~GF_PASSIV;
			if(setF & GF_PRICEWOTAXES) rec.Flags |= GF_PRICEWOTAXES;
			if(resetF & GF_PRICEWOTAXES) rec.Flags &= ~GF_PRICEWOTAXES;
			if(setF & GF_HASIMAGES) rec.Flags |= GF_HASIMAGES;
			if(resetF & GF_HASIMAGES) rec.Flags &= ~GF_HASIMAGES;
			if(old_f != rec.Flags) {
				THROW_DB(P_Tbl->updateRecBuf(&rec));
				DS.LogAction(PPACN_OBJUPD, Obj, goodsID, 0, 0);
				if((old_f & GF_NODISCOUNT) && (!(rec.Flags & GF_NODISCOUNT)))
					DS.LogAction(PPACN_GOODSNODISRMVD, Obj, goodsID, 0, 0);
				// @v9.5.0 {
				if(!(old_f & GF_PASSIV) && (rec.Flags & GF_PASSIV))
					DS.LogAction(PPACN_GOODSPASSVSET, Obj, goodsID, 0, 0);
				// } @v9.5.0
				ok = 1;
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}
//
//
//
int SLAPI PPObjGoods::SelectGoodsInPlaceOfRemoved(PPID rmvdGoodsID, PPID extGoodsID, PPID * pReplaceGoodsID)
{
	int    ok = -1;
	TDialog * dlg = 0;
	Goods2Tbl::Rec goods_rec;
	ASSIGN_PTR(pReplaceGoodsID, 0);
	if(Search(rmvdGoodsID) > 0) {
		ASSIGN_PTR(pReplaceGoodsID, rmvdGoodsID);
		ok = -1;
	}
	else {
		PPID   subst_id = 0;
		GoodsCtrlGroup::Rec goods_ctrl_grp;
		if(Search(extGoodsID, &goods_rec) > 0)
			subst_id = extGoodsID;
		else {
			SysJournal * p_sj = DS.GetTLA().P_SysJ;
			int    found = 0;
			subst_id = rmvdGoodsID;
			do {
				if(Search(subst_id, &goods_rec) > 0)
					found = 1;
			} while(!found && p_sj && p_sj->GetLastObjUnifyEvent(PPOBJ_GOODS, subst_id, &subst_id, 0) > 0);
			if(!found) {
				subst_id = 0;
				MEMSZERO(goods_rec);
			}
		}
		THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_RMVDGDS))));
		dlg->addGroup(1, new GoodsCtrlGroup(CTLSEL_RMVDGDS_GOODSGRP, CTLSEL_RMVDGDS_GOODS));
		goods_ctrl_grp.GrpID = goods_rec.ParentID;
		goods_ctrl_grp.GoodsID = subst_id;
		goods_ctrl_grp.Flags = GoodsCtrlGroup::enableInsertGoods;
		dlg->setGroupData(1, &goods_ctrl_grp);
		dlg->setCtrlData(CTL_RMVDGDS_ID, &rmvdGoodsID);
		dlg->disableCtrl(CTL_RMVDGDS_ID, 1);
		if(ExecView(dlg) == cmOK) {
			dlg->getGroupData(1, &goods_ctrl_grp);
			subst_id = goods_ctrl_grp.GoodsID;
			ASSIGN_PTR(pReplaceGoodsID, subst_id);
			if(subst_id)
				ok = 1;
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}
//
//
//
SLAPI RetailExtrItem::RetailExtrItem() : Cost(0.0), Price(0.0), BasePrice(0.0), ExtPrice(0.0), OuterPrice(0.0), 
	Flags(0), QuotKindUsedForPrice(0), QuotKindUsedForExtPrice(0), CurLotDate(ZERODATE), Expiry(ZERODATE)
{
}

RetailPriceExtractor::ExtQuotBlock::ExtQuotBlock(PPID quotKindID)
	{ QkList.addnz(quotKindID); }

RetailPriceExtractor::ExtQuotBlock::ExtQuotBlock(const PPSCardSerPacket & rScsPack)
{
	if(rScsPack.QuotKindList_.getCount())
		QkList = rScsPack.QuotKindList_;
	else
		QkList.addnz(rScsPack.Rec.QuotKindID_s);
}

SLAPI RetailPriceExtractor::RetailPriceExtractor() : EqBlk(0), LocID(0), ArID(0), Flags(0), P_GObj(new PPObjGoods)
{
}

SLAPI RetailPriceExtractor::RetailPriceExtractor(PPID locID, const ExtQuotBlock * pEqBlk, PPID arID, LDATETIME actualDtm, long flags) : 
	EqBlk(0), P_GObj(new PPObjGoods)
{
	Init(locID, pEqBlk, arID, actualDtm, flags);
}

SLAPI RetailPriceExtractor::~RetailPriceExtractor()
{
	delete P_GObj;
}

void SLAPI RetailPriceExtractor::SetLocation(PPID locID)
{
	LocID = locID;
}

int SLAPI RetailPriceExtractor::Init(PPID locID, const ExtQuotBlock * pEqBlk, PPID arID, LDATETIME actualDtm, long flags)
{
	PPObjQuotKind qk_obj;
	LocID = locID;
	ArID = arID;
	Flags = flags;
	if(!(flags & RTLPF_PRICEBYQUOT)) {
		PPEquipConfig eq_cfg;
		ReadEquipConfig(&eq_cfg);
		SETFLAG(Flags, RTLPF_PRICEBYQUOT, eq_cfg.Flags & PPEquipConfig::fUseQuotAsPrice);
	}
	{
		LDATETIME ctm = !actualDtm ? getcurdatetime_() : actualDtm;
		if(!(Flags & RTLPF_USEQUOTWTIME))
			ctm.t = ZEROTIME;
		{
			if(pEqBlk && pEqBlk->QkList.getCount()) {
				EqBlk.QkList = pEqBlk->QkList;
				qk_obj.ArrangeList(ctm, EqBlk.QkList, Flags);
			}
			else
				EqBlk.QkList.clear();
		}
		{
			RetailQuotList.freeAll();
			qk_obj.GetRetailQuotList(ctm, &RetailQuotList, Flags|RTLPF_USEQKCACHE);
		}
	}
	return 1;
}

int SLAPI RetailPriceExtractor::GetPrice(PPID goodsID, PPID forceBaseLotID, double qtty, RetailExtrItem * pItem)
{
	int    use_quot_cache = 1000; // @v8.2.7 1-->1000 // @v9.2.2 1000-->0 // @v9.3.1 0-->1000
	int    ok = -1, r = -1;
	uint   i, gp_flags = GPRET_INDEF;
	const  LDATE curdt = getcurdate_();
	//
	// Список видов котировок уже использованных для расчета цены.
	// Этот список препятствует двойному применению одной и той же котировки к цене.
	//
	PPIDArray used_quot_list;
	double price = 0.0, ext_price = 0.0, quot = 0.0;
	double base_price = 0.0;
	ReceiptTbl::Rec lot_rec;
	MEMSZERO(lot_rec);
	PROFILE_START
	THROW_INVARG(pItem);
	pItem->QuotKindUsedForPrice = 0;
	pItem->QuotKindUsedForExtPrice = 0;
	pItem->ManufDtm.Z();
	if(LocID == 0)
		gp_flags |= GPRET_OTHERLOC;
	assert(P_GObj);
	if(P_GObj->CheckFlag(goodsID, GF_UNLIM)) {
		QuotIdent qi(LocID, PPQUOTK_BASE, 0, ArID);
		qi.Qtty_ = qtty;
		THROW(r = P_GObj->GetQuotExt(goodsID, qi, &price, use_quot_cache));
		if(r > 0) {
			pItem->QuotKindUsedForPrice = PPQUOTK_BASE;
			used_quot_list.addUnique(PPQUOTK_BASE);
			SETFLAG(pItem->Flags, pItem->fDisabledQuot, r == 2);
		}
		MEMSZERO(lot_rec);
		ok = GPRET_PRESENT;
	}
	else {
		if(forceBaseLotID) {
			lot_rec.ID = forceBaseLotID;
			gp_flags |= GPRET_FORCELOTID;
		}
		THROW(ok = ::GetCurGoodsPrice(goodsID, LocID, gp_flags, &price, &lot_rec));
		// @v8.0.12 {
		if(Flags & RTLPF_USEOUTERPRICE && pItem->OuterPrice > 0.0) {
			price = pItem->OuterPrice;
		}
		// } @v8.0.12
		base_price = price;
		assert(!lot_rec.GoodsID || labs(lot_rec.GoodsID) == goodsID);
		if(lot_rec.ID) {
			ObjTagItem tag;
			LDATETIME dtm;
			if(PPRef->Ot.GetTag(PPOBJ_LOT, lot_rec.ID, PPTAG_LOT_MANUFTIME, &tag) > 0 && tag.GetTimestamp(&dtm) > 0)
				pItem->ManufDtm = dtm;
		}
		if(Flags & RTLPF_PRICEBYQUOT) {
			double q_price = 0.0;
			const QuotIdent qi(curdt, LocID, PPQUOTK_BASE, 0, ArID);
			if(P_GObj->GetQuotExt(goodsID, qi, lot_rec.Cost, price, &q_price, use_quot_cache) > 0) {
				pItem->QuotKindUsedForPrice = PPQUOTK_BASE;
				price = q_price;
				base_price = price;
				used_quot_list.addUnique(PPQUOTK_BASE);
			}
		}
	}
	PROFILE_END
	PROFILE_START
	{
		const uint eqc = EqBlk.QkList.getCount();
		if(eqc) {
			uint   eq_pos = 0;
			int    eq_disabled = 0;
			for(uint i = 0; i < eqc; i++) {
				const PPID ext_qk_id = EqBlk.QkList.get(i);
				const QuotIdent qi(curdt, LocID, ext_qk_id, 0, ArID);
				THROW(r = P_GObj->GetQuotExt(goodsID, qi, lot_rec.Cost, price, &quot, use_quot_cache));
				if(r > 0) {
					if(Flags & RTLPF_USEMINEXTQVAL) {
						if(!eq_pos || quot < ext_price) {
							ext_price = quot;
							eq_pos = i+1;
						}
					}
					else {
						ext_price = quot;
						eq_disabled = BIN(r == 2);
						eq_pos = i+1;
						break;
					}
				}
			}
			if(eq_pos) {
				pItem->QuotKindUsedForExtPrice = EqBlk.QkList.get(eq_pos-1);
				SETFLAG(pItem->Flags, pItem->fDisabledExtQuot, eq_disabled); // @v7.8.1
			}
		}
	}
	for(i = 0; i < pItem->QuotList.getCount(); i++) {
		RAssoc & r_quot_item = pItem->QuotList.at(i);
		const QuotIdent qi(curdt, LocID, r_quot_item.Key, 0, ArID);
		THROW(r = P_GObj->GetQuotExt(goodsID, qi, lot_rec.Cost, price, &quot, use_quot_cache));
		r_quot_item.Val = (r > 0) ? quot : 0.0;
	}
	r = (Flags & RTLPF_GETCURPRICE) ? 1 : -1;
	for(i = 0; r < 0 && i < RetailQuotList.getCount(); i++) {
		const PPID qk_id = RetailQuotList.get(i);
		if(!used_quot_list.lsearch(qk_id)) {
			QuotIdent qi(curdt, LocID, qk_id, 0, ArID);
			qi.Qtty_ = fabs(qtty);
			THROW(r = P_GObj->GetQuotExt(goodsID, qi, lot_rec.Cost, price, &quot, use_quot_cache));
			if(r > 0) {
				pItem->QuotKindUsedForPrice = qk_id;
				price = quot;
				//
				// Здесь base_price не меняется. Таким образом, вызывающая функция сможет отличить фиксированную
				// цену от той, которая может зависеть от всевозможных акций
				//
				used_quot_list.addUnique(qk_id);
				SETFLAG(pItem->Flags, pItem->fDisabledQuot, r == 2); // @v7.0.11
			}
		}
	}
	pItem->Cost       = R5(lot_rec.Cost);
	pItem->Price      = R5(price);
	pItem->BasePrice  = R5(base_price);
	pItem->ExtPrice   = R5(ext_price);
	pItem->CurLotDate = lot_rec.Dt;
	PROFILE_END
	PROFILE_START
	if(lot_rec.Expiry)
		pItem->Expiry = lot_rec.Expiry;
	else {
		GoodsStockExt gse;
		if(P_GObj->GetStockExt(goodsID, &gse, 1) > 0 && gse.ExpiryPeriod > 0)
			pItem->Expiry = plusdate(getcurdate_(), gse.ExpiryPeriod);
	}
	if(price < 0.0 || (price == 0.0 && !pItem->QuotKindUsedForPrice))
		ok = -1;
	PROFILE_END
	CATCHZOK
	return ok;
}
//
//
//
int PPObjGoods::SetupPreferredBarcodeTags()
{
	int    ok = 1;
	BarcodeArray bc_list;
	Goods2Tbl::Rec goods_rec;
	PPWait(1);
	{
		PPTransaction tra(1);
		THROW(tra);
		for(GoodsIterator giter; giter.Next(&goods_rec) > 0;) {
			P_Tbl->ReadBarcodes(goods_rec.ID, bc_list);
			if(bc_list.getCount()) {
				uint   pref_item_pos = 0, single_item_pos = 0;
				BarcodeTbl::Rec * p_pref_item = bc_list.GetPreferredItem(&pref_item_pos);
				if(!p_pref_item) {
					BarcodeTbl::Rec * p_single_item = bc_list.GetSingleItem(&single_item_pos);
					if(p_single_item) {
						if(bc_list.SetPreferredItem(single_item_pos) > 0) {
							THROW(P_Tbl->UpdateBarcodes(goods_rec.ID, &bc_list, 0));
						}
					}
				}
			}
			PPWaitPercent(giter.GetIterCounter());
		}
		THROW(tra.Commit());
	}
	PPWait(0);
	CATCHZOKPPERR
	return ok;
}
//
//
//
#include <..\osf\zint\backend\zint.h>

//static
SString & FASTCALL PPBarcode::ConvertUpceToUpca(const char * pUpce, SString & rUpca)
{
	rUpca.Z();
	char   temp[64];
	SUpceToUpca(pUpce, temp);
	rUpca = temp;
	return rUpca;
}

//static
int FASTCALL PPBarcode::GetStdName(int bcstd, SString & rBuf)
{
	rBuf.Z();
	int    ok = 1;
    switch(bcstd) {
		case BARCSTD_CODE11:      rBuf = "code11"; break;
		case BARCSTD_INTRLVD2OF5: rBuf = "interleaved2of5"; break;
		case BARCSTD_CODE39:      rBuf = "code39"; break;
		case BARCSTD_CODE49:      rBuf = "code49"; break;
		case BARCSTD_PDF417:      rBuf = "pdf417"; break;
		case BARCSTD_EAN8:        rBuf = "ean8";   break;
		case BARCSTD_UPCE:        rBuf = "upce";   break;
		case BARCSTD_CODE93:      rBuf = "code93"; break;
		case BARCSTD_CODE128:     rBuf = "code128"; break;
		case BARCSTD_EAN13:       rBuf = "ean13";   break;
		case BARCSTD_IND2OF5:     rBuf = "industial2of5"; break;
		case BARCSTD_STD2OF5:     rBuf = "standard2of5"; break;
		case BARCSTD_ANSI:        rBuf = "codabar"; break;
		case BARCSTD_LOGMARS:     rBuf = "logmars"; break;
		case BARCSTD_MSI:         rBuf = "msi"; break;
		case BARCSTD_PLESSEY:     rBuf = "plessey"; break;
		case BARCSTD_UPCEAN2EXT:  rBuf = "upcean2ext"; break;
		case BARCSTD_UPCEAN5EXT:  rBuf = "upcean5ext"; break;
		case BARCSTD_UPCA:        rBuf = "upca"; break;
		case BARCSTD_POSTNET:     rBuf = "postnet"; break;
		case BARCSTD_QR:          rBuf = "qr"; break;
		default:
			ok = 0;
    }
    return ok;
}

//static
int FASTCALL PPBarcode::RecognizeStdName(const char * pText)
{
	int    bcstd = 0;
    SString text = pText;
    if(text.NotEmptyS()) {
		// @v9.8.11 {
		const SymbHashTable * p_sht = PPGetStringHash(PPSTR_HASHTOKEN);
		if(p_sht) {
			text.ToLower();
			for(int do_again = 1; do_again;) {
				do_again = 0;
				uint _ut = 0;
				p_sht->Search(text, &_ut, 0);
				switch(_ut) {
					case PPHS_BCS_CODE11:          bcstd = BARCSTD_CODE11; break;
					case PPHS_BCS_CODE39:          bcstd = BARCSTD_CODE39; break;
					case PPHS_BCS_CODE49:          bcstd = BARCSTD_CODE49; break;
					case PPHS_BCS_CODE93:          bcstd = BARCSTD_CODE93; break;
					case PPHS_BCS_CODE128:         bcstd = BARCSTD_CODE128; break;
					case PPHS_BCS_PDF417:          bcstd = BARCSTD_PDF417; break;
					case PPHS_PDF:                 bcstd = BARCSTD_PDF417; break;
					case PPHS_BCS_EAN13:           bcstd = BARCSTD_EAN13; break;
					case PPHS_BCS_EAN8:            bcstd = BARCSTD_EAN8; break;
					case PPHS_BCS_UPCA:            bcstd = BARCSTD_UPCA; break;
					case PPHS_BCS_UPCE:            bcstd = BARCSTD_UPCE; break;
					case PPHS_BCS_QR:              bcstd = BARCSTD_QR; break;
					case PPHS_BCS_QRCODE:          bcstd = BARCSTD_QR; break;
					case PPHS_BCS_INTERLEAVED2OF5: bcstd = BARCSTD_INTRLVD2OF5; break;
					case PPHS_BCS_INDUSTIAL2OF5:   bcstd = BARCSTD_IND2OF5; break;
					case PPHS_BCS_STANDARD2OF5:    bcstd = BARCSTD_STD2OF5; break;
					case PPHS_BCS_CODABAR:         bcstd = BARCSTD_ANSI; break;
					case PPHS_BCS_MSI:             bcstd = BARCSTD_MSI; break;
					case PPHS_BCS_PLESSEY:         bcstd = BARCSTD_PLESSEY; break;
					case PPHS_BCS_POSTNET:         bcstd = BARCSTD_POSTNET; break;
					case PPHS_BCS_LOGMARS:         bcstd = BARCSTD_LOGMARS; break;
					default:
						{
							//
							// Возможно, что символ стандарта записан с разделением двух частей пробелом 
							// или дефисом. Поэтому, найдя первый такой знак (но только один) попытаемся
							// его убрать и повторить попытку поиска.
							// Например: ucp-a == upca или pdf 417 == pdf417
							// 
							uint cpos = 0;
							if(text.StrChr('-', &cpos) || text.StrChr(' ', &cpos)) {
								text.Excise(cpos, 1);
								do_again = 1;
							}
						}
						break;
				}
			}
		}
		// } @v9.8.11 
#if 0 // @v9.8.11 {
		text.ToLower();
		if(oneof3(text, "code11", "code-11", "code 11"))
			bcstd = BARCSTD_CODE11;
		if(oneof3(text, "code39", "code-39", "code 39"))
			bcstd = BARCSTD_CODE39;
		if(oneof3(text, "code49", "code-49", "code 49"))
			bcstd = BARCSTD_CODE49;
		if(oneof3(text, "code93", "code-93", "code 93"))
			bcstd = BARCSTD_CODE93;
		if(oneof3(text, "code128", "code-128", "code 128"))
			bcstd = BARCSTD_CODE128;
		if(oneof4(text, "pdf417", "pdf-417", "pdf 417", "pdf"))
			bcstd = BARCSTD_PDF417;
		else if(oneof3(text, "ean13", "ean-13", "ean 13"))
			bcstd = BARCSTD_EAN13;
		else if(oneof3(text, "ean8", "ean-8", "ean 8"))
			bcstd = BARCSTD_EAN8;
		else if(oneof3(text, "upca", "upc-a", "upc a"))
			bcstd = BARCSTD_UPCA;
		else if(oneof3(text, "upce", "upc-e", "upc e"))
			bcstd = BARCSTD_UPCE;
		else if(oneof4(text, "qr", "qr-code", "qr code", "qrcode"))
			bcstd = BARCSTD_QR;
		else if(text == "interleaved2of5")
			bcstd = BARCSTD_INTRLVD2OF5;
		else if(text == "industial2of5")
			bcstd = BARCSTD_IND2OF5;
		else if(text == "standard2of5")
			bcstd = BARCSTD_STD2OF5;
		else if(text == "codabar")
			bcstd = BARCSTD_ANSI;
		else if(text == "msi")
			bcstd = BARCSTD_MSI;
		else if(text == "plessey")
			bcstd = BARCSTD_PLESSEY;
		else if(text == "postnet")
			bcstd = BARCSTD_POSTNET;
		else if(text == "logmars")
			bcstd = BARCSTD_LOGMARS;
#endif // } @v9.8.11
    }
    if(!bcstd) {
		PPSetError(PPERR_INVBARCODESTDSYMB, pText);
    }
    return bcstd;
}

SLAPI PPBarcode::BarcodeImageParam::BarcodeImageParam() : Std(0), Flags(0), OutputFormat(0), Angle(0), ColorFg(ZEROCOLOR), ColorBg(ZEROCOLOR)
{
	Size.Set(0, 0);
}

//static
int FASTCALL PPBarcode::CreateImage(PPBarcode::BarcodeImageParam & rParam)
{
    int    ok = -1;
    int    zint_barc_std = 0;
	SString temp_buf;
    ZintSymbol * p_zs = ZBarcode_Create();
    THROW(p_zs);
	switch(rParam.Std) {
		case BARCSTD_EAN8:
		case BARCSTD_EAN13:
			zint_barc_std = (rParam.Flags & rParam.fWithCheckDigit) ? BARCODE_EANX_CHK : BARCODE_EANX;
			break;
		case BARCSTD_UPCA: 
			zint_barc_std = (rParam.Flags & rParam.fWithCheckDigit) ? BARCODE_UPCA_CHK : BARCODE_UPCA; 
			break;
		case BARCSTD_UPCE: 
			zint_barc_std = (rParam.Flags & rParam.fWithCheckDigit) ? BARCODE_UPCE_CHK : BARCODE_UPCE; 
			break;
		case BARCSTD_CODE128: zint_barc_std = BARCODE_CODE128; break;
		case BARCSTD_CODE39: zint_barc_std = BARCODE_CODE39; break;
		case BARCSTD_PDF417: zint_barc_std = BARCODE_PDF417; break;
		case BARCSTD_QR: zint_barc_std = BARCODE_QRCODE; break; // @v9.6.11
	}
	THROW(zint_barc_std);
    p_zs->Std = zint_barc_std;
	if(rParam.Size.x > 0)
		p_zs->width = rParam.Size.x;
	if(rParam.Size.y > 0)
		p_zs->height = rParam.Size.y;
	if(!rParam.ColorBg.IsEmpty()) {
		//rParam.ColorBg.ToStr(temp_buf, SColor::fmtRgbHexWithoutPrefix);
		//STRNSCPY(p_zs->bgcolour, temp_buf);
		p_zs->ColorBg = rParam.ColorBg;
	}
	if(!rParam.ColorFg.IsEmpty()) {
		//rParam.ColorFg.ToStr(temp_buf, SColor::fmtRgbHexWithoutPrefix);
		//STRNSCPY(p_zs->fgcolour, temp_buf);
		p_zs->ColorFg = rParam.ColorFg;
	}
    {
		const int rot_angle = oneof4(rParam.Angle, 0, 90, 180, 270) ? rParam.Angle : 0;
		THROW(ZBarcode_Encode(p_zs, rParam.Code.ucptr(), rParam.Code.Len()) == 0);
    	if(rParam.OutputFormat == 0) {
			THROW(ZBarcode_Buffer(p_zs, rot_angle) == 0);
			THROW_SL(rParam.Buffer.Init(p_zs->bitmap_width, p_zs->bitmap_height));
			THROW_SL(rParam.Buffer.AddLines(p_zs->bitmap, SImageBuffer::PixF::s24RGB, p_zs->bitmap_height, 0));
    	}
    	else {
			SString file_name;
			if(isempty(rParam.OutputFileName)) {
				PPGetFilePath(PPPATH_OUT, (temp_buf = rParam.Code).Dot().Cat("png"), file_name);
			}
			else
				file_name = rParam.OutputFileName;
			if(rParam.OutputFormat == SFileFormat::Png)
				SPathStruc::ReplaceExt(file_name, "png", 1);
			else if(rParam.OutputFormat == SFileFormat::Svg)
				SPathStruc::ReplaceExt(file_name, "svg", 1);
			else if(rParam.OutputFormat == SFileFormat::Txt)
				SPathStruc::ReplaceExt(file_name, "txt", 1);
			else if(rParam.OutputFormat == SFileFormat::Gif)
				SPathStruc::ReplaceExt(file_name, "gif", 1);
			else if(rParam.OutputFormat == SFileFormat::Bmp)
				SPathStruc::ReplaceExt(file_name, "bmp", 1);
			else
				SPathStruc::ReplaceExt(file_name, "png", 1);
			STRNSCPY(p_zs->outfile, file_name);
			THROW(ZBarcode_Print(p_zs, rot_angle) == 0);
			rParam.OutputFileName = file_name;
    	}
    }
    CATCHZOK
    ZBarcode_Delete(p_zs);
    return ok;
}

#include <../osf/zbar/include/zbar.h>

struct ZBarToPpBarcStd {
	ushort ZBarCode;
	ushort PpCode;
};

static ZBarToPpBarcStd _ZBarToPpBarcStdTab[] = {
	{ 0,            BARCSTD_CODE11 },
	{ ZBAR_I25,     BARCSTD_INTRLVD2OF5 },
	{ ZBAR_CODE39,  BARCSTD_CODE39 },
	{ 0,            BARCSTD_CODE49 },
	{ ZBAR_PDF417,  BARCSTD_PDF417 },
	{ ZBAR_EAN8,    BARCSTD_EAN8 },
	{ ZBAR_UPCE,    BARCSTD_UPCE },
	{ ZBAR_CODE93,  BARCSTD_CODE93 },
	{ ZBAR_CODE128, BARCSTD_CODE128 },
	{ ZBAR_EAN13,   BARCSTD_EAN13 },
	{ 0,            BARCSTD_IND2OF5 },
	{ 0,            BARCSTD_STD2OF5 },
	{ ZBAR_CODABAR, BARCSTD_ANSI },
	{ 0,            BARCSTD_LOGMARS },
	{ 0,            BARCSTD_MSI },
	{ 0,            BARCSTD_PLESSEY },
	{ ZBAR_EAN2,    BARCSTD_UPCEAN2EXT },
	{ ZBAR_EAN5,    BARCSTD_UPCEAN5EXT },
	{ ZBAR_UPCA,    BARCSTD_UPCA },
	{ 0,            BARCSTD_POSTNET },
	{ ZBAR_QRCODE,  BARCSTD_QR },
};

static int FASTCALL ZBarStdToPp(zbar_symbol_type_t zbarstd)
{
	for(size_t i = 0; i < SIZEOFARRAY(_ZBarToPpBarcStdTab); i++) {
		if(_ZBarToPpBarcStdTab[i].ZBarCode == (ushort)zbarstd) {
			return _ZBarToPpBarcStdTab[i].PpCode;
		}
	}
	return 0;
}
//
//
//static
int FASTCALL PPBarcode::RecognizeImage(const char * pInpFileName, TSCollection <PPBarcode::Entry> & rList)
{
	rList.clear();

	int    ok = -1;
	SString debug_file_name;
	zbar_image_scanner_t * p_scanner = 0;
	zbar_image_t * p_zbar_image = 0;
    SImageBuffer ib__;
	const SImageBuffer * p_ib = 0;
	SDrawFigure * p_fig = 0;
	{
		THROW_SL(p_fig = SDrawFigure::CreateFromFile(pInpFileName, 0));
		if(p_fig->GetKind() == SDrawFigure::kImage) {
			p_ib = &((SDrawImage *)p_fig)->GetBuffer();
		}
		else if(p_fig->GetKind()) { // Вероятно, векторная фигура
			FPoint sz = p_fig->GetSize();
			if(sz.X <= 0.0f || sz.Y <= 0.0f) {
				sz.Set(300.0f);
			}
			else {
				sz.X *= 2.0f;
				sz.Y *= 2.0f;
			}
			THROW_SL(ib__.Init((uint)sz.X, (uint)sz.Y));
			THROW_SL(p_fig->TransformToImage(0, ib__));
			p_ib = &ib__;
		}
	}
	if(p_ib) {
    	const uint src_width = p_ib->GetWidth();
    	const uint src_height = p_ib->GetHeight();
		uint8 * p_zbar_img_buf = (uint8 *)SAlloc::C(src_height * src_width, sizeof(uint8));
    	THROW_MEM(p_zbar_img_buf);
		{
			//
			// @debug
			//
			SPathStruc ps(pInpFileName);
			ps.Nam.Cat("-debug1");
			ps.Ext = "png";
			ps.Merge(debug_file_name);
			SFile f_d(debug_file_name, SFile::mWrite|SFile::mBinary);
			{
				SImageBuffer::StoreParam sp(SFileFormat::Png);
				SImageBuffer temp_ib;
				temp_ib.Copy(*p_ib);
				temp_ib.Store(sp, f_d);
			}
		}
    	{
			const uint32 * p_src_buf = (const uint32 *)p_ib->GetData();
			const SImageBuffer::PixF pf(SImageBuffer::PixF::s8GrayScale);
			for(uint i = 0; i < src_height; i++) {
				const size_t offs = src_width * i;
				THROW_SL(pf.SetUniform(p_src_buf + offs, p_zbar_img_buf + offs, src_width, 0));
			}
    	}
    	{
			p_scanner = zbar_image_scanner_create();
			zbar_image_scanner_set_config(p_scanner, ZBAR_NONE, ZBAR_CFG_ENABLE, 1);
			p_zbar_image = zbar_image_create();
			zbar_image_set_format(p_zbar_image, zbar_fourcc('Y','8','0','0'));
			zbar_image_set_size(p_zbar_image, src_width, src_height);
			zbar_image_set_data(p_zbar_image, p_zbar_img_buf, src_width * src_height, zbar_image_free_data);
			// scan the image for barcodes
			int n = zbar_scan_image(p_scanner, p_zbar_image);
			// extract results
			for(const zbar_symbol_t * p_symbol = zbar_image_first_symbol(p_zbar_image); p_symbol; p_symbol = zbar_symbol_next(p_symbol)) {
				PPBarcode::Entry * p_new_entry = rList.CreateNewItem();
				THROW_SL(p_new_entry);
				p_new_entry->BcStd = ZBarStdToPp(zbar_symbol_get_type(p_symbol));
				p_new_entry->Code = zbar_symbol_get_data(p_symbol);
				ok = 1;
			}
			// clean up
			zbar_image_destroy(p_zbar_image);
			zbar_image_scanner_destroy(p_scanner);
    	}
    }
    CATCHZOK
	delete p_fig;
    return ok;
}

#if SLTEST_RUNNING // {

struct TestBcStdSymb {
	const char * P_Symb;
	int   Std;
};

static const TestBcStdSymb TestBcStdSymbList[] = {
	{ "code11", BARCSTD_CODE11 },
	{ "code-11" , BARCSTD_CODE11 },
	{ "code 11", BARCSTD_CODE11 },
	{ "code39",  BARCSTD_CODE39 },
	{ "code-39", BARCSTD_CODE39 },
	{ "code 39", BARCSTD_CODE39 },
	{ "code49",  BARCSTD_CODE49 },
	{ "code-49", BARCSTD_CODE49 },
	{ "code 49", BARCSTD_CODE49 },
	{ "code93",  BARCSTD_CODE93 },
	{ "code-93", BARCSTD_CODE93 },
	{ "code 93", BARCSTD_CODE93 },
	{ "code128",  BARCSTD_CODE128 },
	{ "code-128", BARCSTD_CODE128 },
	{ "code 128", BARCSTD_CODE128 },
	{ "pdf417",   BARCSTD_PDF417 },
	{ "pdf-417",  BARCSTD_PDF417 },
	{ "pdf 417",  BARCSTD_PDF417 },
	{ "pdf",      BARCSTD_PDF417 },
	{ "ean13", BARCSTD_EAN13 },
	{ "ean-13", BARCSTD_EAN13 },
	{ "ean 13", BARCSTD_EAN13 },
	{ "ean8", BARCSTD_EAN8 },
	{ "ean-8", BARCSTD_EAN8 },
	{ "ean 8", BARCSTD_EAN8 },
	{ "upca", BARCSTD_UPCA },
	{ "upc-a", BARCSTD_UPCA },
	{ "upc a", BARCSTD_UPCA },
	{ "upce", BARCSTD_UPCE },
	{ "upc-e", BARCSTD_UPCE },
	{ "upc e", BARCSTD_UPCE },
	{ "qr", BARCSTD_QR },
	{ "qr-code", BARCSTD_QR },
	{ "qr code", BARCSTD_QR },
	{ "qrcode", BARCSTD_QR },
	{ "interleaved2of5", BARCSTD_INTRLVD2OF5 },
	{ "industial2of5", BARCSTD_IND2OF5 },
	{ "standard2of5", BARCSTD_STD2OF5 },
	{ "codabar", BARCSTD_ANSI },
	{ "msi", BARCSTD_MSI },
	{ "plessey", BARCSTD_PLESSEY },
	{ "postnet", BARCSTD_POSTNET },
	{ "logmars", BARCSTD_LOGMARS }
};

SLTEST_R(BarcodeOutputAndRecognition)
{
	SString temp_buf;
	{
		for(uint i = 0; i < SIZEOFARRAY(TestBcStdSymbList); i++) {
			const TestBcStdSymb & r_entry = TestBcStdSymbList[i];
			(temp_buf = r_entry.P_Symb).ToUpper();
			int std = PPBarcode::RecognizeStdName(temp_buf);
			SLTEST_CHECK_EQ((long)std, (long)r_entry.Std);
			SLTEST_CHECK_NZ(PPBarcode::GetStdName(std, temp_buf));
			int _found = 0;
			for(uint j = 0; !_found && j < SIZEOFARRAY(TestBcStdSymbList); j++) {
				const TestBcStdSymb & r_entry2 = TestBcStdSymbList[j];
				if(r_entry2.Std == std && temp_buf.CmpNC(r_entry2.P_Symb) == 0)
					_found = 1;
			}
			SLTEST_CHECK_NZ(_found);
		}
	}
	SString line_buf;
	SString code_buf;
	SString input_file_path;
	//SString img_path;
	(input_file_path = GetSuiteEntry()->InPath).SetLastSlash().Cat("barcode.txt");
    SFile f_in(input_file_path, SFile::mRead);
    while(f_in.ReadLine(line_buf)) {
		line_buf.Chomp().Strip();
		if(line_buf.Divide(':', temp_buf, code_buf) > 0) {
			PPBarcode::BarcodeImageParam bip;
			bip.Std = PPBarcode::RecognizeStdName(temp_buf.Strip());
			if(bip.Std) {
				code_buf.Strip();
				{
					temp_buf = code_buf;
					if(oneof2(bip.Std, BARCSTD_EAN13, BARCSTD_EAN8))
						temp_buf.TrimRight();
					if(bip.Std == BARCSTD_UPCE) {
						while(temp_buf.C(0) == '0')
							temp_buf.ShiftLeft();
						temp_buf.TrimRight();
					}
					bip.Code = temp_buf;
				}
				{
					//
					// PNG
					//
					bip.OutputFormat = SFileFormat::Png;
					(bip.OutputFileName = GetSuiteEntry()->OutPath).SetLastSlash().Cat(code_buf).Dot().Cat("png");
					if(SLTEST_CHECK_NZ(PPBarcode::CreateImage(bip))) {
						TSCollection <PPBarcode::Entry> bc_list;
						if(SLTEST_CHECK_LT(0L, PPBarcode::RecognizeImage(bip.OutputFileName, bc_list))) {
							SLTEST_CHECK_EQ(bc_list.getCount(), 1);
							if(bc_list.getCount() > 0) {
								const PPBarcode::Entry * p_entry = bc_list.at(0);
								SLTEST_CHECK_EQ((long)p_entry->BcStd, (long)bip.Std);
								SLTEST_CHECK_EQ(p_entry->Code, code_buf);
							}
						}
					}
				}
				{
					//
					// SVG
					//
					bip.OutputFormat = SFileFormat::Svg;
					(bip.OutputFileName = GetSuiteEntry()->OutPath).SetLastSlash().Cat(code_buf).Dot().Cat("svg");
					if(SLTEST_CHECK_NZ(PPBarcode::CreateImage(bip))) {
						TSCollection <PPBarcode::Entry> bc_list;
						if(SLTEST_CHECK_LT(0L, PPBarcode::RecognizeImage(bip.OutputFileName, bc_list))) {
							SLTEST_CHECK_EQ(bc_list.getCount(), 1);
							if(bc_list.getCount() > 0) {
								const PPBarcode::Entry * p_entry = bc_list.at(0);
								SLTEST_CHECK_EQ((long)p_entry->BcStd, (long)bip.Std);
								SLTEST_CHECK_EQ(p_entry->Code, code_buf);
							}
						}
					}
				}
			}
		}
    }
	return CurrentStatus;
}

#endif // SLTEST_RUNNING
