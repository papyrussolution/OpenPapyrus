// OBJGOODS.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016
//
#include <pp.h>
#pragma hdrstop
//
// @ModuleDef(PPObjGoods)
//
TLP_IMPL(PPObjGoods, GoodsCore, P_Tbl);
TLP_IMPL(PPObjGoods, HistGoodsCore, HistGoods);
//
// HistGoodsCore
//
int SLAPI HistGoodsCore::Insert(PPID * pHistID, SBuffer * pBuf, int use_ta)
{
	int    ok = -1;
	if(pBuf) {
		RECORDSIZE fix_rec_size = 0;
		RECORDSIZE data_len = (RECORDSIZE)pBuf->GetAvailableSize();
		const void * p_data_buf = pBuf->GetBuf();
		{
			PPTransaction tra(use_ta);
			THROW(tra);
			getRecSize(&fix_rec_size);
			copyBufFrom(p_data_buf, fix_rec_size);
			THROW(writeLobData(this->Data, PTR8(p_data_buf)+fix_rec_size, (data_len > fix_rec_size) ? (data_len-fix_rec_size) : 0));
			THROW_DB(insertRec(0, pHistID));
			THROW(tra.Commit());
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SLAPI HistGoodsCore::Search(PPID goodsID, long ver, HistGoodsTbl::Rec * pRec)
{
	int    ok = -1;
	int    sp = 0;
	HistGoodsTbl::Key1 k;
	k.GoodsID = goodsID;
	if(ver == 0) {
		k.Ver = MAXLONG;
		sp = spLt;
	}
	else {
		k.Ver = ver;
		sp = spEq;
	}
	if(search(1, &k, sp) > 0 && k.GoodsID == goodsID) {
		copyBufTo(pRec);
		ok = 1;
	}
	return ok;
}

int SLAPI HistGoodsCore::Search(PPID histID, SBuffer * pBuf)
{
	int    ok = -1;
	HistGoodsTbl::Key0 k;
	k.ID = histID;
	if(search(0, &k, spEq) > 0) {
		if(pBuf) {
			RECORDSIZE fix_rec_size = 0;
			getRecSize(&fix_rec_size);
			pBuf->Write(&data, fix_rec_size);
			readLobData(this->Data, *pBuf);
		}
		ok = 1;
	}
	return ok;
}

long SLAPI HistGoodsCore::GetNextVer(PPID goodsID)
{
	HistGoodsTbl::Rec rec;
	return (Search(goodsID, 0, &rec) > 0) ? (rec.Ver + 1) : 1;
}
//
//
//static
int SLAPI PPObjGoods::GetBarcodeDiagText(int diag, SString & rBuf)
{
	/*
	const char * p_line = "OK;Пустой код;Код содержит нецифровые символы;Неверная контрольная цифра EAN13;Неверная контрольная цифра UPCA;\
	Неверная контрольная цифра EAN8;Неверная контрольная цифра UPCE;\
	Лишняя цифра в конце EAN13;Лишняя цифра в конце EAN8;EAN14;Не известный формат;Код UPCA в формате EAN13;\
	Возможно, лишняя цифра в конце UPCA;Возможно, UPCA без контрольной цифры;Возможно, EAN13 без контрольной цифры;\
	Возможно, EAN8 без контрольной цифры;UPCA без лидирующего нуля;Возможно, UPCA без лидирующего нуля и без контрольной цифры;\
	Возможно, UPCE без лидирующего нуля;Возможно, UPCE без контрольной цифры;Возможно, UPCE без лидирующего нуля и без контрольной цифры;\
	Код EAN8 в формате EAN13;Код EAN8 с лишним ведущим нулем";
	*/
	SString line_buf;
	PPLoadText(PPTXT_BARCODEDIAGMSG, line_buf);
	return PPGetSubStr(line_buf, diag, rBuf);
}

//static
int SLAPI PPObjGoods::DiagBarcode(const char * pBarcode, int * pDiag, int * pStd, SString * pNormalizedCode)
{
	int    ok = 0;
	uint   bclen = pBarcode ? strlen(pBarcode) : 0;
	uint   j;
	int    cd = 0;
	int    cdstd = 0;
	int    diag = 0;
	SString temp_buf, norm_code;
	norm_code = pBarcode;
	if(bclen == 0) {
		diag = cddEmpty; // Empty barcode
	}
	else {
		// @v7.7.2 {
		const PPID glob_acc_id = DS.GetConstTLA().GlobAccID;
		if(glob_acc_id) {
			PPObjGoods goods_obj;
			const PPGoodsConfig & r_cfg = goods_obj.GetConfig();
			if(r_cfg.BcPrefixGuaTagID) {
				PPObjTag tag_obj;
				ObjTagItem tag_item;
				if(tag_obj.FetchTag(glob_acc_id, r_cfg.BcPrefixGuaTagID, &tag_item) > 0 && tag_item.GetStr(temp_buf) > 0) {
					StringSet ss(';', temp_buf);
					for(uint p = 0; !ok && ss.get(&p, temp_buf);) {
						if(temp_buf.CmpL(pBarcode, 1) == 0) {
							diag = cddGuaPrefix;
							ok = -1;
						}
					}
				}
			}
		}
		// } @v7.7.2
		if(!ok) {
			int    first = pBarcode[0];
			int    second = pBarcode[1];
			int    digit_only = 1;
			uint   zcount = 0; // Количество ведущих нулей
			{
				int    prev_zero = 1;
				for(j = 0; j < bclen; j++) {
					if(prev_zero && pBarcode[j] == '0') {
						zcount++;
					}
					else {
						prev_zero = 0;
						if(!isdec((uchar)pBarcode[j])) {
							digit_only = 0;
							break;
						}
					}
				}
			}
			if(!digit_only)
				diag = cddNonDigit;
			else {
				if(bclen == 13) { // ean13
					if(zcount == 5) {
						(temp_buf = pBarcode).ShiftLeft(5);
						cd = CalcBarcodeCheckDigit(temp_buf.Trim(7));
						if(cd == (pBarcode[bclen-1]-'0')) {
							diag = cddEan8AsEan13;
							cdstd = BARCSTD_EAN8;
							norm_code = pBarcode + 5;
							ok = -1;
						}
					}
					else {
						cd = CalcBarcodeCheckDigit((temp_buf = pBarcode).Trim(12));
						if(cd == (pBarcode[bclen-1]-'0')) {
							if(zcount == 2) {
								diag = cddUpcaAsEan13;
								cdstd = BARCSTD_UPCA;
								norm_code = pBarcode+1;
								ok = -1;
							}
							else if(pBarcode[0] == '2') {
								diag = cddFreePrefixEan13;
								cdstd = BARCSTD_EAN13;
								ok = -1;
							}
							else {
								cdstd = BARCSTD_EAN13;
								ok = 1;
							}
						}
						else {
							cd = CalcBarcodeCheckDigit((temp_buf = pBarcode).Trim(11));
							if(cd == (pBarcode[bclen-2]-'0')) {
								diag = cdd_ExtraDigitUpca;
								cdstd = BARCSTD_UPCA;
								(norm_code = pBarcode).Trim(12);
								ok = -1;
							}
							else
								diag = (first == '0') ? cddInvCheckDigUpca : cddInvCheckDigEan13;
						}
					}
				}
				else if(bclen == 14) {
					// Возможно, ean13/upca с лишней контрольной цифрой
					cd = CalcBarcodeCheckDigit((temp_buf = pBarcode).Trim(12));
					if(cd == pBarcode[bclen-2]-'0') {
						if(first == '0' && second == '0') {
							diag = cdd_ExtraDigitUpca;
							cdstd = BARCSTD_UPCA;
							(norm_code = pBarcode+1).Trim(12);
							ok = -1;
						}
						else {
							diag = cddExtraDigitEan13;
							cdstd = BARCSTD_EAN13;
							(norm_code = pBarcode).Trim(13);
							ok = -1;
						}
					} //Может быть EAN14?
					else if(first >= '1' && first <= '8') {
						cd = CalcBarcodeCheckDigit((temp_buf = pBarcode+1).Trim(12));
						if(cd == pBarcode[bclen-1]-'0') {
							diag = cddEan14;
							norm_code = pBarcode+1;
							ok = -1;
						}
						else
							diag = cddUnknownFormat;
					}
					else
						diag = cddUnknownFormat;
				}
				else if(bclen == 12) {
					cd = CalcBarcodeCheckDigit((temp_buf = pBarcode).Trim(11));
					if(cd == (pBarcode[bclen-1]-'0')) {
						cdstd = BARCSTD_UPCA;
						ok = 1;
					}
					else if(first == '0') {
						// UPCA с неверной контрольной цифрой
						diag = cddInvCheckDigUpca;
					}
					else {
						// Возможно, EAN13 без контрольной цифры
						cd = CalcBarcodeCheckDigit(pBarcode);
						diag = cdd_Ean13WoCheckDig;
						cdstd = BARCSTD_EAN13;
						(norm_code = pBarcode).CatChar(cd + '0');
						ok = -1;
					}
				}
				else if(bclen == 11) {
					if(first == '0') {
						// Возможно, UPCA без контрольной цифры
						cd = CalcBarcodeCheckDigit(pBarcode);
						diag = cdd_UpcaWoCheckDig;
						cdstd = BARCSTD_UPCA;
						(norm_code = pBarcode).CatChar(cd + '0');
						ok = -1;
					}
					else {
						// Возможно, UPCA без лидирующего нуля //
						cd = CalcBarcodeCheckDigit((temp_buf = 0).CatChar('0').Cat(pBarcode).Trim(11));
						if(cd == (pBarcode[bclen-1]-'0')) {
							diag = cdd_UpcaWoLead0;
							cdstd = BARCSTD_UPCA;
							(norm_code = 0).CatChar('0').Cat(pBarcode);
							ok = -1;
						}
						else
							diag = cddUnknownFormat;
					}
				}
				else if(bclen == 10) {
					// Возможно, UPCA без лидирующего нуля и без контрольной цифры
					cd = CalcBarcodeCheckDigit((temp_buf = 0).CatChar('0').Cat(pBarcode));
					diag = cdd_UpcaWoCheckDigAndLead0;
					cdstd = BARCSTD_UPCA;
					(norm_code = 0).CatChar('0').Cat(pBarcode).CatChar(cd + '0');
					ok = -1;
				}
				else if(bclen == 9) {
					// Возможно, EAN8 с лишней цифрой
					if(zcount > 0) {
						(temp_buf = pBarcode).ShiftLeft(1);
						cd = CalcBarcodeCheckDigit(temp_buf.Trim(7));
						if(cd == (pBarcode[bclen-1]-'0')) {
							if(zcount > 1) {
								diag = cddExtraZeroUpce;
								cdstd = BARCSTD_UPCE;
								(norm_code = 0).Cat(pBarcode).ShiftLeft(1);
								ok = -1;
							}
							else {
								diag = cddExtraZeroEan8;
								cdstd = BARCSTD_EAN8;
								(norm_code = 0).Cat(pBarcode).ShiftLeft(1);
								ok = -1;
							}
						}
						else
							diag = cddUnknownFormat;
					}
					else {
						cd = CalcBarcodeCheckDigit((temp_buf = pBarcode).Trim(8));
						if(cd == (pBarcode[bclen-2]-'0')) {
							diag = cddExtraDigitEan8;
							cdstd = BARCSTD_EAN8;
							(norm_code = 0).Cat(pBarcode).Trim(8);
							ok = -1;
						}
						else
							diag = cddUnknownFormat;
					}
				}
				else if(bclen == 8) {
					if(first == '0') {
						cd = CalcBarcodeCheckDigit((temp_buf = pBarcode).Trim(7));
						if(cd == (pBarcode[bclen-1]-'0')) {
							cdstd = BARCSTD_UPCE;
							ok = 1;
						}
						else
							diag = cddInvCheckDigUpce;
					}
					else {
						cd = CalcBarcodeCheckDigit((temp_buf = pBarcode).Trim(7));
						if(cd == (pBarcode[bclen-1]-'0')) {
							cdstd = BARCSTD_EAN8;
							ok = 1;
						}
						else
							diag = cddInvCheckDigEan8;
					}
				}
				else if(bclen == 7) {
					if(first != '0') {
						// Возможно, UPCE без лидирующего нуля //
						cd = CalcBarcodeCheckDigit((temp_buf = 0).CatChar('0').Cat(pBarcode).Trim(7));
						if(cd == (pBarcode[bclen-1]-'0')) {
							diag = cdd_UpceWoLead0;
							cdstd = BARCSTD_UPCE;
							(norm_code = 0).CatChar('0').Cat(pBarcode);
							ok = -1;
						}
						else {
							// Возможно, EAN8 без контрольной цифры //
							cd = CalcBarcodeCheckDigit(pBarcode);
							diag = cdd_Ean8WoCheckDig;
							cdstd = BARCSTD_EAN8;
							(norm_code = pBarcode).CatChar(cd + '0');
							ok = -1;
						}
					}
					else {
						// Возможно, UPCE без контрольной цифры //
						cd = CalcBarcodeCheckDigit((temp_buf = 0).Cat(pBarcode));
						diag = cdd_UpceWoCheckDig;
						cdstd = BARCSTD_UPCE;
						(norm_code = 0).Cat(pBarcode).CatChar(cd + '0');
						ok = -1;
					}
				}
				else if(bclen == 6) {
					// Возможно, UPCE без лидирующего нуля и без контрольной цифры
					cd = CalcBarcodeCheckDigit((temp_buf = 0).CatChar('0').Cat(pBarcode));
					diag = cdd_UpceWoCheckDigAndLead0;
					cdstd = BARCSTD_UPCE;
					(norm_code = 0).CatChar('0').Cat(pBarcode).CatChar(cd + '0');
					ok = -1;
				}
				else
					diag = cddUnknownFormat;
			}
		}
	}
	ASSIGN_PTR(pDiag, diag);
	ASSIGN_PTR(pStd, cdstd);
	ASSIGN_PTR(pNormalizedCode, norm_code);
	return ok;
}
//
//
//
#define DEF_MIN_UNIQ_BC_LEN  -11002

static int SLAPI _Lock(PPID id)
{
	int    r;
	if(id) {
		PPID   mutex_id = 0;
		PPSyncItem sync_item;
		if((r = DS.GetSync().CreateMutex(LConfig.SessionID, PPOBJ_GOODS, id, &mutex_id, &sync_item)) < 0)
			return PPSetError(PPERR_GOODSISLOCKED, sync_item.Name);
		else if(r == 0)
			return PPSetError(PPERR_LOGICLOCKFAULT);
	}
	return 1;
}

static int SLAPI _Unlock(PPID id)
{
	return id ? DS.GetSync().ReleaseMutex(PPOBJ_GOODS, id) : 1;
}

int SLAPI PPObjGoods::Lock(PPID id)
{
	int    ok = 1, is_locked = 0;
	if(id && !Locks.bsearch(id)) {
		THROW(_Lock(id));
		is_locked = 1;
		THROW_SL(Locks.ordInsert(id, 0));
	}
	CATCH
		if(is_locked)
			_Unlock(id);
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI PPObjGoods::Unlock(PPID id)
{
	int    ok = 1;
	uint   p = 0;
	if(id && Locks.bsearch(id, &p)) {
		_Unlock(Locks.at(p));
		Locks.atFree(p);
	}
	return ok;
}

void SLAPI PPObjGoods::InitInstance(SCtrLite sctr, void * extraPtr)
{
	Sctr = sctr;
	//TLP_OPEN(tbl);
	TLP_OPEN(P_Tbl);
	ExtraPtr = extraPtr;
	Kind = PPGDSK_GOODS;
	EcoSel = 0;
	P_PsnObj = 0;
	HistGoods = 0;
	if(Sctr != SConstructorLite && CConfig.Flags & CCFLG_USEHISTGOODS)
		TLP_OPEN(HistGoods);
}

// Protected constructor
SLAPI PPObjGoods::PPObjGoods(PPID objType, void * extraPtr) : PPObject(objType)
{
	InitInstance(SConstructorDef, extraPtr);
}

SLAPI PPObjGoods::PPObjGoods(void * extraPtr) : PPObject(PPOBJ_GOODS)
{
	InitInstance(SConstructorDef, extraPtr);
	P_Tbl->FetchConfig(&Cfg);
}

SLAPI PPObjGoods::PPObjGoods(SCtrLite sctr) : PPObject(PPOBJ_GOODS)
{
	InitInstance(sctr, 0);
	if(Sctr != SConstructorLite)
		P_Tbl->FetchConfig(&Cfg);
}

SLAPI PPObjGoods::~PPObjGoods()
{
	delete P_PsnObj;
	// @v8.0.9 {
	if(HistGoods) {
		TLP_CLOSE(HistGoods);
	}
	// } @v8.0.9
	TLP_CLOSE(P_Tbl);
}

int SLAPI PPObjGoods::Search(PPID id, void * b)
{
	return SearchByID(P_Tbl, Obj, id, b);
}

const char * SLAPI PPObjGoods::GetNamePtr()
{
	return P_Tbl->data.Name;
}

int SLAPI PPObjGoods::EditRights(uint bufSize, ObjRights * rt, EmbedDialog * pDlg)
{
	return (Kind == PPGDSK_GOODS) ?
		EditSpcRightFlags(DLG_RTGOODS, 0, 0, bufSize, rt, pDlg) : PPObject::EditRights(bufSize, rt, pDlg);
}

int SLAPI PPObjGoods::InitPacket(PPGoodsPacket * pPack, GoodsPacketKind gpk, PPID parentID, PPID clsID, const char * pBarcode)
{
	MEMSZERO(pPack->Rec);
	MEMSZERO(pPack->ExtRec);
	pPack->Codes.freeAll();
	pPack->GS.Items.freeAll();
	pPack->P_Filt = 0;
	Goods2Tbl::Rec parent_rec;
	switch(gpk) {
		case gpkndGoods:
			pPack->Rec.Kind = PPGDSK_GOODS;
			pPack->Rec.GdsClsID = clsID;
			break;
		case gpkndOrdinaryGroup:
			pPack->Rec.Kind = PPGDSK_GROUP;
			break;
		case gpkndFolderGroup:
			pPack->Rec.Kind = PPGDSK_GROUP;
			pPack->Rec.Flags |= GF_FOLDER;
			break;
		case gpkndAltGroup:
			pPack->Rec.Kind = PPGDSK_GROUP;
			pPack->Rec.Flags |= GF_ALTGROUP;
			break;
	}
	if(parentID && P_Tbl->Fetch(parentID, &parent_rec) > 0 && parent_rec.Kind == PPGDSK_GROUP) {
		pPack->Rec.ParentID = parentID;
		if(!(parent_rec.Flags & GF_ALTGROUP)) {
			pPack->Rec.UnitID   = parent_rec.UnitID;
			pPack->Rec.PhUnitID = parent_rec.PhUnitID;
			if(oneof2(pPack->Rec.Kind, PPGDSK_GOODS, PPGDSK_GROUP)) {
				SETIFZ(pPack->Rec.GdsClsID, parent_rec.GdsClsID);
				pPack->Rec.GoodsTypeID = parent_rec.GoodsTypeID;
				// @v7.8.1 {
				if(pPack->Rec.Kind == PPGDSK_GOODS) {
					if(parent_rec.StrucID) {
						PPGoodsStruc gs;
						if(GSObj.Get(parent_rec.StrucID, &gs) > 0) {
							if(gs.IsNamed()) {
								pPack->GS = gs;
							}
							else {
								// @todo К группе привязана неименованная структура
							}
						}
					}
				}
				// } @v7.8.1
			}
		}
	}
	if(pPack->Rec.Kind == PPGDSK_GOODS && pPack->Rec.GdsClsID) {
		PPObjGoodsClass gc_obj;
		PPGdsClsPacket gc_pack;
		if(gc_obj.Fetch(pPack->Rec.GdsClsID, &gc_pack) > 0) {
			if(gc_pack.Rec.GetDynGenMask(PPGdsCls::eKind)) {
				const PPID obj_type_id = gc_pack.PropKind.ItemsListID;
				if(obj_type_id > 0 && (IS_DYN_OBJTYPE(obj_type_id) || obj_type_id < 1000)) {
					PPObjReference ref_obj(obj_type_id, 0);
					PPID single_id = ref_obj.GetSingle();
					if(single_id)
						pPack->ExtRec.KindID = single_id;
				}
			}
			if(gc_pack.Rec.GetDynGenMask(PPGdsCls::eGrade)) {
				const PPID obj_type_id = gc_pack.PropGrade.ItemsListID;
				if(obj_type_id > 0 && (IS_DYN_OBJTYPE(obj_type_id) || obj_type_id < 1000)) {
					PPObjReference ref_obj(obj_type_id, 0);
					PPID single_id = ref_obj.GetSingle();
					if(single_id)
						pPack->ExtRec.GradeID = single_id;
				}
			}
			if(gc_pack.Rec.GetDynGenMask(PPGdsCls::eAdd)) {
				const PPID obj_type_id = gc_pack.PropAdd.ItemsListID;
				if(obj_type_id > 0 && (IS_DYN_OBJTYPE(obj_type_id) || obj_type_id < 1000)) {
					PPObjReference ref_obj(obj_type_id, 0);
					PPID single_id = ref_obj.GetSingle();
					if(single_id)
						pPack->ExtRec.AddObjID = single_id;
				}
			}
			if(gc_pack.Rec.GetDynGenMask(PPGdsCls::eAdd2)) {
				const PPID obj_type_id = gc_pack.PropAdd2.ItemsListID;
				if(obj_type_id > 0 && (IS_DYN_OBJTYPE(obj_type_id) || obj_type_id < 1000)) {
					PPObjReference ref_obj(obj_type_id, 0);
					PPID single_id = ref_obj.GetSingle();
					if(single_id)
						pPack->ExtRec.AddObj2ID = single_id;
				}
			}
		}
		else
			pPack->Rec.GdsClsID = 0;
	}
	if(oneof3(gpk, gpkndOrdinaryGroup, gpkndFolderGroup, gpkndAltGroup))
		return BIN(pPack->SetGroupCode(pBarcode));
	else
		return BIN(pPack->AddCode(pBarcode, 0, 1));
}
/*
	PPERR_NAMENEEDED
	PPERR_ABBRNEEDED
	PPERR_UNITNEEDED
	PPERR_INVALIDPHUPERU
	PPERR_GOODSGROUPNEEDED
	PPERR_GOODSGRPLOOP
	PPERR_GGRPFOLDERNEEDED
	PPERR_INVBARCODE
	PPERR_INVBCODELEN
	PPERR_INVBCODEPRFX
	PPERR_DUPBARCODE
	PPERR_BARCODENEEDED
*/
int SLAPI PPObjGoods::ValidatePacket(const PPGoodsPacket * pPack)
{
	int    ok = -1;
	if(pPack) {
		SString temp_buf = pPack->Rec.Name;
		THROW_PP(temp_buf.NotEmptyS(), PPERR_NAMENEEDED);
		if(pPack->Rec.Kind == PPGDSK_GOODS) {
			temp_buf = pPack->Rec.Abbr;
			THROW_PP(temp_buf.NotEmptyS(), PPERR_ABBRNEEDED);
			THROW_PP(pPack->Rec.UnitID, PPERR_UNITNEEDED);
			THROW_PP(!pPack->Rec.PhUnitID || (pPack->Rec.PhUPerU > 0.0 && pPack->Rec.PhUPerU != HUGE_VAL), PPERR_INVALIDPHUPERU);
			THROW_PP(pPack->Rec.ParentID, PPERR_GOODSGROUPNEEDED);
		}
		THROW_PP(!pPack->Rec.ID || pPack->Rec.ID != pPack->Rec.ParentID, PPERR_GOODSGRPLOOP);
		if(pPack->Rec.ParentID) {
			Goods2Tbl::Rec goods_rec;
			THROW(Fetch(pPack->Rec.ParentID, &goods_rec) > 0);
			if(pPack->Rec.Kind != PPGDSK_GOODS)
				THROW_PP(goods_rec.Flags & GF_FOLDER, PPERR_GGRPFOLDERNEEDED);
		}
		THROW(CheckBarcodeList(pPack));
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjGoods::IsPacketEq(const PPGoodsPacket & rS1, const PPGoodsPacket & rS2, long options, long * pResultFlags)
{
	int    eq = 1;
	long   result_flags = 0;
	{
		const uint c = rS1.Codes.getCount();
		if(c != rS2.Codes.getCount())
			eq = 0;
		else {
			for(uint i = 0; eq && i < c; i++) {
				if(!P_Tbl->GetBcTbl_().fields.IsEqualRecords(&rS1.Codes.at(i), &rS2.Codes.at(i)))
					eq = 0;
			}
		}
		if(eq)
			result_flags |= pefCodeEq;
	}
	if(eq) {
		const int ext_empty1 = BIN(rS1.IsExtRecEmpty());
		const int ext_empty2 = BIN(rS2.IsExtRecEmpty());
		if(ext_empty1 != ext_empty2)
			eq = 0;
		else if(!ext_empty1 && !P_Tbl->GetExtTbl_().fields.IsEqualRecords(&rS1.ExtRec, &rS2.ExtRec))
			eq = 0;
		else if(!rS1.Stock.IsEqual(rS2.Stock))
			eq = 0;
	}
	if(eq) {
		const uint c = rS1.ArCodes.getCount();
		if(c != rS2.ArCodes.getCount())
			eq = 0;
		else {
			for(uint i = 0; eq && i < c; i++) {
				if(!P_Tbl->GetACodTbl_().fields.IsEqualRecords(&rS1.ArCodes.at(i), &rS2.ArCodes.at(i)))
					eq = 0;
			}
		}
	}
	if(eq) {
		if(rS1.ExtString != rS2.ExtString)
			eq = 0;
		else if(rS1.ExTitles != rS2.ExTitles)
			eq = 0;
		else if(!rS1.GS.IsEqual(rS2.GS))
			eq = 0;
		else if(rS1.P_Filt && !rS2.P_Filt)
			eq = 0;
		else if(!rS1.P_Filt && rS2.P_Filt)
			eq = 0;
		else if(rS1.P_Filt && rS2.P_Filt && !rS1.P_Filt->IsEqual(rS2.P_Filt, 0))
			eq = 0;
		else if(!rS1.TagL.IsEqual(rS2.TagL))
			eq = 0;
		else if(rS1.LinkFiles.IsChanged(rS1.Rec.ID, 0L))
			eq = 0;
	}
	if(eq) {
		//
		// Сравнение полей основной записи оставим на последний 'тап дабы можно было
		// идентифицировать факт изменения только флагов записи.
		//
		LongArray neq_fld_list;
		if(!P_Tbl->fields.IsEqualRecords(&rS1.Rec, &rS2.Rec, &neq_fld_list)) {
			if(neq_fld_list.getCount() == 1 && neq_fld_list.get(0) == P_Tbl->Flags.fld)
				result_flags |= pefRecFlagsOnly;
			eq = 0;
		}
	}
	ASSIGN_PTR(pResultFlags, result_flags);
	return eq;
}

int SLAPI PPObjGoods::GetValueAddedData(PPID id, PPGoodsPacket * pPack)
{
	int    ok = -1;
	pPack->ExtString = 0;
	if(pPack->Rec.Flags & GF_EXTPROP) {
		THROW(PPRef->GetPropVlrString(PPOBJ_GOODS, id, GDSPRP_EXTSTRDATA, pPack->ExtString));
		if(pPack->ExtString.NotEmptyS())
			ok = 1;
	}
	CATCHZOK
	return ok;
}

#define GOODSPACK_VER 3 // @v6.2.0 0-->1; @v7.2.0 1-->2; @v7.7.12 2-->3

int SLAPI PPObjGoods::SerializePacket_(int toBuf, PPGoodsPacket * pPack, SBuffer * pBuf)
{
	/* Фиксированная часть записи HistGoodsTbl
	{
		autolong ID;
		long   GoodsID;
		long   Ver;
		long   Sz;
	}
	*/
	int    ok = 1;
	assert(pBuf);
	if(toBuf) {
		SString temp_buf;
		uint16 pack_ver = GOODSPACK_VER;
		long   ver = 0;
		long   data_size = 0;
		PPID   id = 0;
		pBuf->Write(id);
		pBuf->Write(pPack->Rec.ID);
		pBuf->Write(ver = TLP(HistGoods).IsOpened() ? HistGoods->GetNextVer(pPack->Rec.ID) : 0);
		pBuf->Write(data_size); // Нулевое значение. В предыдущих версиях инициализировалось, но без необходимости.
		pBuf->Write(pack_ver);
		pBuf->Write(&pPack->Rec, sizeof(pPack->Rec));
		pBuf->Write(&pPack->Codes);
		pBuf->Write(&pPack->ArCodes);
		pBuf->Write(&pPack->ExtRec, sizeof(pPack->ExtRec));
		pBuf->Write(&pPack->Stock, offsetof(GoodsStockExt, MinStockList));
		pBuf->Write(&pPack->Stock.MinStockList);
		// @v7.2.0 {
		pBuf->Write(&pPack->Stock.PltList);
		pPack->TagL.Write__(*pBuf);
		// } @v7.2.0
		pBuf->Write(pPack->ExtString);
		pBuf->Write(pPack->ExTitles);
		pPack->GenericList.Write(*pBuf); // @v7.7.12
		{
			//
			// CRC рассчитываем начиная со смещения sizeof(long) из-за того, что
			// после записи в БД первые 4 байта будут содержать идентификатор.
			//
			CRC32 c;
			ulong crc = c.Calc(0, ((const uint8 *)pBuf->GetBuf())+sizeof(long), pBuf->GetAvailableSize()-sizeof(long));
			pBuf->Write(crc);
		}
	}
	else {
		uint16 pack_ver = 0;
		long   ver = 0;
		long   size = 0;
		PPID   id = 0;
		//
		// Считываем содержимое буфера во временный буфер b для вычисления CRC32
		// (для версий пакета 1 или более).
		//
		size_t data_len = pBuf->GetAvailableSize();
		STempBuffer b(data_len);
		THROW_SL(b.IsValid());
		pBuf->ReadStatic(b, data_len);
		//
		pPack->destroy();
		pBuf->Read(id);
		pBuf->Read(pPack->Rec.ID);
		pBuf->Read(ver);
		pBuf->Read(size);
		pBuf->Read(pack_ver);
		if(pack_ver < 1) {
			uint16 len_extstr = 0;
			uint16 len_extitles = 0;

			pBuf->Read(&pPack->Codes);
			pBuf->Read(&pPack->Rec,    sizeof(pPack->Rec));
			pBuf->Read(&pPack->ExtRec, sizeof(pPack->ExtRec));
			pBuf->Read(&pPack->Stock,  offsetof(GoodsStockExt, MinStockList));
			// pBuf->Read(&pPack->Stock.MinStockList);
			pBuf->Read(len_extstr);
			if(len_extstr) {
				STempBuffer temp_buf(len_extitles);
				pBuf->Read(temp_buf, len_extstr);
				pPack->ExtString = temp_buf;
			}
			pBuf->Read(len_extitles);
			if(len_extitles) {
				STempBuffer temp_buf(len_extitles);
				pBuf->Read(temp_buf, len_extitles);
				pPack->ExTitles = temp_buf;
			}
		}
		else {
			CRC32 c;
			ulong crc = 0, crc2;
			crc = c.Calc(0, ((const uint8 *)(char *)b)+sizeof(long), data_len-sizeof(crc)-sizeof(long));
			pBuf->Read(&pPack->Rec, sizeof(pPack->Rec));
			pBuf->Read(&pPack->Codes);
			pBuf->Read(&pPack->ArCodes);
			pBuf->Read(&pPack->ExtRec, sizeof(pPack->ExtRec));
			pBuf->Read(&pPack->Stock, offsetof(GoodsStockExt, MinStockList));
			pBuf->Read(&pPack->Stock.MinStockList);
			// @v7.2.0 {
			if(pack_ver >= 2) {
				pBuf->Read(&pPack->Stock.PltList);
				pPack->TagL.Read__(*pBuf);
			}
			// } @v7.2.0
			pBuf->Read(pPack->ExtString);
			pBuf->Read(pPack->ExTitles);
			// @v7.7.12 {
			if(pack_ver >= 3) {
				THROW(pPack->GenericList.Read(*pBuf));
			}
			// } @v7.7.12
			pBuf->Read(crc2);
			THROW_PP(crc2 == crc, PPERR_HISTGOODSBUFCORRUPTED);
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjGoods::GetPacket(PPID id, PPGoodsPacket * pPack, long options)
{
	int    ok = 1;
	id = labs(id);
	/* @v8.3.7
	ZDELETE(pPack->P_Filt);
	ZDELETE(pPack->P_Quots);
	pPack->ExTitles = 0;
	pPack->TagL.Destroy(); // @v7.0.x AHTOXA
	*/
	pPack->destroy(); // @v8.3.7
	if((ok = Search(id, &pPack->Rec)) > 0) {
		if(pPack->Rec.PhUPerU == HUGE_VAL)
			pPack->Rec.PhUPerU = 0.0;
		THROW(ReadBarcodes(id, pPack->Codes));
		THROW(P_Tbl->ReadArCodes(id, &pPack->ArCodes));
		THROW(P_Tbl->GetExt(id, &pPack->ExtRec));
		THROW(GetStockExt(id, &pPack->Stock, 0));
		if(pPack->Rec.StrucID) {
			int r = GSObj.Get(pPack->Rec.StrucID, &pPack->GS);
			THROW(r);
			if(r > 0)
				pPack->GS.GoodsID = id;
			else {
				SString msg_buf;
				msg_buf.Cat(pPack->Rec.Name).CatDiv(';', 2).CatEq("ID", id).CatDiv(';', 2).CatEq("StrucID", pPack->Rec.StrucID);
				PPSetError(PPERR_STRUCBYGOODSNFOUND, msg_buf);
				PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_USER|LOGMSGF_TIME|LOGMSGF_DBINFO|LOGMSGF_LASTERR);
				pPack->GS.GoodsID = 0;
			}
		}
		if(oneof2(pPack->Rec.Kind, PPGDSK_GOODS, PPGDSK_GROUP) && !(options & gpoSkipQuot)) { // @v8.3.7 && !(options & gpoSkipQuot)
			PPQuotArray quots;
			if(GetQuotList(pPack->Rec.ID, 0, quots) > 0 && quots.getCount())
				THROW_MEM(pPack->P_Quots = new PPQuotArray(quots));
		}
		if(pPack->Rec.Kind == PPGDSK_GROUP && pPack->Rec.Flags & GF_ALTGROUP) {
			GoodsFilt flt;
			if(flt.ReadFromProp(PPOBJ_GOODSGROUP, id, GGPRP_GOODSFILT2, GGPRP_GOODSFLT_) > 0)
				if(!flt.IsEmpty())
					THROW_MEM(pPack->P_Filt = new GoodsFilt(flt));
		}
		THROW(GetValueAddedData(id, pPack));
		if(pPack->Rec.Kind == PPGDSK_GROUP)
			THROW(PPRef->GetPropVlrString(PPOBJ_GOODSGROUP, id, GGPRP_EXSTRTITLES, pPack->ExTitles));
		THROW(GetTagList(pPack->Rec.ID, &pPack->TagL)); // @v7.0.x AHTOXA
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

//static
int FASTCALL PPObjGoods::HasUndupNameSuffix(const SString & rBuf, uint * pPos)
{
	int    ok = 0;
	size_t pos = rBuf.Len();
	uint   nd = 0;
	ASSIGN_PTR(pPos, 0);
	if(pos) do {
		const char c = rBuf.C(--pos);
		if(c == '#') {
			if(pos > 5 && nd > 0 && nd < 3) {
				ASSIGN_PTR(pPos, pos);
				ok = 1;
			}
		}
		else if(c >= '0' && c <= '9') {
			nd++;
		}
		else
			break;
	} while(!ok && pos);
	return ok;
}

// static
int FASTCALL PPObjGoods::RemoveUndupNameSuffix(SString & rBuf)
{
	int    ok = -1;
	size_t pos = rBuf.Len();
	uint   nd = 0;
	if(pos) do {
		const char c = rBuf.C(--pos);
		if(c == '#') {
			if(pos > 5 && nd > 0 && nd < 3) {
				rBuf.Trim(pos-1).Strip();
				ok = 1;
			}
		}
		else if(c >= '0' && c <= '9') {
			nd++;
		}
		else
			break;
	} while(ok < 0 && pos);
	return ok;
}

int SLAPI PPObjGoods::ForceUndupName(PPID id, SString & rBuf)
{
	int    ok = -1;
	const  size_t max_nm_len = sizeof(((Goods2Tbl::Rec*)0)->Name)-1;
	SString new_name = rBuf;
	SString suffix, temp_buf;
	PPID   temp_nm_goods_id = 0;
	long   uc = 1;
	temp_buf = rBuf;
	while(SearchByName(temp_buf, &temp_nm_goods_id, 0) > 0 && temp_nm_goods_id != id) {
		if(uc == 1)
			RemoveUndupNameSuffix(new_name);
		(suffix = 0).Space().CatChar('#').Cat(++uc);
		temp_buf = new_name;
		size_t sum_len = temp_buf.Len() + suffix.Len();
		if(sum_len > max_nm_len)
			temp_buf.Trim(max_nm_len-suffix.Len());
		temp_buf.Cat(suffix);
		ok = 1;
	}
	if(ok > 0)
		rBuf = temp_buf;
	return ok;
}

int SLAPI PPObjGoods::UpdateName(PPID id, const char * pNewName, int use_ta)
{
	int    ok = -1;
	SString new_name = pNewName;
	THROW(CheckRights(PPR_MOD));
	if(new_name.NotEmptyS()) {
		Goods2Tbl::Rec rec;
		PPTransaction tra(use_ta);
		THROW(tra);
		ForceUndupName(id, new_name);
		if(SearchByID(P_Tbl, PPOBJ_GOODS, id, &rec) > 0) {
			if(new_name != rec.Name || new_name != rec.Abbr) {
				PPID   hid = 0;
				DBRowId pos;
				THROW_DB(P_Tbl->getPosition(&pos));
				if(TLP(HistGoods).IsOpened()) {
					SBuffer hist_buf;
					PPGoodsPacket old_pack;
					THROW(GetPacket(id, &old_pack, 0) > 0);
					THROW(SerializePacket_(1, &old_pack, &hist_buf));
					THROW(HistGoods->Insert(&hid, &hist_buf, 0));
				}
				THROW_DB(P_Tbl->getDirectForUpdate(0, 0, pos));
				P_Tbl->copyBufTo(&rec);
				new_name.CopyTo(rec.Name, sizeof(rec.Name));
				new_name.CopyTo(rec.Abbr, sizeof(rec.Abbr));
				P_Tbl->copyBufFrom(&rec);
				THROW_DB(P_Tbl->updateRec());
				DS.LogAction(PPACN_OBJUPD, Obj, id, hid, 0);
				ok = 1;
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjGoods::PutPacket(PPID * pID, PPGoodsPacket * pPack, int use_ta)
{
	int    ok = 1;
	int    no_changes = 0; // Устанавливается в 1, если пакет не изменился по сравнению с образом в БД.
	long   pack_cmp_result_flags = 0;
	const  PPID mtx_qk_id = GetConfig().MtxQkID;
	const  int is_new = (*pID == 0);
	const  int is_removing = BIN(*pID != 0 && pPack == 0);
	PPID   remove_struc_id = 0;
	PPID   hid = 0;
	Goods2Tbl::Rec rec, org_rec, grp_rec;
	SBuffer hist_buf;
	PPGoodsPacket old_pack;
	PPGoodsStrucHeader gs_rec;
	PPID   def_prc_id = 0;
	Reference * p_ref = PPRef;
	PPObjTech * p_tec_obj = 0;
	PPObjProcessor * p_prc_obj = 0;
	PPUserFuncProfiler ufp(PPUPRF_GOODSPUT); // @v8.1.3
	//
	// Выясняем возможность создания автотехнологии по новому товару
	//
	if(pPack && pPack->Rec.Kind == PPGDSK_GOODS && Search(pPack->Rec.ParentID, &grp_rec) > 0 && grp_rec.DefPrcID) {
		def_prc_id = grp_rec.DefPrcID;
		p_tec_obj = new PPObjTech;
		p_prc_obj = new PPObjProcessor;
	}
	//
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID) {
			// @v8.0.3 THROW(Search(*pID, &rec) > 0);
			// @v8.0.3 org_rec = rec;
			THROW(GetPacket(*pID, &old_pack, 0) > 0); // @v7.9.6 (!0)-->(>0)
			rec = old_pack.Rec;     // @v8.0.3
			org_rec = old_pack.Rec; // @v8.0.3
			if(pPack) {
				// @v7.8.2 {
				// Подготовка пакета к корректному сравнению с оригинальным пакетом:
				// если pPack пришел из другого раздела, то идентификаторы товара в структурах, хранящихся в
				// других таблицах могут отличаться от *pID.
				//
				uint   i;
				if(!pPack->IsExtRecEmpty())
					pPack->ExtRec.GoodsID = *pID;
				for(i = 0; i < pPack->Codes.getCount(); i++) {
					pPack->Codes.at(i).GoodsID = *pID;
				}
				for(i = 0; i < pPack->ArCodes.getCount(); i++) {
					pPack->ArCodes.at(i).GoodsID = *pID;
				}
				// } @v7.8.2
				// @v8.1.3 {
				// Если не надо менять изображение, то не следует и сравнивать флаг GF_HASIMAGES
				if(pPack->UpdFlags & PPGoodsPacket::ufDontChgImgFlag) {
					SETFLAG(pPack->Rec.Flags, GF_HASIMAGES, org_rec.Flags & GF_HASIMAGES);
				}
				//
				// GoodsExt::UniqCntr является автоматически управляемым полем. Отличие его
				// значения не должны приводить к изменению товара
				//
				const long preserve_org_ext_uniq_cntr = old_pack.ExtRec.UniqCntr;
				const long preserve_new_ext_uniq_cntr = pPack->ExtRec.UniqCntr;
				old_pack.ExtRec.UniqCntr = 0;
				pPack->ExtRec.UniqCntr = 0;
				{
					//
					// Из другого раздела может прийти товар с незначащими записями
					// в MinStockList.
					//
					i = pPack->Stock.MinStockList.getCount();
					if(i) do {
						--i;
						if(pPack->Stock.MinStockList.at(i).Val <= 0.0)
							pPack->Stock.MinStockList.atFree(i);
					} while(i);
				}
				// } @v8.1.3
				if(IsPacketEq(*pPack, old_pack, 0, &pack_cmp_result_flags))
					no_changes = 1;
				// @v8.1.3 {
				old_pack.ExtRec.UniqCntr = preserve_org_ext_uniq_cntr;
				pPack->ExtRec.UniqCntr = preserve_new_ext_uniq_cntr;
				// } @v8.1.3
			}
			if(!is_new && !no_changes && TLP(HistGoods).IsOpened()) {
				THROW(SerializePacket_(1, &old_pack, &hist_buf));
				//old_pack.ToHistPack(&hist_buf);
			}
		}
		if(pPack == 0) {
			if(*pID) {
				THROW(CheckRights(PPR_DEL));
				THROW(P_Tbl->Update(pID, 0, 0));
				if(rec.StrucID && GSObj.Search(rec.StrucID, &gs_rec) > 0) {
					if(!(gs_rec.Flags & GSF_NAMED)) {
						int    has_forein_ref = 0;
						PPIDArray struc_ref_goods_list;
						P_Tbl->SearchGListByStruc(rec.StrucID, &struc_ref_goods_list);
						for(uint k = 0; !has_forein_ref && k < struc_ref_goods_list.getCount(); k++) {
							PPID   ref_goods_id = struc_ref_goods_list.get(k);
							if(ref_goods_id && ref_goods_id != *pID)
								has_forein_ref = 1;
						}
						if(!has_forein_ref)
							remove_struc_id = rec.StrucID;
					}
				}
				THROW(P_Tbl->PutExt(*pID, 0, 0));
				THROW(p_ref->PutPropVlrString(Obj, *pID, GDSPRP_EXTSTRDATA, 0));
				THROW(P_Tbl->PutStockExt(*pID, 0, 0));
				THROW(p_ref->PutPropVlrString(PPOBJ_GOODSGROUP, *pID, GGPRP_EXSTRTITLES, 0));
				THROW(p_ref->RemoveProp(PPOBJ_GOODSGROUP, *pID, GGPRP_GOODSFILT2, 0));
				THROW(p_ref->RemoveProp(PPOBJ_GOODSGROUP, *pID, GGPRP_GOODSFLT_, 0));
				THROW(PutQuotList(*pID, 0, 0));
				old_pack.LinkFiles.Init(PPOBJ_GOODS);
				old_pack.LinkFiles.Save(*pID, 0L);
				THROW(SetTagList(*pID, 0, 0));
			}
		}
		else if(!no_changes) {
			THROW(CheckRightsModByID(pID));
			if(oneof2(pPack->Rec.Kind, PPGDSK_GOODS, PPGDSK_GROUP)) {
				if(*pID) {
					if(org_rec.TaxGrpID != pPack->Rec.TaxGrpID) {
						if(pPack->UpdFlags & PPGoodsPacket::ufDontChgTaxGrp)
							pPack->Rec.TaxGrpID = org_rec.TaxGrpID;
						else {
							// Флаг прав доступа GOODSRT_UPDTAXGRP определяется только для //
							// объекта PPOBJ_GOODS, но распространяется так же и на        //
							// объект PPOBJ_GOODSGROUP                                     //
							PPObjGoods goods_obj;
							THROW_PP_S(goods_obj.CheckRights(GOODSRT_UPDTAXGRP), PPERR_NRT_GOODS_UPDTAXGRP, pPack->Rec.Name);
						}
					}
					if(org_rec.GoodsTypeID != pPack->Rec.GoodsTypeID) {
						// Флаг прав доступа GOODSRT_UPDGTYPE определяется только для //
						// объекта PPOBJ_GOODS, но распространяется так же и на        //
						// объект PPOBJ_GOODSGROUP                                     //
						PPObjGoods goods_obj;
						THROW_PP_S(goods_obj.CheckRights(GOODSRT_UPDGTYPE), PPERR_NRT_GOODS_UPDGTYPE, pPack->Rec.Name);
					}
				}
				if(pPack->UpdFlags & PPGoodsPacket::ufDontChgImgFlag)
					SETFLAG(pPack->Rec.Flags, GF_HASIMAGES, org_rec.Flags & GF_HASIMAGES);
			}
			//
			// Обработка структуры товара {
			//
			PPID   struc_id = 0;
			if(pPack->Rec.Kind == PPGDSK_GOODS) {
				struc_id = pPack->GS.Rec.ID;
				if(pPack->GS.IsNamed()) {
					//
					// Если структура, ассоциированная с товаром была собственной, то ее удаляем
					//
					if(*pID && rec.StrucID && rec.StrucID != struc_id && GSObj.Search(rec.StrucID, &gs_rec) > 0) {
						if(!(gs_rec.Flags & GSF_NAMED))
							remove_struc_id = rec.StrucID;
					}
					if(pPack->UpdFlags & PPGoodsPacket::ufChgNamedStruc)
						THROW(GSObj.Put(&struc_id, &pPack->GS, 0));
				}
				else {
					if(*pID && struc_id && pPack->GS.IsEmpty()) { // Struc must be deleted
						PPID   temp_id = *pID;
						rec.StrucID = 0;
						THROW(P_Tbl->Update(&temp_id, &rec, 0));
					}
					THROW(GSObj.Put(&struc_id, &pPack->GS, 0));
				}
			}
			else if(pPack->Rec.Kind == PPGDSK_GROUP) {
				struc_id = pPack->Rec.StrucID;
			}
			pPack->Rec.StrucID = struc_id;
			//
			// }
			//
			SETFLAG(pPack->Rec.Flags, GF_EXTPROP, pPack->ExtString.NotEmpty());
			THROW(P_Tbl->Update(pID, &pPack->Rec, 0));
			if(pPack->Rec.Kind == PPGDSK_GOODS && GetConfig().Flags & GCF_AUTOPREFBARCODE) {
				uint   pref_item_pos = 0, single_item_pos = 0;
				BarcodeTbl::Rec * p_pref_item = pPack->Codes.GetPreferredItem(&pref_item_pos);
				if(!p_pref_item) {
					BarcodeTbl::Rec * p_single_item = pPack->Codes.GetSingleItem(&single_item_pos);
					if(p_single_item) {
						pPack->Codes.SetPreferredItem(single_item_pos);
						pack_cmp_result_flags &= ~pefCodeEq; // @v8.1.3
					}
				}
			}
			if(!(pack_cmp_result_flags & pefCodeEq)) { // @v8.1.3
				THROW(P_Tbl->UpdateBarcodes(*pID, &pPack->Codes, 0));
			}
			if(pPack->Rec.Kind == PPGDSK_GOODS) {
				if(is_new || !(pPack->UpdFlags & PPGoodsPacket::ufDontChgArCodes))
					THROW(P_Tbl->UpdateArCodes(*pID, &pPack->ArCodes, 0));
			}
			if(pPack->Rec.Kind == PPGDSK_GOODS && pPack->Rec.GdsClsID && !pPack->IsExtRecEmpty()) {
				pPack->ExtRec.GoodsClsID = pPack->Rec.GdsClsID;
				THROW(P_Tbl->PutExt(*pID, &pPack->ExtRec, 0));
			}
			else
				THROW(P_Tbl->PutExt(*pID, 0, 0));
			{
				if(pPack->UpdFlags & PPGoodsPacket::ufDontChgPltList) {
					GoodsStockExt prev_gse;
					if(P_Tbl->GetStockExt(*pID, &prev_gse, 0) > 0 && prev_gse.PltList.getCount())
						pPack->Stock.PltList = prev_gse.PltList;
				}
				if(pPack->Rec.Kind == PPGDSK_GOODS && !pPack->Stock.IsEmpty()) {
					THROW(P_Tbl->PutStockExt(*pID, &pPack->Stock, 0));
				}
				else
					THROW(P_Tbl->PutStockExt(*pID, 0, 0));
			}
			if(pPack->Rec.Kind == PPGDSK_GROUP && (pPack->Rec.Flags & GF_ALTGROUP) && pPack->P_Filt)
				THROW(pPack->P_Filt->WriteToProp(PPOBJ_GOODSGROUP, *pID, GGPRP_GOODSFILT2, GGPRP_GOODSFLT_));
			THROW(p_ref->PutPropVlrString(Obj, *pID, GDSPRP_EXTSTRDATA, pPack->ExtString));
			if(pPack->Rec.Kind == PPGDSK_GROUP)
				THROW(p_ref->PutPropVlrString(PPOBJ_GOODSGROUP, *pID, GGPRP_EXSTRTITLES, pPack->ExTitles));
			if(pPack->LinkFiles.IsChanged(*pID, 0L)) {
				PPObjGoods goods_obj;
				THROW_PP(goods_obj.CheckRights(GOODSRT_UPDIMAGE), PPERR_NRT_UPDIMAGE);
				pPack->LinkFiles.Save(*pID, 0L);
			}
			THROW(SetTagList(*pID, &pPack->TagL, 0));
		}
		if(!no_changes) {
			if(remove_struc_id)
				THROW(GSObj.Put(&remove_struc_id, 0, 0));
			if(def_prc_id && p_tec_obj) {
				THROW(p_tec_obj->CreateAutoTech(def_prc_id, *pID, 0, 0));
			}
			if(!is_new) {
				if(TLP(HistGoods).IsOpened())
					THROW(HistGoods->Insert(&hid, &hist_buf, 0));
			}
			if(!is_removing) {
				DS.LogAction((is_new ? PPACN_OBJADD : PPACN_OBJUPD), Obj, *pID, hid, 0);
				if(pPack) {
					if(is_new) {
						//
						// Создаваемый товар может быть неявно ассоциирован с товарной матрицей
						// из-за принадлежности группе, которая состоит в матрице.
						// Для правильной актуализации кэша работающими сеансами необходимо
						// сформировать событие, индицирующее этот факт.
						//
						if(mtx_qk_id) {
							int    has_mtx = 0;
							PPID   parent_id = pPack->Rec.ParentID;
							PPIDArray recur_list;
							Goods2Tbl::Rec parent_rec;
							while(!has_mtx && parent_id && Fetch(parent_id, &parent_rec) > 0) {
								if(recur_list.addUnique(parent_id) < 0) {
									//
									// Защита от рекурсии
									//
									break;
								}
								else {
									PPQuotArray ql;
									P_Tbl->GetQuotList(parent_id, 0, ql);
									for(uint i = 0; !has_mtx && i < ql.getCount(); i++)
										if(ql.at(i).Kind == mtx_qk_id && ql.at(i).Quot > 0.0)
											has_mtx = 1;
									if(!has_mtx)
										parent_id = parent_rec.ParentID;
								}
							}
							if(has_mtx)
								DS.LogAction(PPACN_MTXGOODSADD, Obj, *pID, parent_id, 0);
						}
					}
					else {
						if((org_rec.Flags & GF_NODISCOUNT) && (!(pPack->Rec.Flags & GF_NODISCOUNT)))
							DS.LogAction(PPACN_GOODSNODISRMVD, Obj, *pID, 0, 0);
						if(pPack->Rec.TaxGrpID != org_rec.TaxGrpID)
							DS.LogAction(PPACN_GOODSVATGRPCHD, Obj, *pID, org_rec.TaxGrpID, 0);
					}
				}
			}
			else if((old_pack.Rec.Flags & GF_TEMPALTGROUP) != GF_TEMPALTGROUP)
				DS.LogAction(PPACN_OBJRMV, Obj, *pID, hid, 0);
		}
		THROW(tra.Commit());
	}
	if(no_changes) {
		ok = -1;
	}
	else {
		if(!is_new)
			P_Tbl->Dirty(*pID);
		else if(!is_removing) {
			if(mtx_qk_id) {
				PPIDArray _goods_list;
				_goods_list.add(*pID);
				P_Tbl->DirtyMatrix(&_goods_list, 0);
			}
			if(GetConfig().Flags & GCF_ADDTOALTGRPWITHOUTMSG)
				SupplyAddingToAltGrp(*pID, use_ta);
		}
	}
	ufp.Commit(); // @v8.1.3
	CATCH
		if(is_new) {
			*pID = 0;
			if(pPack)
				pPack->Rec.ID = 0;
		}
		ok = 0;
	ENDCATCH
	delete p_prc_obj;
	delete p_tec_obj;
	return ok;
}

int SLAPI PPObjGoods::GetGoodsByExt(const GoodsExtTbl::Rec * pExtRec, PPID * pGoodsID, int createIfNExists, int use_ta)
{
	int    ok = -1;
	PPID   goods_id = 0;
	Goods2Tbl::Rec goods_rec;
	int    r = P_Tbl->SearchByExt(pExtRec, &goods_id, &goods_rec);
	THROW(r);
	if(r > 0) {
		ok = 1;
		ASSIGN_PTR(pGoodsID, goods_id);
	}
	else if(createIfNExists) {
		goods_id = 0;
		PPObjGoodsClass gc_obj;
		PPGdsClsPacket gc_pack;
		PPGoodsPacket pack;
		THROW(InitPacket(&pack, gpkndGoods, 0, pExtRec->GoodsClsID, 0));
		THROW(gc_obj.Fetch(pExtRec->GoodsClsID, &gc_pack) > 0);
		pack.ExtRec = *pExtRec;
		pack.ExtRec.GoodsID = 0;
		THROW(gc_pack.CompleteGoodsPacket(&pack));
		THROW(PutPacket(&goods_id, &pack, use_ta));
		ASSIGN_PTR(pGoodsID, goods_id);
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjGoods::FetchUnit(PPID unitID, PPUnit * pUnitRec)
{
	PPObjUnit unit_obj;
	return unit_obj.Fetch(unitID, pUnitRec);
}

int SLAPI PPObjGoods::FetchTax(PPID goodsID, LDATE dt, PPID opID, PPGoodsTaxEntry * pGtx)
{
	Goods2Tbl::Rec grec;
	if(pGtx)
		memzero(pGtx, sizeof(*pGtx));
	return (Fetch(goodsID, &grec) > 0 && grec.TaxGrpID) ? GTxObj.Fetch(grec.TaxGrpID, dt, opID, pGtx) : -1;
}

int SLAPI PPObjGoods::FetchCls(PPID goodsID, Goods2Tbl::Rec * pRec, PPGdsClsPacket * pClsPack)
{
	int    ok = -1;
	Goods2Tbl::Rec goods_rec;
	if(Fetch(goodsID, &goods_rec) > 0 && goods_rec.GdsClsID) {
		ASSIGN_PTR(pRec, goods_rec);
		PPObjGoodsClass gc_obj;
		ok = (gc_obj.Fetch(goods_rec.GdsClsID, pClsPack) > 0) ? 1 : -2;
	}
	return ok;
}

int SLAPI PPObjGoods::AdjPriceToTaxes(PPID taxGrpID, double taxFactor, double * pPrice, int exclSTax)
{
	double p = *pPrice;
	GTaxVect vect;
	PPGoodsTaxEntry gt;
	if(GTxObj.FetchByID(taxGrpID, &gt) > 0) {
		vect.Calc_(&gt, p, taxFactor, GTAXVF_AFTERTAXES, exclSTax ? GTAXVF_SALESTAX : 0);
		p = vect.GetValue(GTAXVF_BEFORETAXES);
	}
	*pPrice = p;
	return 1;
}

int SLAPI PPObjGoods::AdjCostToVat(PPID lotTaxGrpID, PPID goodsTaxGrpID,
	LDATE lotDate, double qtty, double * pCost /* In, Out */, int withOrWithout, int vatFreeSuppl)
{
	return Helper_AdjCostToVat(lotTaxGrpID, goodsTaxGrpID, lotDate, qtty, pCost, 0, withOrWithout, vatFreeSuppl);
}

int SLAPI PPObjGoods::CalcCostVat(PPID lotTaxGrpID, PPID goodsTaxGrpID,
	LDATE lotDate, double qtty, double cost, double * pSum, int withOrWithout, int vatFreeSuppl)
{
	return Helper_AdjCostToVat(lotTaxGrpID, goodsTaxGrpID, lotDate, qtty, &cost, pSum, withOrWithout, vatFreeSuppl);
}

int SLAPI PPObjGoods::Helper_AdjCostToVat(PPID lotTaxGrpID, PPID goodsTaxGrpID,
	LDATE lotDate, double qtty, double * pCost /* In, Out */, double * pVatSum, int withOrWithout, int vatFreeSuppl)
{
	double c = *pCost;
	double vat_sum = 0;
	PPID   tax_grp_id = NZOR(lotTaxGrpID, goodsTaxGrpID);
	GTaxVect vect;
	PPGoodsTaxEntry gt;
	if(GTxObj.Fetch(tax_grp_id, lotDate, 0L, &gt) > 0) {
		long   amt_fl = 0, val_fl = 0;
		long   excl_flags = (gt.Flags & GTAXF_NOLOTEXCISE) ? GTAXVF_EXCISE : 0;
		if(withOrWithout) {
			amt_fl = ~GTAXVF_VAT;
			val_fl = GTAXVF_VAT;
			//
			// @v4.2.11 {
			// Без этой вставки неправильно считалась сумма в ценах поступления //
			// в остатках товаров если товар оприходован с признаком "Цена поступления без НДС"
			// от поставщика, свободного от НДС
			//
			if(vatFreeSuppl > 0)
				excl_flags |= GTAXVF_VAT;
			// }
		}
		else {
			amt_fl = ~GTAXVF_SALESTAX;
			val_fl = GTAXVF_AFTERTAXES;
			if(vatFreeSuppl > 0)
				excl_flags |= GTAXVF_VAT;
		}
		vect.Calc_(&gt, c, qtty, amt_fl, excl_flags);
		vat_sum = vect.GetValue(GTAXVF_VAT);
		if(withOrWithout)
			c += vect.GetValue(val_fl);
		else
			c = vect.GetValue(val_fl);
	}
	*pCost = c;
	ASSIGN_PTR(pVatSum, vat_sum);
	return 1;
}

int SLAPI PPObjGoods::Helper_SearchMaxLikeByBarcode(const char * pCode, PPID * pID)
{
	int    ok = -1;
	BarcodeTbl::Rec bc_rec;
	int16  min_uniq_bc_len = (Cfg.MinUniqBcLen > 0) ? Cfg.MinUniqBcLen : 7;
	size_t len = strlen(pCode);
	const  int wp = Cfg.IsWghtPrefix(pCode);
	if(wp == 1)
		len -= strlen(Cfg.WghtPrefix);
	else if(wp == 2)
		len -= strlen(Cfg.WghtCntPrefix);
	if(len >= (size_t)min_uniq_bc_len && SearchByBarcode(pCode, &bc_rec, 0, 0) > 0) {
		ASSIGN_PTR(pID, bc_rec.GoodsID);
		ok = 1;
	}
	return ok;
}

int SLAPI PPObjGoods::SearchMaxLike(PPGoodsPacket * pPack, PPID * pID)
{
	int    ok = -1;
	if(SearchByName(pPack->Rec.Name, pID) > 0)
		ok = 1;
	else {
		GetConfig();
		BarcodeTbl::Rec * p_bc_rec;
		uint   pref_pos = 0;
		BarcodeTbl::Rec * p_pref_bc = pPack->Codes.GetPreferredItem(&pref_pos);
		if(p_pref_bc && Helper_SearchMaxLikeByBarcode(p_pref_bc->Code, pID) > 0) {
			ok = 1;
		}
		else {
			if(p_pref_bc)
				pref_pos++;
			else
				pref_pos = 0;
			for(uint i = 0; ok < 0 && pPack->Codes.enumItems(&i, (void**)&p_bc_rec);) {
				if(i != pref_pos && Helper_SearchMaxLikeByBarcode(p_bc_rec->Code, pID) > 0)
					ok = 1;
			}
		}
	}
	return ok;
}
//
//
//
GoodsSubstItem::GoodsSubstItem()
{
	THISZERO();
}

IMPL_CMPFUNC(GoodsSubstList_AssocItem, p1, p2)
{
	int    si = 0;
	CMPCASCADE4(si, (const GoodsSubstList::AssocItem *)p1, (const GoodsSubstList::AssocItem *)p2, SrcGoodsID, LocID, LotID, Dt);
	return si;
}

SLAPI GoodsSubstList::InnerAssocItem::InnerAssocItem(PPID substID)
{
	SubstID = substID;
}

int SLAPI GoodsSubstList::InnerAssocItem::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW_SL(pSCtx->Serialize(dir, SubstID, rBuf));
	THROW_SL(pSCtx->Serialize(dir, &List, rBuf));
	CATCHZOK
	return ok;
}

uint SLAPI GoodsSubstList::InnerAssocItem::SearchAssoc(const GoodsSubstList::AssocItem & rPattern) const
{
	uint   pos = 0;
	return List.lsearch(&rPattern, &pos, PTR_CMPFUNC(GoodsSubstList_AssocItem)) ? (pos+1) : 0;
}

SLAPI GoodsSubstList::AssocCollection::AssocCollection() : TSCollection <InnerAssocItem> ()
{
}

int SLAPI GoodsSubstList::AssocCollection::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	InnerAssocItem * p_new_item = 0;
	SString temp_buf;
	if(dir > 0) {
		uint32 c = getCount();
		THROW(rBuf.Write(&c, sizeof(c)));
		for(uint i = 0; i < c; i++) {
			InnerAssocItem * p_item = at(i);
			THROW(p_item->Serialize(dir, rBuf, pSCtx));
		}
	}
	else if(dir < 0) {
		freeAll();
		uint32 c = 0;
		THROW(rBuf.ReadV(&c, sizeof(c)));
		for(uint i = 0; i < c; i++) {
			THROW_MEM(p_new_item = new InnerAssocItem(0));
			THROW(p_new_item->Serialize(dir, rBuf, pSCtx));
			THROW_SL(insert(p_new_item));
			p_new_item = 0; // Обязательно обнулить - в случае сбоя объект по указателю будет удален (см. ниже)
		}
	}
	CATCHZOK
	delete p_new_item;
	return ok;
}

SLAPI GoodsSubstList::GoodsSubstList(int saveAssoc)
{
	P_Arp = 0;
	P_Goa = 0;
	SaveAssoc = saveAssoc;
	StrPool.add("$", 0); // zero index - is empty string
}

SLAPI GoodsSubstList::~GoodsSubstList()
{
	delete P_Arp;
	delete P_Goa;
}

int SLAPI GoodsSubstList::Init(int saveAssoc, const GoodsSubstList * pSrc)
{
	SaveAssoc = saveAssoc;
	Clear();
	if(pSrc)
		List = pSrc->List;
	return 1;
}

int SLAPI GoodsSubstList::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW_SL(pCtx->Serialize(dir, SaveAssoc, rBuf));
	THROW_SL(pCtx->Serialize(dir, &List, rBuf));
	THROW(AsscList.Serialize(dir, rBuf, pCtx));
	THROW(StrPool.Serialize(dir, rBuf, pCtx)); // @v7.5.4
	CATCHZOK
	return ok;
}

IMPL_CMPFUNC(GoodsSubstItem_Srch, p1, p2)
{
	const GoodsSubstItem * i1 = (const GoodsSubstItem *)p1;
	const GoodsSubstItem * i2 = (const GoodsSubstItem *)p2;
	if(i1->ClsID < i2->ClsID) return -1;
	if(i1->ClsID > i2->ClsID) return 1;
	if(i1->Par1 < i2->Par1)   return -1;
	if(i1->Par1 > i2->Par1)   return 1;
	if(i1->U.L.Par2 < i2->U.L.Par2)   return -1;
	if(i1->U.L.Par2 > i2->U.L.Par2)   return 1;
	if(i1->U.L.Par3 < i2->U.L.Par3)   return -1;
	if(i1->U.L.Par3 > i2->U.L.Par3)   return 1;
	{
		if(i1->P_Text != 0 && i2->P_Text != 0) {
			GoodsSubstList * p_gsl = (GoodsSubstList *)pExtraData;
			const char * p_text1 = p_gsl->StrPool.getBuf() + i1->StrP;
			return strcmp(p_text1, i2->P_Text);
		}
		else if(i1->P_Text == 0 && i2->P_Text)
			return -1;
		else if(i1->P_Text && i2->P_Text == 0)
			return +1;
		else
			return 0;
	}
}

int SLAPI GoodsSubstList::AddItem(const GoodsSubstItem & rItem, const AssocItem & rAssocItem, PPID * pID)
{
	int    ok = 1;
	PPID   id = 0;
	uint   pos = 0;
	if(List.lsearch(&rItem, &pos, PTR_CMPFUNC(GoodsSubstItem_Srch), 0, this)) {
		id = List.at(pos).ID;
	}
	else {
		GoodsSubstItem item = rItem;
		id = item.ID = ((List.getCount() + 1) | GOODSSUBSTMASK);
		if(item.P_Text) {
			uint text_pos = 0;
			StrPool.add(item.P_Text, &text_pos);
			item.StrP = text_pos;
		}
		else
			item.StrP = 0;
		THROW_SL(List.insert(&item));
	}
	THROW(AddToAssoc(*pID, rAssocItem));
	ASSIGN_PTR(pID, id);
	CATCHZOK
	return ok;
}

int SLAPI GoodsSubstList::GetItem(PPID id, GoodsSubstItem * pItem) const
{
	uint   pos = 0;
	if(List.lsearch(&id, &pos, CMPF_LONG)) {
		if(pItem) {
			*pItem = List.at(pos);
			if(pItem->StrP)
				pItem->P_Text = StrPool.getBuf() + pItem->StrP;
			else
				pItem->P_Text = 0;
		}
		return 1;
	}
	return -1;
}

int SLAPI GoodsSubstList::Clear()
{
	List.freeAll();
	AsscList.freeAll();
	StrPool.clear();
	StrPool.add("$", 0); // zero index - is empty string
	return 1;
}

int SLAPI GoodsSubstList::AddToAssoc(PPID substID, const AssocItem & rAssocItem)
{
	int    ok = -1;
	if(SaveAssoc) {
		uint   pos = 0;
		if(!AsscList.lsearch(&substID, &pos, CMPF_LONG)) {
			InnerAssocItem * p_new_item = new InnerAssocItem(substID);
			THROW_MEM(p_new_item);
			THROW_SL(AsscList.insert(p_new_item));
			pos = AsscList.getCount()-1;
		}
		InnerAssocItem * p_item = AsscList.at(pos);
		if(!p_item->SearchAssoc(rAssocItem)) {
			THROW_SL(p_item->List.insert(&rAssocItem));
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}

long SLAPI GoodsSubstList::GetAssocCount(PPID substID) const
{
	uint   pos = 0;
	return AsscList.lsearch(&substID, &pos, CMPF_LONG) ? AsscList.at(pos)->List.getCount() : 0;
}

int SLAPI GoodsSubstList::GetSubstList(PPIDArray & rList) const
{
	int    ok = -1;
	rList.clear();
	for(uint i = 0; i < AsscList.getCount(); i++) {
		rList.add(AsscList.at(i)->SubstID);
		ok = 1;
	}
	rList.sortAndUndup();
	return ok;
}

int SLAPI GoodsSubstList::GetGoodsBySubstID(PPID substID, PPIDArray * pList) const
{
	int    ok = -1;
	if(pList) {
		pList->clear();
		uint   pos = 0;
		if(AsscList.lsearch(&substID, &pos, CMPF_LONG)) {
			const InnerAssocItem * p_item = AsscList.at(pos);
            for(uint i = 0; i < p_item->List.getCount(); i++) {
            	const PPID src_goods_id = p_item->List.at(i).SrcGoodsID;
				pList->addnz(src_goods_id);
            }
            if(pList->getCount()) {
				pList->sortAndUndup();
				ok = 1;
            }
		}
	}
	return ok;
}

int SLAPI GoodsSubstList::GetGoodsBySubstID(PPID substID, ObjIdListFilt * pGoodsList) const
{
	int    ok = -1;
	if(pGoodsList) {
		PPIDArray temp_list;
		ok = GetGoodsBySubstID(substID, &temp_list) ? !!pGoodsList->Set(&temp_list) : 0;
	}
	return ok;
}

PPID SLAPI GoodsSubstList::SubstAlcoCategory(PPID goodsID)
{
	long   arp_pos = 0;
	if(P_Arp == 0) {
		P_Arp = new PrcssrAlcReport;
		if(P_Arp) {
			P_Arp->Init();
			P_Arp->SetConfig(0);
		}
	}
	if(P_Arp) {
		PrcssrAlcReport::GoodsItem arp_item;
		P_Arp->PreprocessGoodsItem(goodsID, 0, 0, 0, arp_item);
		arp_pos = (long)arp_item.CategoryCodePos;
	}
	return (arp_pos | GOODSSUBSTMASK);
}

int SLAPI GoodsSubstList::GetAlcoCategoryText(PPID substID, SString & rBuf) const
{
	rBuf = 0;
	int    ok = 0;
	long   arp_pos = (substID & ~GOODSSUBSTMASK);
	if(arp_pos && P_Arp) {
		ok = P_Arp->GetCategoryNameByCodePos(arp_pos, rBuf);
		rBuf.ToOem();
	}
	return ok;
}

PPID SLAPI GoodsSubstList::SubstLocAssoc(PPID goodsID)
{
	PPID   assoc_id = 0;
	if(P_Goa == 0) {
		P_Goa = new GoodsToObjAssoc(PPASS_GOODS2WAREPLACE, PPOBJ_LOCATION);
		if(P_Goa)
			P_Goa->Load();
	}
	if(P_Goa) {
		P_Goa->Get(goodsID, &assoc_id);
	}
	return (assoc_id | GOODSSUBSTMASK);
}

int SLAPI GoodsSubstList::GetLocAssocText(PPID substID, SString & rBuf) const
{
	rBuf = 0;
	int    ok = 0;
	long   assoc_id = (substID & ~GOODSSUBSTMASK);
	if(assoc_id) {
        if(GetLocationName(assoc_id, rBuf) > 0)
			ok = 1;
	}
	return ok;
}

int SLAPI PPObjGoods::SubstGoods(PPID srcID, PPID * pDestID, SubstGrpGoods sgg, SubstBlock * pBlk, GoodsSubstList * pGsl)
{
	int    ok = 1;
	PPID   goods_id = srcID;
	PPID   gen_goods_id = 0;
	Goods2Tbl::Rec goods_rec;
	ReceiptTbl::Rec lot_rec;
	if(sgg == sggNone)
		ok = -1;
	else {
		PPObjBill * p_bobj = BillObj;
		GoodsSubstList::AssocItem assoc_item;
		MEMSZERO(assoc_item);
		assoc_item.SrcGoodsID = srcID;
		/*
		if(pBlk) {
			assoc_item.Dt = pBlk->Dt;
			assoc_item.LocID = pBlk->LocID;
			assoc_item.LotID = pBlk->P_LotRec ? pBlk->P_LotRec->ID : pBlk->LotID;
		}
		*/
		if(oneof2(sgg, sggSuppl, sggSupplAgent)) {
			if(pBlk) {
				PPID _s_id = 0;
				if(pBlk->P_LotRec) {
					if(sgg == sggSupplAgent)
						_s_id = p_bobj->GetSupplAgent(pBlk->P_LotRec->ID);
					else
						_s_id = pBlk->P_LotRec->SupplID;
					assoc_item.LotID = pBlk->P_LotRec->ID;
				}
				else if(pBlk->LotID) {
					if(sgg == sggSupplAgent)
						_s_id = p_bobj->GetSupplAgent(pBlk->LotID);
					else if(p_bobj->trfr->Rcpt.Search(pBlk->LotID, &lot_rec) > 0)
						_s_id = lot_rec.SupplID;
					assoc_item.LotID = pBlk->LotID;
				}
				goods_id = _s_id | GOODSSUBSTMASK;
			}
			else
				goods_id = GOODSSUBSTMASK;
		}
		else if(sgg == sggLocation) {
			if(pBlk) {
				goods_id = (pBlk->LocID | GOODSSUBSTMASK);
				assoc_item.LocID = pBlk->LocID;
			}
			else
				goods_id = GOODSSUBSTMASK;
		}
		// @v8.4.4 {
		else if(sgg == sggAlcoCategory) {
			goods_id = pGsl ? pGsl->SubstAlcoCategory(srcID) : GOODSSUBSTMASK;
		}
		// } @v8.4.4
		// @v8.6.1 {
		else if(sgg == sggLocAssoc) {
			goods_id = pGsl ? pGsl->SubstLocAssoc(srcID) : GOODSSUBSTMASK;
		}
		// } @v8.6.1
		else if(Fetch(srcID, &goods_rec) <= 0)
			ok = -1;
		else if(sgg == sggGeneric) {
			if(BelongToGen(goods_id, &gen_goods_id) > 0)
				goods_id = gen_goods_id;
		}
		else if(sgg > sggTagBias) {
			goods_id = 0;
			const PPID tag_id = (sgg - sggTagBias);
			ObjTagItem tag_item;
			if(PPRef->Ot.GetTag(PPOBJ_GOODS, goods_rec.ID, tag_id, &tag_item) > 0) {
				GoodsSubstItem gsitem;
				switch(tag_item.TagDataType) {
					case OTTYP_BOOL:
						gsitem.Par1 = tag_item.Val.IntVal;
						break;
					case OTTYP_NUMBER:
						gsitem.U.RVal = tag_item.Val.RealVal;
						break;
					case OTTYP_ENUM:
						gsitem.Par1 = tag_item.Val.IntVal;
						break;
					case OTTYP_INT:
						gsitem.Par1 = tag_item.Val.IntVal;
						break;
					case OTTYP_OBJLINK:
						gsitem.Par1 = tag_item.Val.IntVal;
						break;
					case OTTYP_DATE:
						gsitem.Par1 = tag_item.Val.IntVal;
						break;
					case OTTYP_TIMESTAMP:
						gsitem.U.RVal = tag_item.Val.RealVal;
						break;
					case OTTYP_GUID:
					case OTTYP_IMAGE:
					case OTTYP_STRING:
						break;
					default:
						ok = -1;
						break;
				}
				if(ok > 0) {
					SString temp_buf;
					tag_item.GetStr(temp_buf);
					gsitem.P_Text = temp_buf;
					if(!pGsl->AddItem(gsitem, assoc_item, &goods_id))
						ok = 0;
				}
			}
		}
		else if(sgg == sggGroup || sgg >= sggGroupSecondLvl) {
			goods_id = goods_rec.ParentID;
			if(pBlk && pBlk->ExclParentID && CheckFlag(pBlk->ExclParentID, GF_EXCLALTFOLD))
				P_Tbl->GetExclusiveAltParent(srcID, pBlk->ExclParentID, &goods_id);
			else if(sgg >= sggGroupSecondLvl) {
				PPObjGoodsGroup ggrp_obj;
				long level = 1;
				Goods2Tbl::Rec temp_grec;
				temp_grec = goods_rec;
				ggrp_obj.GetLevel(goods_rec.ParentID, &level);
				level = MIN(level, sgg - sggGroupSecondLvl + 2);
				for(long idx = 1; idx < level && ggrp_obj.Fetch(temp_grec.ParentID, &temp_grec) > 0; idx++)
					goods_id = temp_grec.ParentID;
			}
		}
		else if(sgg == sggBrand)
			goods_id = goods_rec.BrandID;
		else if(sgg == sggBrandOwner) {
			if(goods_rec.BrandID) {
				PPBrand brand_rec;
				PPObjBrand brand_obj;
				if(brand_obj.Fetch(goods_rec.BrandID, &brand_rec) > 0)
					goods_id = (brand_rec.OwnerID | GOODSSUBSTMASK);
			}
		}
		else if(sgg == sggManuf)
			goods_id = (goods_rec.ManufID | GOODSSUBSTMASK);
		else if(sgg == sggVatRate) {
			PPGoodsTaxEntry gtx;
			const LDATE actual_date = (pBlk && pBlk->Dt) ? pBlk->Dt : getcurdate_();
			const PPID  op_id = 0;
			if(GTxObj.Fetch(goods_rec.TaxGrpID, actual_date, op_id, &gtx) > 0) {
				goods_id = (gtx.VAT | GOODSSUBSTMASK);
			}
			else
				goods_id = GOODSSUBSTMASK;
			assoc_item.Dt = actual_date;
		}
		else if(sgg == sggCVatRate) {
			PPGoodsTaxEntry gtx;
			PPID  lot_tax_grp_id = 0;
			LDATE actual_date = ZERODATE;
			int   vat_free = 0;
			if(pBlk) {
				if(pBlk->P_LotRec) {
					vat_free = IsLotVATFree(*pBlk->P_LotRec);
					if(!vat_free) {
						lot_tax_grp_id = pBlk->P_LotRec->InTaxGrpID;
						p_bobj->trfr->Rcpt.GetOriginDate(pBlk->P_LotRec, &actual_date);
					}
					assoc_item.LotID = pBlk->P_LotRec->ID;
				}
				else if(pBlk->LotID && p_bobj->trfr->Rcpt.Search(pBlk->LotID, &lot_rec) > 0) {
					vat_free = IsLotVATFree(lot_rec);
					if(!vat_free) {
						lot_tax_grp_id = lot_rec.InTaxGrpID;
						p_bobj->trfr->Rcpt.GetOriginDate(&lot_rec, &actual_date);
					}
					assoc_item.LotID = pBlk->LotID;
				}
				SETIFZ(actual_date, pBlk->Dt);
			}
			assoc_item.Dt = actual_date;
			if(vat_free) {
				goods_id = (1 | GOODSSUBSTMASK);
			}
			else {
				SETIFZ(actual_date, getcurdate_());
				PPID  tax_grp_id = NZOR(lot_tax_grp_id, goods_rec.TaxGrpID);
				const PPID  op_id = 0;
				if(GTxObj.Fetch(tax_grp_id, actual_date, op_id, &gtx) > 0)
					goods_id = (gtx.VAT | GOODSSUBSTMASK);
				else
					goods_id = GOODSSUBSTMASK;
			}
		}
		else if(IS_SGG_CLSSUBST(sgg)) {
			GoodsExtTbl::Rec gext_rec;
			GoodsSubstItem gsitem;
			if(P_Tbl->GetExt(goods_id, &gext_rec) > 0) {
				gsitem.ClsID = gext_rec.GoodsClsID;
				if(sgg == sggDimX)
					gsitem.Par1 = gext_rec.X;
				else if(sgg == sggDimY)
					gsitem.Par1 = gext_rec.Y;
				else if(sgg == sggDimZ)
					gsitem.Par1 = gext_rec.Z;
				else if(sgg == sggDimW) // @v8.6.1
					gsitem.Par1 = gext_rec.W;
				else if(sgg == sggClsKind)
					gsitem.Par1 = gext_rec.KindID;
				else if(sgg == sggClsGrade)
					gsitem.Par1 = gext_rec.GradeID;
				else if(sgg == sggClsAddObj)
					gsitem.Par1 = gext_rec.AddObjID;
				else if(sgg == sggClsKind_Grade) {
					gsitem.Par1 = gext_rec.KindID;
					gsitem.U.L.Par2 = gext_rec.GradeID;
				}
				// AHTOXA {
				else if(sgg == sggClsKind_Grade_AddObj) {
					gsitem.Par1 = gext_rec.KindID;
					gsitem.U.L.Par2 = gext_rec.GradeID;
					gsitem.U.L.Par3 = gext_rec.AddObjID;
				}
				else if(sgg == sggClsKind_AddObj_Grade) {
					gsitem.Par1 = gext_rec.KindID;
					gsitem.U.L.Par2 = gext_rec.AddObjID;
					gsitem.U.L.Par3 = gext_rec.GradeID;
				}
				// } AHTOXA
				else
					ok = -1;
				if(ok > 0)
					if(!pGsl->AddItem(gsitem, assoc_item, &goods_id))
						ok = 0;
			}
		}
		if(ok > 0 && pGsl && srcID != goods_id)
			ok = pGsl->AddToAssoc(goods_id, assoc_item);
	}
	ASSIGN_PTR(pDestID, goods_id);
	return ok;
}

int SLAPI PPObjGoods::GetSubstText(PPID id, SubstGrpGoods sgg, const GoodsSubstList * pGsl, SString & rBuf)
{
	rBuf = 0;
	if(sgg > sggTagBias) {
		GoodsSubstItem gsitem;
		if(pGsl && pGsl->GetItem(id, &gsitem) > 0) {
			rBuf = gsitem.P_Text;
		}
	}
	else if(oneof2(sgg, sggVatRate, sggCVatRate)) {
		const  long v = (id & ~GOODSSUBSTMASK);
		if(v == 0) {
			PPLoadString("novat", rBuf);
		}
		else if(v == 1) {
			PPLoadString("vatexempt", rBuf);
		}
		else {
			long   fmt = MKSFMTD(0, 2, NMBF_NOTRAILZ | ALIGN_LEFT | NMBF_NOZERO);
			rBuf.Cat(fdiv100i(v), fmt);
		}
	}
	// @v8.4.4 {
	else if(sgg == sggAlcoCategory) {
		if(pGsl)
			pGsl->GetAlcoCategoryText(id, rBuf);
	}
	// } @v8.4.4
	// @v8.6.1 {
	else if(sgg == sggLocAssoc) {
		if(pGsl)
			pGsl->GetLocAssocText(id, rBuf);
	}
	// } @v8.6.1
	else if(oneof4(sgg, sggNone, sggGeneric, sggGroup, sggBrand) || sgg >= sggGroupSecondLvl || !(id & GOODSSUBSTMASK)) {
		GetGoodsName(id, rBuf);
	}
	else if(sgg == sggBrandOwner) {
		GetPersonName(id & ~GOODSSUBSTMASK, rBuf);
	}
	else if(sgg == sggManuf) {
		GetPersonName(id & ~GOODSSUBSTMASK, rBuf);
	}
	else if(sgg == sggSuppl || sgg == sggSupplAgent) {
		GetArticleName(id & ~GOODSSUBSTMASK, rBuf);
	}
	else if(sgg == sggLocation) {
		GetLocationName(id & ~GOODSSUBSTMASK, rBuf);
	}
	else if(IS_SGG_CLSSUBST(sgg)) {
		GoodsSubstItem gsitem;
		if(pGsl && pGsl->GetItem(id, &gsitem) > 0) {
			PPObjGoodsClass gc_obj;
			PPGdsClsPacket gc_pack;
			if(gc_obj.Fetch(gsitem.ClsID, &gc_pack) > 0) {
				double val = 0.0;
				GoodsExtTbl::Rec gext_rec;
				MEMSZERO(gext_rec);
				gext_rec.GoodsClsID = gsitem.ClsID;
				if(sgg == sggDimX) {
					gext_rec.X = gsitem.Par1;
					gc_pack.GetExtDim(&gext_rec, PPGdsCls::eX, &val);
					rBuf.Cat(val, MKSFMTD(0, 6, NMBF_NOTRAILZ));
				}
				else if(sgg == sggDimY) {
					gext_rec.Y = gsitem.Par1;
					gc_pack.GetExtDim(&gext_rec, PPGdsCls::eY, &val);
					rBuf.Cat(val, MKSFMTD(0, 6, NMBF_NOTRAILZ));
				}
				else if(sgg == sggDimZ) {
					gext_rec.Z = gsitem.Par1;
					gc_pack.GetExtDim(&gext_rec, PPGdsCls::eZ, &val);
					rBuf.Cat(val, MKSFMTD(0, 6, NMBF_NOTRAILZ));
				}
				else if(sgg == sggDimW) { // @v8.6.1
					gext_rec.W = gsitem.Par1;
					gc_pack.GetExtDim(&gext_rec, PPGdsCls::eW, &val);
					rBuf.Cat(val, MKSFMTD(0, 6, NMBF_NOTRAILZ));
				}
				else if(sgg == sggClsKind) {
					gext_rec.KindID = gsitem.Par1;
					gc_pack.GetExtProp(&gext_rec, PPGdsCls::eKind, 0, rBuf);
					if(rBuf.Empty()) {
						rBuf = gc_pack.Rec.Name;
						rBuf.Space().CatChar('#').Cat(gsitem.Par1);
					}
				}
				else if(sgg == sggClsGrade) {
					gext_rec.GradeID = gsitem.Par1;
					gc_pack.GetExtProp(&gext_rec, PPGdsCls::eGrade, 0, rBuf);
					if(rBuf.Empty()) {
						rBuf = gc_pack.Rec.Name;
						rBuf.Space().CatChar('#').Cat(gsitem.Par1);
					}
				}
				else if(sgg == sggClsAddObj) {
					gext_rec.AddObjID = gsitem.Par1;
					gc_pack.GetExtProp(&gext_rec, PPGdsCls::eAdd, 0, rBuf);
					if(rBuf.Empty()) {
						rBuf = gc_pack.Rec.Name;
						rBuf.Space().CatChar('#').Cat(gsitem.Par1);
					}
				}
				else if(sgg == sggClsKind_Grade) {
					SString temp_buf2;
					gext_rec.KindID  = gsitem.Par1;
					gext_rec.GradeID = gsitem.U.L.Par2;
					gc_pack.GetExtProp(&gext_rec, PPGdsCls::eKind,  0, rBuf);
					rBuf.Space();
					gc_pack.GetExtProp(&gext_rec, PPGdsCls::eGrade, 0, temp_buf2);
					rBuf.Cat(temp_buf2);
					if(rBuf.Strip().Empty()) {
						rBuf = gc_pack.Rec.Name;
						rBuf.Space().CatChar('#').Cat(gsitem.Par1).Space().CatChar('#').Cat(gsitem.U.L.Par2);
					}
				}
				else if(oneof2(sgg, sggClsKind_Grade_AddObj, sggClsKind_AddObj_Grade)) {
					SString temp_buf2, grade_buf, add_buf;
					gext_rec.KindID   = gsitem.Par1;
					gext_rec.GradeID  = (sgg == sggClsKind_Grade_AddObj) ? gsitem.U.L.Par2 : gsitem.U.L.Par3;
					gext_rec.AddObjID = (sgg == sggClsKind_Grade_AddObj) ? gsitem.U.L.Par3 : gsitem.U.L.Par2;
					gc_pack.GetExtProp(&gext_rec, PPGdsCls::eKind, 0, rBuf);
					rBuf.Space();
					gc_pack.GetExtProp(&gext_rec, PPGdsCls::eGrade, 0, grade_buf);
					gc_pack.GetExtProp(&gext_rec, PPGdsCls::eAdd, 0, add_buf);
					if(sgg == sggClsKind_Grade_AddObj)
						rBuf.Cat(grade_buf).Space().Cat(add_buf);
					else
						rBuf.Cat(add_buf).Space().Cat(grade_buf);
					if(rBuf.Strip().Empty()) {
						rBuf = gc_pack.Rec.Name;
						rBuf.Space().CatChar('#').Cat(gsitem.U.L.Par2).Space().CatChar('#').Cat(gsitem.U.L.Par3);
					}
				}
			}
		}
	}
	return 1;
}

int SLAPI PPObjGoods::GetParentID(PPID id, PPID * pParentID)
{
	Goods2Tbl::Rec goods_rec;
	if(id > 0 && Search(id, &goods_rec) > 0) {
		ASSIGN_PTR(pParentID, goods_rec.ParentID);
		return 1;
	}
	else {
		ASSIGN_PTR(pParentID, 0);
		return -1;
	}
}

int SLAPI PPObjGoods::GetPhUPerU(const Goods2Tbl::Rec * pRec, PPID * pPhUnitID, double * pPhUPerU)
{
	int    ok = -1;
	PPID   ph_unit_id = 0;
	double phuperu = 0;
	PPUnit unit_rec;
	if(pRec)
		if(pRec->PhUnitID && pRec->PhUPerU > 0) {
			ph_unit_id = pRec->PhUnitID;
			phuperu    = pRec->PhUPerU;
			ok = 1;
		}
		else if(FetchUnit(pRec->UnitID, &unit_rec) > 0) {
			ph_unit_id = pRec->UnitID;
			phuperu = 1;
			ok = 1;
		}
	ASSIGN_PTR(pPhUnitID, ph_unit_id);
	ASSIGN_PTR(pPhUPerU, phuperu);
	return ok;
}

int SLAPI PPObjGoods::GetPhUPerU(PPID goodsID, PPID * pPhUnitID, double * pPhUPerU)
{
	Goods2Tbl::Rec goods_rec;
	return (goodsID && Fetch(goodsID, &goods_rec) > 0) ?
		GetPhUPerU(&goods_rec, pPhUnitID, pPhUPerU) : GetPhUPerU((Goods2Tbl::Rec *)0, pPhUnitID, pPhUPerU);
}

int SLAPI PPObjGoods::ValidateGoodsParent(PPID parentID)
{
	int    ok = 1;
	Goods2Tbl::Rec goods_rec;
	if(parentID && Fetch(parentID, &goods_rec) > 0) {
		if(goods_rec.Kind != PPGDSK_GROUP || (goods_rec.Flags & (GF_FOLDER|GF_ALTGROUP)))
			ok = 0;
	}
	else
		ok = -1;
	return ok;
}

int SLAPI PPObjGoods::Fetch(PPID id, Goods2Tbl::Rec * rec)
	{ return P_Tbl->Fetch(id, rec); }
int SLAPI PPObjGoods::SearchByName(const char * pName, PPID * pID, Goods2Tbl::Rec * pRec)
	{ return P_Tbl->SearchByName(Kind, pName, pID, pRec); }
int SLAPI PPObjGoods::BelongToGroup(PPID id, PPID grp, PPID * pSubGrpID)
	{ return P_Tbl->BelongToGroup(id, grp, pSubGrpID); }
int SLAPI PPObjGoods::IsAltGroup(PPID grpID)
	{ return P_Tbl->IsAltGroup(grpID); }
int SLAPI PPObjGoods::IsTempAltGroup(PPID grpID)
	{ return P_Tbl->IsTempAltGroup(grpID); }
int SLAPI PPObjGoods::IsDynamicAltGroup(PPID grpID)
	{ return P_Tbl->IsDynamicAltGroup(grpID); }
int SLAPI PPObjGoods::IsGoodsCompatibleByUnit(PPID id1, PPID id2, double * ratio)
	{ return P_Tbl->IsCompatibleByUnit(id1, id2, ratio); }
int SLAPI PPObjGoods::GetGenericList(PPID genID, PPIDArray * pList)
	{ return P_Tbl->GetGenericList(genID, pList); }
int SLAPI PPObjGoods::BelongToGen(PPID goodsID, PPID * gen, ObjAssocTbl::Rec * b)
	{ return P_Tbl->BelongToGen(goodsID, gen, b); }
int SLAPI PPObjGoods::AssignGoodsToGen(PPID goodsID, PPID genID, int abbr, int use_ta)
	{ return P_Tbl->AssignGoodsToGen(goodsID, genID, abbr, use_ta); }
int SLAPI PPObjGoods::RemoveGoodsFromGen(PPID goodsID, PPID genID, int use_ta)
	{ return P_Tbl->RemoveGoodsFromGen(goodsID, genID, use_ta); }
int SLAPI PPObjGoods::AssignGoodsToAltGrp(PPID goodsID, PPID grp, long innerNum, int use_ta)
	{ return P_Tbl->AssignGoodsToAltGrp(goodsID, grp, innerNum, use_ta); }
int SLAPI PPObjGoods::ReadBarcodes(PPID id, BarcodeArray & rAry)
	{ return P_Tbl->ReadBarcodes(id, rAry); }
int SLAPI PPObjGoods::GetSingleBarcode(PPID id, SString & rBuf)
	{ return P_Tbl->GetSingleBarcode(id, rBuf); }
int SLAPI PPObjGoods::FetchSingleBarcode(PPID id, SString & rBuf)
	{ return P_Tbl->FetchSingleBarcode(id, rBuf); }
int SLAPI PPObjGoods::GetSingleBarcode(PPID id, char * pCode, size_t bufLen) // @obsolete
{
	SString temp_buf;
	int ok = P_Tbl->GetSingleBarcode(id, temp_buf);
	temp_buf.CopyTo(pCode, bufLen);
	return ok;
}

int SLAPI PPObjGoods::GetBarcodeByTemplate(PPID grp, const char * pTempl, char * pBuf)
{
	return P_Tbl->GetBarcodeByTemplate(grp, GetConfig()/*GetConfig().WghtPrefix*/, pTempl, pBuf);
}

/* @v9.1.4 int SLAPI PPObjGoods::GetGoodsArticle(PPID id, PPID * pArticle)
{
	return tbl->GetGoodsArticle(id, pArticle);
} */

int SLAPI PPObjGoods::SearchByArticle(PPID article, BarcodeTbl::Rec * pRec)
{
	int      is_found = 0;
	SString  gds_code;
	gds_code.CatChar('$');
	BarcodeTbl::Rec  bc_rec;
	MEMSZERO(bc_rec);
	gds_code.Cat(article).CopyTo(bc_rec.Code, sizeof(bc_rec.Code));
	if(P_Tbl->SearchBarcode(bc_rec.Code, &bc_rec) > 0 && bc_rec.BarcodeType == -1)
		is_found = 1;
	ASSIGN_PTR(pRec, bc_rec);
	return is_found;
}

int SLAPI PPObjGoods::CheckFlag(PPID goodsID, long flag)
{
	Goods2Tbl::Rec goods_rec;
	return BIN(Fetch(labs(goodsID), &goods_rec) > 0 && (goods_rec.Flags & flag) == flag);
}

int SLAPI PPObjGoods::IsAsset(PPID goodsID)
	{ return CheckFlag(goodsID, GF_ASSETS); }
int SLAPI PPObjGoods::IsGeneric(PPID goodsID)
	{ return CheckFlag(goodsID, GF_GENERIC); }

int SLAPI PPObjGoods::IsAssetType(PPID goodsTypeID)
{
	PPGoodsType gt_rec;
	return BIN(goodsTypeID && GtObj.Fetch(goodsTypeID, &gt_rec) > 0 && gt_rec.Flags & GTF_ASSETS);
}

int SLAPI PPObjGoods::MultTaxFactor(PPID goodsID, double * pVal)
{
	if(CConfig.Flags & CCFLG_USEGDSCLS) {
		GoodsExtTbl::Rec gext_rec;
		if(CheckFlag(goodsID, GF_TAXFACTOR) && P_Tbl->GetExt(labs(goodsID), &gext_rec) > 0 && gext_rec.TaxFactor > 0) {
			*pVal *= gext_rec.TaxFactor;
			return 1;
		}
	}
	return -1;
}

int SLAPI PPObjGoods::Unite(PPID destID, PPID srcID)
{
	int    ok = 1;
	if(Obj == PPOBJ_GOODS) {
		if(GetConfig().Flags & GCF_ONUNITEMOVEBARCODE) {
			int    src_pack_inited = 0;
			int    turn_packets = 0;
			uint   i;
			PPGoodsPacket dest_pack, src_pack;
			THROW(GetPacket(destID, &dest_pack, 0) > 0);
			if(dest_pack.Codes.getCount()) {
				BarcodeTbl::Rec * p_bc_rec;
				THROW(GetPacket(srcID, &src_pack, 0) > 0);
				src_pack_inited = 1;
				for(i = 0; dest_pack.Codes.enumItems(&i, (void **)&p_bc_rec);) {
					BarcodeTbl::Rec bc_rec = *p_bc_rec;
					bc_rec.GoodsID = src_pack.Rec.ID;
					THROW_SL(src_pack.Codes.insert(&bc_rec));
				}
				dest_pack.Codes.freeAll();
				turn_packets = 1;
			}
			//
			//
			//
			if(dest_pack.ArCodes.getCount()) {
				ArGoodsCodeTbl::Rec * p_ac_rec;
				if(!src_pack_inited) {
					THROW(GetPacket(srcID, &src_pack, 0) > 0);
				}
				for(i = 0; dest_pack.ArCodes.enumItems(&i, (void **)&p_ac_rec);) {
					ArGoodsCodeTbl::Rec ac_rec = *p_ac_rec;
					ac_rec.GoodsID = src_pack.Rec.ID;
					THROW_SL(src_pack.ArCodes.insert(&ac_rec));
				}
				dest_pack.ArCodes.freeAll();
				turn_packets = 1;
			}
			if(turn_packets) {
				THROW(PutPacket(&destID, &dest_pack, 0));
				THROW(PutPacket(&srcID, &src_pack, 0));
			}
		}
		THROW(BroadcastObjMessage(DBMSG_OBJREPLACE, Obj, destID, (void *)srcID));
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjGoods::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
{
	int    ok = DBRPL_OK;
	Goods2Tbl::Rec rec;
	PPIDArray id_list;
	if(msg == DBMSG_OBJDELETE) {
		PPID   goods_id = 0;
		int    r = P_Tbl->SearchAnyRef(_obj, _id, &goods_id);
		THROW(r);
		if(r > 0) {
			PPID   obj_type = Obj;
			if(Search(goods_id) > 0) {
				if(P_Tbl->data.Kind == PPGDSK_GOODS)
					obj_type = PPOBJ_GOODS;
				else if(P_Tbl->data.Kind == PPGDSK_GROUP)
					obj_type = PPOBJ_GOODSGROUP;
			}
			ok = RetRefsExistsErr(obj_type, goods_id);
		}
		else
			ok = r ? DBRPL_OK : DBRPL_ERROR;
	}
	else if(msg == DBMSG_OBJREPLACE) {
		switch(_obj) {
			case PPOBJ_GOODS:
				ok = Unite(_id, (long)extraPtr) ? DBRPL_OK : DBRPL_ERROR;
				break;
			case PPOBJ_PERSON:
				{
					PPID   k = 0;
					BExtQuery q(P_Tbl, 0, 256);
					q.select(P_Tbl->ID, 0L).where(P_Tbl->ManufID == _id);
					for(q.initIteration(0, &k, spFirst); q.nextIteration() > 0;) {
						THROW_SL(id_list.add(P_Tbl->data.ID));
					}
					for(uint i = 0; i < id_list.getCount(); i++) {
						PPID   key_id = id_list.get(i);
						if(SearchByKey_ForUpdate(P_Tbl, 0, &key_id, &rec) > 0 && rec.ManufID == _id) {
							rec.ManufID = (long)extraPtr;
							THROW_DB(P_Tbl->updateRecBuf(&rec)); // @sfu
							Dirty(rec.ID);
						}
					}
				}
				break;
			case PPOBJ_GOODSTAX:
				{
					PPID   k = 0;
					BExtQuery q(P_Tbl, 0, 256);
					q.select(P_Tbl->ID, 0L).where(P_Tbl->TaxGrpID == _id);
					for(q.initIteration(0, &k, spFirst); q.nextIteration() > 0;) {
						THROW_SL(id_list.add(P_Tbl->data.ID));
					}
					for(uint i = 0; i < id_list.getCount(); i++) {
						PPID   key_id = id_list.get(i);
						if(SearchByKey_ForUpdate(P_Tbl, 0, &key_id, &rec) > 0 && rec.TaxGrpID == _id) {
							rec.TaxGrpID = (long)extraPtr;
							THROW_DB(P_Tbl->updateRecBuf(&rec)); // @sfu
							Dirty(rec.ID);
						}
					}
				}
				break;
			case PPOBJ_ARTICLE:
				ok = P_Tbl->ReplaceArticleRefs(_id, (long)extraPtr, 0) ? DBRPL_OK : DBRPL_ERROR;
				break;
			case PPOBJ_UNIT:
				{
					PPID   k = 0;
					BExtQuery q(P_Tbl, 0, 256);
					q.select(P_Tbl->ID, 0L).where(P_Tbl->UnitID == _id || P_Tbl->PhUnitID == _id);
					for(q.initIteration(0, &k, spFirst); q.nextIteration() > 0;) {
						THROW(id_list.add(P_Tbl->data.ID));
					}
					for(uint i = 0; i < id_list.getCount(); i++) {
						PPID   key_id = id_list.get(i);
						if(SearchByKey_ForUpdate(P_Tbl, 0, &key_id, &rec) > 0) {
							if(rec.UnitID == _id)
								rec.UnitID = (long)extraPtr;
							if(rec.PhUnitID == _id)
								rec.PhUnitID = (long)extraPtr;
							THROW_DB(P_Tbl->updateRecBuf(&rec)); // @sfu
							Dirty(rec.ID);
						}
					}
				}
				break;
			case PPOBJ_BRAND:
				{
					Goods2Tbl::Key3 k3;
					MEMSZERO(k3);
					k3.Kind = PPGDSK_GOODS;
					k3.BrandID = _id;
					BExtQuery q(P_Tbl, 3, 256);
					q.select(P_Tbl->ID, 0L).where(P_Tbl->Kind == PPGDSK_GOODS && P_Tbl->BrandID == _id);
					for(q.initIteration(0, &k3, spEq); q.nextIteration() > 0;) {
						THROW(id_list.add(P_Tbl->data.ID));
					}
					for(uint i = 0; i < id_list.getCount(); i++) {
						PPID   key_id = id_list.get(i);
						if(SearchByKey_ForUpdate(P_Tbl, 0, &key_id, &rec) > 0) {
							if(rec.BrandID == _id)
								rec.BrandID = (long)extraPtr;
							THROW_DB(P_Tbl->updateRecBuf(&rec)); // @sfu
							Dirty(rec.ID);
						}
					}
				}
				break;
		}
	}
	CATCH
		ok = DBRPL_ERROR;
	ENDCATCH
	return ok; // @v8.5.3 @fix DBRPL_OK-->ok
}

int SLAPI PPObjGoods::SupplyAddingToAltGrp(PPID id, int use_ta)
{
	int    ok = 1;
	int    all = BIN(GetConfig().Flags & GCF_ADDTOALTGRPWITHOUTMSG);
	if(!IsGeneric(id)) {
		TaggedStringArray grp_txt_list;
		PPIDArray grp_list;
		GoodsFilt grp_filt;
		P_Tbl->GetGroupTerminalList(0, &grp_list, 0);
		for(uint i = 0; i < grp_list.getCount(); i++) {
			Goods2Tbl::Rec grp_rec;
			const PPID grp_id = grp_list.get(i);
			if(Fetch(grp_id, &grp_rec) > 0 && grp_rec.Flags & GF_ALTGROUP && !(grp_rec.Flags & (GF_TEMPALTGRP_|GF_DYNAMIC))) {
				if(P_Tbl->GetGroupFilt(grp_id, &grp_filt) > 0) {
					if(!grp_filt.IsEmpty() && CheckForFilt(&grp_filt, id)) {
						TaggedString item;
						item.Id = grp_id;
						STRNSCPY(item.Txt, grp_rec.Name);
						grp_txt_list.insert(&item);
					}
				}
			}
		}
		if(grp_txt_list.getCount() > 0 && !all) {
			TaggedStringArray  sel_list;
			ListToListAryData  grp_data(0, &grp_txt_list, &sel_list);
			grp_data.TitleStrID = PPTXT_SELALTGOODSGRPS;
			if(ListToListAryDialog(&grp_data) > 0)
				grp_txt_list.copy(sel_list);
			else
				grp_txt_list.freeAll();
		}
		if(grp_txt_list.getCount()) {
			PPTransaction tra(use_ta);
			THROW(tra);
			for(uint i = 0; i < grp_txt_list.getCount(); i++) {
				THROW(AssignGoodsToAltGrp(id, grp_txt_list.at(i).Id, 0, 0));
			}
			THROW(tra.Commit());
		}
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPObjGoods::GetManufCountry(PPID goodsID, const Goods2Tbl::Rec * pRec, PPID * pCountryID, PPCountryBlock * pBlk)
{
	Goods2Tbl::Rec rec;
	CALLPTRMEMB(pBlk, Clear());
	if(pRec == 0)
		if(Fetch(goodsID, &rec) > 0)
			pRec = &rec;
		else
			return -1;
	SETIFZ(P_PsnObj, new PPObjPerson);
	return P_PsnObj ? P_PsnObj->GetCountry(pRec->ManufID, pCountryID, pBlk) : PPSetErrorNoMem();
}

int SLAPI PPObjGoods::CheckForFilt(const GoodsFilt * pFilt, PPID id, long implFlags)
{
	#define CFF_RET  return negation ? 1 : 0

	if(pFilt->IsEmpty())
		return 1;
	int    negation = BIN(pFilt->Flags & GoodsFilt::fNegation);
	Goods2Tbl::Rec rec;
	if(id) {
		if(Fetch(id, &rec) <= 0)
			return 0;
	}
	else
		P_Tbl->copyBufTo(&rec);
	id = rec.ID;
	if(pFilt->Flags & GoodsFilt::fExcludeAsset && IsAssetType(rec.GoodsTypeID) > 0)
		return 0;
	if(pFilt->Flags & GoodsFilt::fHidePassive && rec.Flags & GF_PASSIV)
		CFF_RET;
	if(pFilt->Flags & GoodsFilt::fPassiveOnly && !(rec.Flags & GF_PASSIV))
		CFF_RET;
	if(pFilt->Flags & GoodsFilt::fGenGoodsOnly && !(rec.Flags & GF_GENERIC))
		CFF_RET;
	if(pFilt->Flags & GoodsFilt::fWOTaxGdsOnly && !(rec.Flags & GF_PRICEWOTAXES))
		CFF_RET;
	if(pFilt->Flags & GoodsFilt::fNoDisOnly && !(rec.Flags & GF_NODISCOUNT))
		CFF_RET;
	if(pFilt->Flags & GoodsFilt::fHasImages && !(rec.Flags & GF_HASIMAGES))
		CFF_RET;
	// @v8.3.9 {
	if(pFilt->Flags & GoodsFilt::fUseIndepWtOnly && !(rec.Flags & GF_USEINDEPWT))
		CFF_RET;
	// } @v8.3.9
	if(pFilt->ManufID && pFilt->ManufID != rec.ManufID)
		CFF_RET;
	{
		const ObjIdListFilt & result_brand_list = pFilt->GetResultBrandList();
		if(result_brand_list.GetCount() && !result_brand_list.CheckID(rec.BrandID))
			CFF_RET;
	}
	if(pFilt->UnitID && pFilt->UnitID != rec.UnitID)
		CFF_RET;
	if(pFilt->PhUnitID && pFilt->PhUnitID != rec.PhUnitID)
		CFF_RET;
	if(pFilt->Flags & GoodsFilt::fUndefType) {
		if(rec.GoodsTypeID)
			CFF_RET;
	}
	else if(pFilt->GoodsTypeID && pFilt->GoodsTypeID != rec.GoodsTypeID)
		CFF_RET;
	if(pFilt->VatRate) {
		PPGoodsTaxEntry gtx;
		if(FetchTax(id, pFilt->VatDate, 0, &gtx) > 0) {
			if(pFilt->VatRate != gtx.VAT)
				CFF_RET;
		}
		else
			CFF_RET;
	}
	else if(pFilt->TaxGrpID && pFilt->TaxGrpID != rec.TaxGrpID)
		CFF_RET;
	else if(pFilt->Flags & GoodsFilt::fWoTaxGrp) {
		if(rec.TaxGrpID)
			CFF_RET;
		else {
			Goods2Tbl::Rec parent_rec;
			MEMSZERO(parent_rec);
			Fetch(rec.ParentID, &parent_rec);
			if(parent_rec.TaxGrpID)
				CFF_RET;
		}
	}
#if 0 // @v7.7.9 { Функционал блока теперь реализуется проверкой по pFilt->GetResultBrandList() выше
	if(pFilt->BrandOwnerID) {
		if(rec.BrandID) {
			PPObjBrand brand_obj;
			PPBrand brand_rec;
			if(brand_obj.Fetch(rec.BrandID, &brand_rec) > 0) {
				if(brand_rec.OwnerID != pFilt->BrandOwnerID)
					CFF_RET;
			}
			else
				CFF_RET;
		}
		else
			CFF_RET;
	}
#endif // } 0 // @v7.7.9
	if(pFilt->ManufCountryID) {
		if(rec.ManufID) {
			SETIFZ(P_PsnObj, new PPObjPerson);
			PersonTbl::Rec psn_rec;
			if(P_PsnObj && P_PsnObj->Fetch(rec.ManufID, &psn_rec) > 0) {
				PPID   cid = 0;
				P_PsnObj->GetCountry(rec.ManufID, &cid, 0);
				if(cid != pFilt->ManufCountryID)
					CFF_RET;
			}
			else
				CFF_RET;
		}
		else
			CFF_RET;
	}
	if(pFilt->Flags & (GoodsFilt::fIntUnitOnly | GoodsFilt::fFloatUnitOnly)) {
		PPObjUnit uobj;
		if(uobj.IsInteger(rec.UnitID)) {
			if(pFilt->Flags & GoodsFilt::fFloatUnitOnly)
				CFF_RET;
		}
		else if(pFilt->Flags & GoodsFilt::fIntUnitOnly)
			CFF_RET;
	}
	{
		SString temp_buf;
		pFilt->GetExtssData(pFilt->extssNameText, temp_buf);
		if(temp_buf.NotEmptyS() && !ExtStrSrch(rec.Name, temp_buf))
			CFF_RET;
	}
	if(!(implFlags & GOCFIF_ARCODE) && pFilt->IsRestrictedByAr()) {
		ArGoodsCodeArray arcode_list;
		P_Tbl->ReadArCodesByAr(id, pFilt->CodeArID, &arcode_list);
		if(!arcode_list.getCount())
			CFF_RET;
	}
	if(!(implFlags & GOCFIF_GROUP)) {
		if(pFilt->GrpIDList.GetCount()) {
			PPID * p_grp_id;
			uint found = 0;
			for(uint i = 0; !found && pFilt->GrpIDList.Get().enumItems(&i, (void**)&p_grp_id);)
				if(BelongToGroup(id, *p_grp_id, 0))
					found = 1;
			if(!found)
				CFF_RET;
		}
		else if(pFilt->GrpID && !BelongToGroup(id, pFilt->GrpID, 0))
			CFF_RET;
	}
	if(pFilt->Ep.GdsClsID && rec.GdsClsID == pFilt->Ep.GdsClsID) {
		if(pFilt->Ep.HasAttrRestrictions()) {
			PPObjGoodsClass gc_obj;
			PPGdsClsPacket gc_pack;
			GoodsExtTbl::Rec ext_rec;
			MEMSZERO(ext_rec);
			if(P_Tbl->GetExt(rec.ID, &ext_rec) > 0 && gc_obj.Fetch(ext_rec.GoodsClsID, &gc_pack) > 0) {
				if(!pFilt->Ep.KindList.CheckID(ext_rec.KindID))
					CFF_RET;
				if(!pFilt->Ep.GradeList.CheckID(ext_rec.GradeID))
					CFF_RET;
				if(!pFilt->Ep.AddObjList.CheckID(ext_rec.AddObjID))
					CFF_RET;
				if(!pFilt->Ep.AddObj2List.CheckID(ext_rec.AddObj2ID))
					CFF_RET;
				double dim_x = 0.0, dim_y = 0.0, dim_z = 0.0, dim_w = 0.0;
				gc_pack.GetExtDim(&ext_rec, PPGdsCls::eX, &dim_x);
				gc_pack.GetExtDim(&ext_rec, PPGdsCls::eY, &dim_y);
				gc_pack.GetExtDim(&ext_rec, PPGdsCls::eZ, &dim_z);
				gc_pack.GetExtDim(&ext_rec, PPGdsCls::eW, &dim_w);
				if(!pFilt->Ep.DimX_Rng.CheckVal(dim_x) || !pFilt->Ep.DimY_Rng.CheckVal(dim_y) ||
					!pFilt->Ep.DimZ_Rng.CheckVal(dim_z) || !pFilt->Ep.DimW_Rng.CheckVal(dim_w))
					CFF_RET;
			}
			else
				CFF_RET;
		}
	}
	else if(pFilt->Ep.GdsClsID)
		CFF_RET;
	if(pFilt->BarcodeLen.NotEmpty() && !(implFlags & GOCFIF_BCLEN)) {
		PPIDArray len_list;
		if(pFilt->GetBarcodeLenList(len_list) > 0) {
			int    ok = -1;
			BarcodeArray bc_list;
			ReadBarcodes(id, bc_list);
			if(bc_list.getCount()) {
				for(uint i = 0; ok < 0 && i < bc_list.getCount(); i++)
					if(len_list.bsearch(strlen(bc_list.at(i).Code)))
						ok = 1;
			}
			else if(len_list.bsearch(0))
				ok = 1;
			if(ok < 0)
				CFF_RET;
		}
	}
	if(!(implFlags & GOCFIF_GROUP)) {
		int    ok = 1;
		GoodsFilt grp_filt;
		if(pFilt->GrpIDList.GetCount()) {
			PPID * p_grp_id;
			for(uint i = 0; ok > 0 && pFilt->GrpIDList.Get().enumItems(&i, (void**)&p_grp_id);)
				if(CheckFlag(pFilt->GrpID, GF_DYNAMIC) && P_Tbl->GetGroupFilt(pFilt->GrpID, &grp_filt) > 0)
					if(CheckForFilt(&grp_filt, id, 0) < 0)
						ok = -1;
			if(ok < 0)
				CFF_RET;
		}
		else if(CheckFlag(pFilt->GrpID, GF_DYNAMIC) && P_Tbl->GetGroupFilt(pFilt->GrpID, &grp_filt) > 0)
			if(CheckForFilt(&grp_filt, id, 0) < 0)
				CFF_RET;
	}
	if(!(implFlags & GOCFIF_TAG) && pFilt->P_TagF) {
		if(!PPObjTag::CheckForTagFilt(PPOBJ_GOODS, id, pFilt->P_TagF))
			CFF_RET;
	}
	if(!(implFlags & GOCFIF_SYSJ) && pFilt->P_SjF) {
		SysJournal * p_sj = DS.GetTLA().P_SysJ;
		if(p_sj && p_sj->CheckObjForFilt(PPOBJ_GOODS, id, pFilt->P_SjF) < 0)
			CFF_RET;
	}
	return negation ? 0 : 1;
#undef CFF_RET
}

int SLAPI PPObjGoods::GetOpenedList(PPID locID, const PPIDArray * pOverlapList, UintHashTable & rHash)
{
	const uint Threshold = 30;

	int    ok = 1;
	int    done = 0;
	int    use_minmax = 0;
	long   _min = MAXLONG;
	long   _max = 0;
	CurRestTbl * p_cr = &BillObj->trfr->CRest;
	union {
		CurRestTbl::Key0 k0;
		CurRestTbl::Key1 k1;
	} key;
	if(pOverlapList) {
		const uint ol_c = pOverlapList->getCount();
		if(ol_c > Threshold) {
			for(uint i = 0; i < ol_c; i++) {
				const long goods_id = pOverlapList->get(i);
				SETMIN(_min, goods_id);
				SETMAX(_max, goods_id);
			}
			use_minmax = 1;
		}
		else if(ol_c) {
			for(uint i = 0; i < ol_c; i++) {
				const long goods_id = pOverlapList->get(i);
				key.k0.GoodsID = goods_id;
				if(locID) {
					key.k0.LocID = locID;
					if(p_cr->search(0, &key.k0, spEq) && p_cr->data.Rest > 0.0) {
						THROW_SL(rHash.Add(goods_id));
					}
				}
				else {
					key.k0.LocID = 0;
					if(p_cr->search(0, &key.k0, spGe) && p_cr->data.GoodsID == goods_id) {
						do {
							if(p_cr->data.Rest > 0.0) {
								THROW_SL(rHash.Add(goods_id));
								break;
							}
						} while(p_cr->search(0, &key.k0, spNext) && p_cr->data.GoodsID == goods_id);
					}
				}
			}
			done = 1;
		}
		else
			done = 1;
	}
	if(!done) {
		int    idx = 0;
		MEMSZERO(key);
		DBQ * dbq = 0;
		if(locID) {
			idx = 1;
			key.k1.LocID = locID;
			dbq = &(*dbq && p_cr->LocID == locID);
			if(use_minmax) {
				key.k1.GoodsID = _min;
				dbq = &(*dbq && p_cr->GoodsID >= _min && p_cr->GoodsID <= _max);
			}
			else
				dbq = &(*dbq && p_cr->GoodsID > 0L);
		}
		else {
			idx = 0;
			if(use_minmax) {
				key.k0.GoodsID = _min;
				dbq = &(*dbq && p_cr->GoodsID >= _min && p_cr->GoodsID <= _max);
			}
			else
				dbq = &(*dbq && p_cr->GoodsID > 0L);
		}
		dbq = &(*dbq && p_cr->Rest > 0.0);
		BExtQuery q(p_cr, idx, 256);
		q.select(p_cr->GoodsID, 0).where(*dbq);
		for(q.initIteration(0, &key, spGe); q.nextIteration() > 0;) {
			const PPID goods_id = p_cr->data.GoodsID;
			if(!pOverlapList || pOverlapList->lsearch(goods_id, 0)) {
				THROW_SL(rHash.Add(goods_id));
			}
		}
	}
	CATCHZOK
	return ok;
}

ListBoxDef * SLAPI PPObjGoods::_Selector2(ListBoxDef * pDef, long parent, long options, void * extraPtr, PPID locID, PPID mtxLocID)
{
	struct LbxDataGoods {
		long   ParentID;
		int    Gen;
		long   Options;
		void * ExtraPtr;
	} lbx_extra;
	if(pDef) {
		size_t s = sizeof(lbx_extra);
		MEMSZERO(lbx_extra);
		pDef->GetUserData(&lbx_extra, &s);
		parent = lbx_extra.ParentID;
		options = lbx_extra.Options;
		extraPtr = lbx_extra.ExtraPtr;
	}
	else {
		lbx_extra.ParentID = parent;
		lbx_extra.Gen = 0;
		lbx_extra.Options = options;
		lbx_extra.ExtraPtr = extraPtr;
	}
	const  int  code_prefix = BIN(DS.CheckExtFlag(ECF_CODEPREFIXEDLIST));
	const  long grp   = (parent == LONG_MIN) ? 0 : labs(parent);
	const  long brand = (options & selfExtLongAsBrand) ? (long)extraPtr : 0;
	const  int  exist_only = BIN(parent < 0);
	const  int  dont_sel_passive = (options & selfHidePassive) ? 1 : ((options & selfForcePassive) ? 0 : BIN(GetConfig().Flags & GCF_DONTSELPASSIVE));
	const  PPID loc_id = NZOR(locID, LConfig.Location);
	const  PPID mtx_qk_id = (options & selfUseMatrix) ? GetConfig().MtxQkID : 0;
	Goods2Tbl * t = 0;
	DBQuery   * p_q = 0;
	StrAssocArray * p_array = 0;
	ListBoxDef  * p_def = 0;
	PPIDArray list;
	union {
		Goods2Tbl::Key2 k2;
		Goods2Tbl::Key4 k4;
	} k_;
	if(oneof3(Kind, PPGDSK_TRANSPORT, PPGDSK_BRAND, PPGDSK_PCKGTYPE)) {
		BExtQuery q(P_Tbl, (options & selfByName) ? 2 : 4);
		DBQ * dbq = &(P_Tbl->Kind == Kind);
		if(Kind == PPGDSK_TRANSPORT && (long)extraPtr)
			dbq = &(*dbq && P_Tbl->GdsClsID == (long)extraPtr);
		q.select(P_Tbl->ID, P_Tbl->Name, P_Tbl->GoodsTypeID, P_Tbl->Flags, 0L).where(*dbq);
		MEMSZERO(k_);
		k_.k2.Kind = Kind;
		SETIFZ(p_array, new StrAssocArray);
		THROW_MEM(p_array);
		for(q.initIteration(0, &k_, spGe); q.nextIteration() > 0;)
			THROW_SL(p_array->AddFast(P_Tbl->data.ID, P_Tbl->data.Name)); // @v7.9.0 Add-->AddFast
	}
	else if(mtx_qk_id || grp || brand || exist_only || code_prefix) { // @v8.4.11 code_prefix
		int    r = -1;
		SETIFZ(p_array, new StrAssocArray);
		PPJobSrvClient * p_cli = DS.GetClientSession(0);
		if(p_cli) {
			SString q;
			q.Cat("SELECT").Space().Cat("GOODS").Space().Cat("BY").Space();
			if(grp)
				q.Cat("PARENT").CatParStr(grp).Space();
			if(brand)
				q.Cat("BRAND").CatParStr(brand).Space();
			if(exist_only)
				q.Cat("ACTUAL").Space();
			if(mtx_qk_id) {
				if(mtxLocID)
					q.Cat("MATRIXLOC").CatParStr(mtxLocID).Space();
				else
					q.Cat("MATRIX").Space();
			}
			if(dont_sel_passive)
				q.Cat("PASSIVE").CatParStr("no").Space();
			q.Cat("LOCATION").CatParStr(loc_id).Space();
			q.Cat("FORMAT").Dot().Cat("BIN").CatParStr((const char *)0);
			PPJobSrvReply reply;
			if(p_cli->Exec(q, reply)) {
				reply.StartReading(0);
				THROW(reply.CheckRepError());
				p_array->Read(reply, 0);
				r = 1;
			}
		}
		if(r <= 0) {
			GoodsFilt gf(grp);
			gf.LocList.Add(loc_id);
			if(mtx_qk_id) {
				gf.Flags |= GoodsFilt::fRestrictByMatrix;
				gf.MtxLocID = mtxLocID;
			}
			if(brand)
				gf.BrandList.Add(brand);
			if(dont_sel_passive)
				gf.Flags |= GoodsFilt::fHidePassive;
			THROW_MEM(p_array);
			if(exist_only)
				gf.Flags |= GoodsFilt::fActualOnly;
			GoodsIterator::GetListByFilt(&gf, p_array, BIN(options & selfByName));
		}
		p_array->ClearParents();
		if(code_prefix) {
			StrAssocArray temp_array;
			SString text_buf;
			for(uint i = 0; i < p_array->getCount(); i++) {
				StrAssocArray::Item item = p_array->at_WithoutParent(i);
				if(FetchSingleBarcode(item.Id, text_buf = 0) > 0) {
					text_buf.CatCharN(' ', 3).Cat(item.Txt);
				}
				else
					text_buf = item.Txt;
				temp_array.AddFast(item.Id, text_buf);
			}
			*p_array = temp_array;
		}
	}
	else if(pDef)
		pDef->refresh();
	else {
		THROW(CheckTblPtr(t = new Goods2Tbl));
		DBQ * dbq = &(t->Kind == Kind);
		if(dont_sel_passive && Kind == PPGDSK_GOODS)
			dbq = &(*dbq && (t->Flags & GF_PASSIV) == 0L); // @memleak
		p_q = & select(t->ID, ((options & selfByName) ? t->Name : t->Abbr), 0L).from(t, 0L);
		p_q->where(*dbq).orderBy(t->Kind, ((options & selfByName) ? t->Name : t->Abbr), 0L);
	}
	if(!pDef) {
		if(p_q)
			p_def = new DBQListBoxDef(*p_q, lbtDblClkNotify | lbtFocNotify | lbtDisposeData);
		else if(p_array) {
			p_array->SortByText();
			p_def = new StrAssocListBoxDef(p_array, lbtDblClkNotify | lbtFocNotify | lbtDisposeData);
		}
		THROW_MEM(p_def);
		p_def->SetUserData(&lbx_extra, sizeof(lbx_extra));
	}
	else {
		if(p_array) {
			p_array->SortByText();
			((StrAssocListBoxDef *)pDef)->setArray(p_array);
		}
		p_def = pDef;
	}
	CATCH
		if(p_def)
			ZDELETE(p_def);
		else if(p_q)
			delete p_q;
		else {
			delete p_array;
			delete t;
		}
	ENDCATCH
	return p_def;
}

// virtual
StrAssocArray * SLAPI PPObjGoods::MakeStrAssocList(void * extraPtr)
{
	StrAssocListBoxDef * p_def = (StrAssocListBoxDef*)PPObjGoods::Selector(extraPtr);
	StrAssocArray * p_ret_list = 0;
	if(p_def) {
		p_ret_list = new StrAssocArray;
		*p_ret_list = *p_def->getArray();
	}
	ZDELETE(p_def);
	return p_ret_list;
}

int SLAPI PPObjGoods::GenGroupCode(int numDigits, SString & rBuf)
{
	char   temp[32], code[32];
	if(numDigits <= 0 || numDigits > 8)
		numDigits = 3;
	long   num = (long)fpow10i(numDigits-1);
	do {
		num++;
		if(num >= 20 && num < 30)
			num = 30;
		longfmtz(num, numDigits, temp, sizeof(temp));
		code[0] = '@';
		strcpy(code+1, temp);
	} while(P_Tbl->SearchBarcode(code, 0) > 0);
	rBuf = temp;
	return 1;
}

int SLAPI PPObjGoods::CheckBarcodeList(const PPGoodsPacket * pPack)
{
	int    ok = 1;
	if(pPack) {
		const int is_group_code = BIN(pPack->Rec.Kind == PPGDSK_GROUP);
		int    len, allow_empty = 1;
		SString msg_buf;
		PPIDArray code_len_list;
		char * p, k[32];
		BarcodeTbl::Rec bc_rec;
		GetConfig().GetCodeLenList(&code_len_list, &allow_empty);
		{
			PPID   gt_id = NZOR(pPack->Rec.GoodsTypeID, PPGT_DEFAULT);
			PPGoodsType2 gt_rec;
			if(gt_id && GtObj.Fetch(gt_id, &gt_rec) > 0 && gt_rec.Flags & GTF_REQBARCODE)
				allow_empty = 0;
		}
		if(pPack->Codes.getCount()) {
			for(uint i = 0; ok && i < pPack->Codes.getCount(); i++) {
				memzero(k, sizeof(k));
				len = strlen(strip(STRNSCPY(k, pPack->Codes.at(i).Code)));
				if(!(Cfg.Flags & GCF_BCNDIG))
					for(p = k; *p && ok; p++)
						THROW_PP((is_group_code && p == k && *p == '@') || (*p >= '0' && *p <= '9'), PPERR_INVBARCODE);
				if(!is_group_code) {
					THROW_PP_S(!code_len_list.getCount() || code_len_list.lsearch(len), PPERR_INVBCODELEN, k);
					strip(Cfg.WghtPrefix);
					THROW_PP((Cfg.Flags & GCF_ENABLEWP) || !Cfg.IsWghtPrefix(k), PPERR_INVBCODEPRFX);
					if(Cfg.Flags & GCF_SUPPRLZERO) {
						for(p = k; *p == '0';)
							p++;
						if(p != k)
							STRNSCPY(k, p);
						STRNSCPY(pPack->Codes.at(i).Code, k);
					}
				}
				if(P_Tbl->SearchBarcode(k, &bc_rec) > 0 && bc_rec.GoodsID != pPack->Rec.ID) {
					GetGoodsName(bc_rec.GoodsID, msg_buf);
					CALLEXCEPT_PP_S(PPERR_DUPBARCODE, msg_buf);
				}
				bc_rec = pPack->Codes.at(i);
				for(uint j = 0; ok && j < pPack->Codes.getCount(); j++) {
					if(j != i && strcmp(bc_rec.Code, pPack->Codes.at(j).Code) == 0) {
						CALLEXCEPT_PP_S(PPERR_DUPBARCODE, bc_rec.Code);
					}
				}
			}
		}
		else if(!is_group_code) {
			THROW_PP(allow_empty, PPERR_BARCODENEEDED);
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}

#if 0 // @v7.7.5 Модифицированный вариант функии определен ниже {

int SLAPI PPObjGoods::SearchByBarcode(const char * pCode, BarcodeTbl::Rec * pBcRec, Goods2Tbl::Rec * b, int adoptSearching)
{
	int    r = 1, check_digit_removed = 0;
	const  int bcchkdig = BIN(GetConfig().Flags & GCF_BCCHKDIG);
	char   bc[32];
	size_t init_len = strlen(pCode);
	STRNSCPY(bc, pCode);
	while((r = tbl->SearchByBarcode(bc, pBcRec, b)) < 0)
		if(adoptSearching) {
			size_t len = strlen(bc);
			if(adoptSearching != 1001 && (!check_digit_removed && (!bcchkdig || (bc[0] != '0' && len > 3 && len < 7)))) {
				if(oneof3(init_len, 13, 12, 8))
					bc[len-1] = 0;
				check_digit_removed = 1;
			}
			else if(bc[0] == '0')
				STRNSCPY(bc, bc+1);
			else if(adoptSearching != 1001)
				return SearchByBarcode(pCode, pBcRec, b, 1001); // @recursion
			else
				return r;
		}
		else
			return r;
	return r;
}

#else // }{

int SLAPI PPObjGoods::SearchByBarcode(const char * pCode, BarcodeTbl::Rec * pBcRec, Goods2Tbl::Rec * b, int adoptSearching)
{
	int    r = 1;
	const  int bcchkdig = BIN(GetConfig().Flags & GCF_BCCHKDIG);
	SString bc, org_bc, temp_buf;
	(org_bc = pCode).Strip();
	const size_t init_len = org_bc.Len();
	bc = org_bc;
	StringSet processed_list;
	processed_list.add(bc);
	for(int do_process = 1; do_process && (r = P_Tbl->SearchByBarcode(bc, pBcRec, b)) < 0;) {
		do_process = 0;
		if(adoptSearching) {
			//
			// Пытаемся приклеить в конце кода контрольную цифру
			//
			if(oneof3(init_len, 12, 11, 7)) {
				int    cd = CalcBarcodeCheckDigit(org_bc);
				(temp_buf = org_bc).CatChar('0'+cd);
				if(!processed_list.search(temp_buf, 0, 0)) {
					processed_list.add(bc = temp_buf);
					do_process = 1;
				}
			}
			if(!do_process) {
				//
				// Пытаемся убрать контрольную цифру
				//
				if(oneof3(init_len, 13, 12, 8)) {
					(temp_buf = org_bc).TrimRight();
					if(CalcBarcodeCheckDigit(temp_buf) == (org_bc.Last()-'0')) {
						if(!processed_list.search(temp_buf, 0, 0)) {
							processed_list.add(bc = temp_buf);
							do_process = 1;
						}
					}
				}
				//
				// Убираем по-одному ведущие нули
				//
				while(!do_process && bc.C(0) == '0') {
					bc.ShiftLeft();
					if(!processed_list.search(bc, 0, 0)) {
						processed_list.add(bc);
						do_process = 1;
					}
				}
			}
		}
	}
	if(adoptSearching && CConfig.Flags & CCFLG_DEBUG) {
		(temp_buf = 0).Cat("SearchByBarcode");
		if(r > 0)
			temp_buf.Eq().Cat(bc);
		else
			temp_buf.Cat("=#0");
		temp_buf.CatDiv(':', 0);
		for(uint ssp = 0; processed_list.get(&ssp, bc);)
			temp_buf.Space().Cat(bc);
		PPLogMessage(PPFILNAM_DEBUG_LOG, temp_buf, LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
	}
	return r;
}

#endif // }

int SLAPI PPObjGoods::SearchBy2dBarcode(const char * pCodeLine, BarcodeTbl::Rec * pRec, Goods2Tbl::Rec * pGoodsRec)
{
	return P_Tbl->SearchBy2dBarcode(pCodeLine, pRec, pGoodsRec);
}

int SLAPI PPObjGoods::DeleteObj(PPID id)
{
	return PutPacket(&id, 0, 0);
}

StrAssocArray * SLAPI PPObjGoods::CreateListByList(const PPIDArray * pIdList)
{
	StrAssocArray * p_list = new StrAssocArray;
	THROW_MEM(p_list);
	for(uint i = 0; i < pIdList->getCount(); i++) {
		Goods2Tbl::Rec goods_rec;
		if(Fetch(pIdList->at(i), &goods_rec) > 0)
			THROW_SL(p_list->Add(goods_rec.ID, goods_rec.Name));
	}
	p_list->SortByText();
	CATCH
		ZDELETE(p_list);
	ENDCATCH
	return p_list;
}

StrAssocArray * SLAPI PPObjGoods::CreateListByPrice(PPID locID, double price)
{
	StrAssocArray * p_list = 0;
	PPIDArray id_list;
	ReceiptTbl * rcpt = &BillObj->trfr->Rcpt;
	ReceiptTbl::Key3 k;
	BExtQuery  * p_q = new BExtQuery(rcpt, 3, 128);
	DBQ * dbq = &(rcpt->Closed == 0L && rcpt->GoodsID > 0L && rcpt->LocID == locID);
	{
		double ri = 0.0;
		if(modf(fabs(price), &ri) != 0.0)
			dbq = &(*dbq && rcpt->Price > R2(ri) && rcpt->Price < R2(ri+1));
		else
			dbq = &(*dbq && rcpt->Price == R2(price));
	}
	p_q->select(rcpt->GoodsID, rcpt->Price, 0L).where(*dbq);
	MEMSZERO(k);
	for(p_q->initIteration(0, &k, spGt); p_q->nextIteration() > 0;) {
		if(R2(rcpt->data.Price) == R2(price))
			id_list.add(rcpt->data.GoodsID);
	}
	ZDELETE(p_q);
	if(GetConfig().ACGI_Threshold > 0) {
		ReceiptTbl::Key1 k1;
		LDATE  dt = plusdate(LConfig.OperDate, -Cfg.ACGI_Threshold);
		THROW_MEM(p_q = new BExtQuery(rcpt, 1, 128));
		p_q->select(rcpt->GoodsID, 0L).where(rcpt->Dt >= dt && rcpt->Closed > 0L &&
			rcpt->GoodsID > 0L && rcpt->LocID == locID && rcpt->Price == price);
		MEMSZERO(k1);
		k1.Dt = dt;
		for(p_q->initIteration(0, &k1, spGe); p_q->nextIteration() > 0;)
			id_list.add(rcpt->data.GoodsID);
	}
	id_list.sortAndUndup();
	THROW(p_list = CreateListByList(&id_list));
	CATCH
		ZDELETE(p_list);
	ENDCATCH
	return p_list;
}

int SLAPI PPObjGoods::GetHierarchy(PPID goodsID, StrAssocArray * pList)
{
	CALLPTRMEMB(pList, Clear());
	int    ok = -1;
	int    r = -1;
	Goods2Tbl::Rec rec;
	PPIDArray trace;
	for(PPID id = goodsID; id && (r = Fetch(id, &rec)) > 0; id = rec.ParentID) {
		if(trace.addUnique(id) < 0) {
			//
			// Мы зациклились (замкнутый контур в дереве групп)
			//
			ok = 0;
			break;
		}
		else {
			CALLPTRMEMB(pList, AddFast(id, rec.Name)); // @v8.3.4 Add-->AddFast
			ok = 1;
		}
	}
	CALLPTRMEMB(pList, Reverse());
	return ok;
}

ListBoxDef * SLAPI PPObjGoods::Selector(void * extraPtr)
{
	return (ImplementFlags & implStrAssocMakeList) ? PPObject::Selector(extraPtr) : _Selector2(0, (long)extraPtr, 0, 0, 0, 0);
}

int SLAPI PPObjGoods::UpdateSelector(ListBoxDef * pDef, void * extraPtr)
{
	return (ImplementFlags & implStrAssocMakeList) ? PPObject::UpdateSelector(pDef, extraPtr) : BIN(_Selector2(pDef, (long)extraPtr, 0, 0, 0, 0));
}

int SLAPI PPObjGoods::Browse(void * extraPtr /*groupID*/)
{
	const PPID group_id = (PPID)extraPtr;
	if(group_id) {
		GoodsFilt flt;
		flt.GrpID = group_id;
		return PPView::Execute(PPVIEW_GOODS, &flt, 1, 0);
	}
	else
		return PPView::Execute(PPVIEW_GOODS, 0, 1, 0);
}

const PPGoodsConfig & SLAPI PPObjGoods::GetConfig() const
{
	return Cfg;
}

int SLAPI PPObjGoods::Edit(PPID * pID, void * extraPtr /*group*/)
{
	return Edit(pID, gpkndGoods, (PPID)extraPtr, 0, 0);
}

int SLAPI PPObjGoods::ViewVersion(PPID histID)
{
	int    ok = -1;
	if(histID) {
		if(TLP(HistGoods).IsOpened()) {
			SBuffer buf;
			PPGoodsPacket pack;
			THROW(HistGoods->Search(histID, &buf) > 0);
			THROW(SerializePacket_(0, &pack, &buf));
			THROW(Helper_Edit(&pack.Rec.ID, &pack, pack.GetPacketKind(), 0, 1));
			ok = 1;
		}
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPObjGoods::EditClsdGoods(PPGoodsPacket * pPack, int modifyOnlyExtRec)
{
	int    ok = -1, cmd = 0;
	PPObjGoodsClass gc_obj;
	PPGdsClsPacket  gc_pack;
	uint   dlg_id = DLG_SG;
	ClsdGoodsDialog * dlg = 0;

	if(modifyOnlyExtRec != 2)
		THROW(gc_obj.GetPacket(pPack->Rec.GdsClsID, &gc_pack) > 0);
	THROW(CheckDialogPtr(&(dlg = new ClsdGoodsDialog(dlg_id, &gc_pack, modifyOnlyExtRec)), 0));
	dlg->setDTS(pPack);
	while(ok <= 0 && ((cmd = ExecView(dlg)) == cmOK || cmd == cmFullGoodsDialog))
		if(dlg->getDTS(pPack)) {
			//
			// Закладываемся на то, что вызывающая функция (PPObjGoods::Helper_Edit)
			// обработает код возврата 1000 как сигнал к продолжению редактирования //
			// пакета товара в полном диалоге
			//
			ok = (cmd == cmFullGoodsDialog) ? 1000 : 1;
		}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int SLAPI PPObjGoods::Helper_Edit(PPID * pID, PPGoodsPacket * pPack, GoodsPacketKind gpk, int isNew, int viewOnly /*=0*/)
{
	int    r = cmCancel, use_std_dialog = 1;
	GoodsDialog * dlg = 0;
	if(gpk == gpkndGoods) {
		PPAccessRestriction accsr;
		ObjRts.GetAccessRestriction(accsr);
		if(accsr.OnlyGoodsGrpID && accsr.CFlags & PPAccessRestriction::cfStrictOnlyGoodsGrp) {
			if(*pID) {
				THROW_PP(BelongToGroup(*pID, accsr.OnlyGoodsGrpID, 0) > 0, PPERR_ONLYGOODSGRPVIOL);
			}
		}
		if(pPack->Rec.GdsClsID > 1) {
			PPObjGoodsClass gc_obj;
			PPGdsCls gc_rec;
			if(gc_obj.Search(pPack->Rec.GdsClsID, &gc_rec) > 0 && !(gc_rec.Flags & PPGdsCls::fStdEditDlg)) {
				use_std_dialog = 0;
				int    r2 = 0;
				while(!use_std_dialog && r != cmOK && (r2 = EditClsdGoods(pPack, 0)) > 0) {
					if(r2 == 1000) {
						//
						// Пользователь в сокращенном диалоге редактирования классового товара
						// выбрал команду редактирования полного диалога - переходим к редактированию
						// того же пакета в полном диалоге.
						//
						use_std_dialog = 1;
					}
					else {
						THROW(CheckRightsModByID(pID));
						if(!viewOnly) {
							if(PutPacket(pID, pPack, 1)) {
								if(isNew && gpk == gpkndGoods)
									SupplyAddingToAltGrp(*pID);
								r = cmOK;
							}
							else
								PPError();
						}
						else
							r = cmOK;
					}
				}
			}
		}
	}
	if(use_std_dialog) {
		uint   dlg_id = 0;
		switch(gpk) {
			case gpkndGoods:
				dlg_id = DLG_GOODS;
				if(IsAssetType(pPack->Rec.GoodsTypeID) > 0)
					dlg_id = DLG_ASSET;
				break;
			case gpkndOrdinaryGroup: dlg_id = DLG_GOODSGROUP; break;
			case gpkndFolderGroup:   dlg_id = DLG_GGRPFOLD;   break;
			case gpkndAltGroup:      dlg_id = DLG_GGRPALT;    break;
			default:                 dlg_id = 0;              break;
		}
		if(dlg_id) {
			int    valid_data = 0;
			THROW(CheckDialogPtr(&(dlg = new GoodsDialog(dlg_id))));
			THROW(dlg->setDTS(pPack));
			while(!valid_data && (r = ExecView(dlg)) == cmOK) {
				MemLeakTracer mlt;
				THROW(CheckRightsModByID(pID));
				if(viewOnly)
					valid_data = 1;
				else if(dlg->getDTS(pPack)) {
					if(PutPacket(pID, pPack, 1)) {
						if(isNew && gpk == gpkndGoods)
							SupplyAddingToAltGrp(*pID);
						valid_data = 1;
					}
					else
						PPError();
				}
			}
		}
	}
	CATCH
		r = 0;
	ENDCATCH
	delete dlg;
	return r;
}

int SLAPI PPObjGoods::AddDefaultBarcode(PPGoodsPacket * pPack)
{
	int    ok = -1;
	if(pPack && pPack->Rec.Kind == PPGDSK_GOODS) {
		Goods2Tbl::Rec grp_rec;
		if(Search(pPack->Rec.ParentID, &grp_rec) > 0 && grp_rec.DefBCodeStrucID) {
			PPBarcodeStruc bcs_rec;
			if(SearchObject(PPOBJ_BCODESTRUC, grp_rec.DefBCodeStrucID, &bcs_rec) > 0) {
				BarcodeTbl::Rec bc_rec;
				MEMSZERO(bc_rec);
				if(GetBarcodeByTemplate(grp_rec.ID, bcs_rec.Templ, bc_rec.Code)) {
					bc_rec.Qtty = 1;
					ok = pPack->Codes.insert(&bc_rec) ? 1 : PPSetErrorSLib();
				}
			}
		}
	}
	return ok;
}

int SLAPI PPObjGoods::Edit(PPID * pID, GoodsPacketKind gpk, long group, PPID clsID, char * pBarcode)
{
	int    r = cmCancel, is_locked = 0, is_new = 0;
	TDialog * kind_dlg = 0;
	PPGoodsPacket pack;
	THROW(EditPrereq(pID, 0, &is_new));
	if(!is_new) {
		THROW(GetPacket(*pID, &pack, 0) > 0);
		gpk = pack.GetPacketKind();
		THROW(Lock(*pID));
		is_locked = 1;
	}
	else {
		if(gpk == gpkndUndef) {
			THROW(CheckDialogPtr(&(kind_dlg = new TDialog(DLG_GGRPKIND))));
			kind_dlg->setCtrlUInt16(CTL_GGRPKIND_KIND, 0);
			if(ExecView(kind_dlg) == cmOK) {
				switch(kind_dlg->getCtrlUInt16(CTL_GGRPKIND_KIND)) {
					case 0:  gpk = gpkndOrdinaryGroup; break;
					case 1:  gpk = gpkndFolderGroup; break;
					case 2:  gpk = gpkndAltGroup; break;
					default: gpk = gpkndUndef; break;
				}
			}
			else
				gpk = gpkndUndef;
			ZDELETE(kind_dlg);
		}
		THROW(InitPacket(&pack, gpk, labs(group), clsID, pBarcode));
		if(pack.Codes.getCount() == 0)
			AddDefaultBarcode(&pack);
	}
	THROW(r = Helper_Edit(pID, &pack, gpk, is_new));
	CATCH
		r = PPErrorZ();
	ENDCATCH
	delete kind_dlg;
	if(is_locked)
		Unlock(*pID);
	return r;
}

int SLAPI PPObjGoods::AddBySample(PPID * pID, PPID sampleID)
{
	int    ok = cmCancel;
	PPGoodsPacket pack;
	GoodsPacketKind gpk;
	THROW(CheckRights(PPR_INS));
	THROW(GetPacket(sampleID, &pack, 0) > 0);
	gpk = pack.GetPacketKind();
	pack.Rec.ID = 0;
	if(!(pack.GS.Rec.Flags & GSF_NAMED)) {
		pack.Rec.StrucID = 0;
		pack.GS.Rec.ID = 0;
	}
	pack.ExtRec.GoodsID = 0;
	if(pack.Codes.getCount()) {
		pack.Codes.freeAll();
		AddDefaultBarcode(&pack);
	}
	pack.ArCodes.freeAll();
	THROW(ok = Helper_Edit(pID, &pack, gpk, 1));
	CATCHZOKPPERR
	return ok;
}

IMPL_DESTROY_OBJ_PACK(PPObjGoods, PPGoodsPacket);

int SLAPI PPObjGoods::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	if(p && p->Data) {
		uint   i;
		PPGoodsPacket * gp = (PPGoodsPacket*)p->Data;
		ProcessObjRefInArray(PPOBJ_GOODSGROUP, &gp->Rec.ParentID,    ary, replace);
		ProcessObjRefInArray(PPOBJ_GOODSTYPE,  &gp->Rec.GoodsTypeID, ary, replace);
		ProcessObjRefInArray(PPOBJ_UNIT,       &gp->Rec.UnitID,      ary, replace);
		ProcessObjRefInArray(PPOBJ_UNIT,       &gp->Rec.PhUnitID,    ary, replace);
		ProcessObjRefInArray(PPOBJ_PERSON,     &gp->Rec.ManufID,     ary, replace);
		ProcessObjRefInArray(PPOBJ_GOODSSTRUC, &gp->Rec.StrucID,     ary, replace);
		ProcessObjRefInArray(PPOBJ_GOODSTAX,   &gp->Rec.TaxGrpID,    ary, replace);
		ProcessObjRefInArray(PPOBJ_ASSTWROFFGRP, &gp->Rec.WrOffGrpID,  ary, replace);
		ProcessObjRefInArray(PPOBJ_GOODSCLASS, &gp->Rec.GdsClsID,    ary, replace);
		if(gp->Rec.Kind == PPGDSK_TRANSPORT) {
			ProcessObjRefInArray(PPOBJ_TRANSPMODEL, &gp->Rec.BrandID, ary, replace);
			ProcessObjRefInArray(PPOBJ_WORLD, &gp->Rec.DefBCodeStrucID, ary, replace);
		}
		else {
			ProcessObjRefInArray(PPOBJ_BRAND, &gp->Rec.BrandID, ary, replace);
			ProcessObjRefInArray(PPOBJ_BCODESTRUC, &gp->Rec.DefBCodeStrucID, ary, replace);
		}
		ProcessObjRefInArray(PPOBJ_PERSON,     &gp->Rec.RspnsPersonID, ary, replace);
		if(gp->P_Quots) {
			PPQuot * p_quot;
			for(i = 0; gp->P_Quots->enumItems(&i, (void**)&p_quot);) {
				ProcessObjRefInArray(PPOBJ_LOCATION, &p_quot->LocID, ary, replace);
				ProcessObjRefInArray(PPOBJ_QUOTKIND, &p_quot->Kind,  ary, replace);
				ProcessObjRefInArray(PPOBJ_CURRENCY, &p_quot->CurID, ary, replace);
				ProcessObjRefInArray(PPOBJ_ARTICLE,  &p_quot->ArID,  ary, replace);
			}
		}
		if(gp->P_Gled)
			ProcessObjRefInArray(PPOBJ_QCERT, &gp->P_Gled->QCertID,  ary, replace);
		if(gp->Rec.GdsClsID && !gp->IsExtRecEmpty()) {
			PPObjGoodsClass gc_obj;
			PPGdsClsPacket gc_pack;
			PPID   cls_id = gp->Rec.GdsClsID;
			{
				//
				// Сложный случай: во время акцепта товара на предварительном этапе
				// разрешения ссылок ИД класса необходимо получить из таблицы синхронизации.
				//
				PPID   prim_id = 0;
				if(!replace && pCtx->GetPrimaryObjID(PPOBJ_GOODSCLASS, cls_id, &prim_id) > 0)
					cls_id = prim_id;
			}
			if(gc_obj.Fetch(cls_id, &gc_pack) > 0) {
				ProcessObjRefInArray(gc_pack.PropKind.ItemsListID,  &gp->ExtRec.KindID,    ary, replace);
				ProcessObjRefInArray(gc_pack.PropGrade.ItemsListID, &gp->ExtRec.GradeID,   ary, replace);
				ProcessObjRefInArray(gc_pack.PropAdd.ItemsListID,   &gp->ExtRec.AddObjID,  ary, replace);
				ProcessObjRefInArray(gc_pack.PropAdd2.ItemsListID,  &gp->ExtRec.AddObj2ID, ary, replace);
			}
		}
		for(i = 0; i < gp->ArCodes.getCount(); i++) {
			ArGoodsCodeTbl::Rec & r_arcode = gp->ArCodes.at(i);
			ProcessObjRefInArray(PPOBJ_ARTICLE, &r_arcode.ArID, ary, replace);
		}
		for(i = 0; i < gp->Stock.MinStockList.getCount(); i++) {
			RAssoc & r_item = gp->Stock.MinStockList.at(i);
			ProcessObjRefInArray(PPOBJ_LOCATION, &r_item.Key, ary, replace);
		}
		// @v7.0.0 {
		for(i = 0; i < gp->Stock.PltList.getCount(); i++) {
			GoodsStockExt::Pallet & r_item = gp->Stock.PltList.at(i);
			ProcessObjRefInArray(PPOBJ_PALLET, &r_item.PalletTypeID, ary, replace);
		}
		// } @v7.0.0
		gp->TagL.ProcessObjRefs(ary, replace); // @v7.2.0
		// @v7.7.12 {
		PPIDArray * p_list = gp->GenericList.GetP();
		if(p_list) {
			for(i = 0; i < p_list->getCount(); i++) {
				PPID * p_id = &p_list->at(i);
				ProcessObjRefInArray(PPOBJ_GOODS, p_id, ary, replace);
			}
		}
		// } @v7.7.12
		ProcessObjRefInArray(PPOBJ_PROCESSOR,  &gp->Rec.DefPrcID,    ary, replace); // @v9.0.5
		return 1;
	}
	return -1;
}

int SLAPI PPObjGoods::SerializePacket(int dir, PPGoodsPacket * pPack, SBuffer & rBuf, SSerializeContext * pSCtx, const DBDivPack * pDestDbDiv)
{
	int    ok = 1;
	if(dir > 0 && DS.CheckStateFlag(CFGST_TRANSQUOT))
		pPack->Rec.Flags |= GF_TRANSQUOT;
	THROW_SL(P_Tbl->SerializeRecord(dir, &pPack->Rec, rBuf, pSCtx));
	THROW_SL(P_Tbl->GetExtTbl_().SerializeRecord(dir, &pPack->ExtRec, rBuf, pSCtx));
	THROW_SL(P_Tbl->GetBcTbl_().SerializeArrayOfRecords(dir, &pPack->Codes, rBuf, pSCtx));
	THROW_SL(P_Tbl->GetACodTbl_().SerializeArrayOfRecords(dir, &pPack->ArCodes, rBuf, pSCtx));
	THROW_SL(pSCtx->Serialize(dir, pPack->ExtString, rBuf));
	THROW(pPack->Stock.Serialize(dir, rBuf, pSCtx));
	THROW(pPack->TagL.Serialize(dir, rBuf, pSCtx)); // @v7.2.0
	THROW(pPack->GenericList.Serialize(dir, rBuf, pSCtx)); // @v7.7.12
	THROW(pPack->LinkFiles.Serialize(dir, (GetConfig().Flags & GCF_XCHG_SENDATTACHMENT) ? 0 : 1, rBuf, pSCtx)); // @v8.2.3
	{
		PPQuotArray quot_list;
		if(dir > 0) {
			if(pPack->P_Quots) {
				for(int32 i = 0; i < (int32)pPack->P_Quots->getCount(); i++) {
					PPQuot & r_item = pPack->P_Quots->at(i);
					if(!pDestDbDiv || pDestDbDiv->ResponsibleForLoc(r_item.LocID, 1 /*for quot*/))
						THROW_SL(quot_list.insert(&r_item));
				}
			}
		}
		int32  c = (int32)quot_list.getCount();
		THROW_SL(pSCtx->Serialize(dir, c, rBuf));
		for(int32 i = 0; i < c; i++) {
			PPQuot item;
			if(dir > 0)
				item = quot_list.at(i);
			THROW_SL(pSCtx->Serialize(dir, item.ID, rBuf));
			THROW_SL(pSCtx->Serialize(dir, item.ArID, rBuf));
			THROW_SL(pSCtx->Serialize(dir, item.GoodsID, rBuf));
			THROW_SL(pSCtx->Serialize(dir, item.LocID, rBuf));
			THROW_SL(pSCtx->Serialize(dir, item.Kind, rBuf));
			THROW_SL(pSCtx->Serialize(dir, item.CurID, rBuf));
			THROW_SL(pSCtx->Serialize(dir, item.Flags, rBuf));
			THROW_SL(pSCtx->Serialize(dir, item.MinQtty, rBuf));
			THROW_SL(pSCtx->Serialize(dir, item.Quot, rBuf));
			THROW_SL(pSCtx->Serialize(dir, item.Dtm, rBuf)); // @v7.2.0
			THROW_SL(pSCtx->Serialize(dir, item.Period.low,  rBuf)); // @v7.3.5
			THROW_SL(pSCtx->Serialize(dir, item.Period.upp,  rBuf)); // @v7.3.5
			if(dir < 0) {
				item.ID = 0;
				THROW_SL(quot_list.insert(&item));
			}
		}
		if(dir < 0) {
			if(quot_list.getCount()) {
				if(pPack->P_Quots)
					*pPack->P_Quots = quot_list;
				else
					THROW_MEM(pPack->P_Quots = new PPQuotArray(quot_list));
			}
			else
				ZDELETE(pPack->P_Quots);
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjGoods::Read(PPObjPack * p, PPID id, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	THROW_MEM(p->Data = new PPGoodsPacket);
	PPGoodsPacket * gp = (PPGoodsPacket*)p->Data;
	if(stream == 0) {
		PPGoodsConfig cfg;
		THROW(GetPacket(id, gp, 0) > 0);
		CALLPTRMEMB(gp->P_Quots, RemoveHangedLocations());
		const long _gf = gp->Rec.Flags;
		// @v7.7.12 {
		if(GetConfig().Flags & GCF_XCHG_SENDGENGOODSCONTENT && gp->Rec.Kind == PPGDSK_GOODS && _gf & GF_GENERIC) {
			PPIDArray generic_list;
			GetGenericList(id, &generic_list);
			gp->GenericList.Set(&generic_list);
		}
		// } @v7.7.12
		// @v8.1.12 {
		else if(gp->Rec.Kind == PPGDSK_GROUP && (_gf & GF_ALTGROUP) && !(_gf & GF_DYNAMIC)) {
			if(P_Tbl->FetchConfig(&cfg) > 0 && cfg.Flags & GCF_XCHG_SENDALTGROUP) {
				PPIDArray member_list;
				PPRef->Assc.GetListByPrmr(PPASS_ALTGOODSGRP, id, &member_list);
				gp->GenericList.Set(&member_list);
			}
		}
		// } @v8.1.12
		// @v8.2.3 {
		if(oneof2(gp->Rec.Kind, PPGDSK_GOODS, PPGDSK_BRAND)) {
			if(P_Tbl->FetchConfig(&cfg) > 0 && cfg.Flags & GCF_XCHG_SENDATTACHMENT) {
				gp->LinkFiles.Init(Obj);
				gp->LinkFiles.Load(gp->Rec.ID, 0L);
			}
		}
		// } @v8.2.3
	}
	else {
		SBuffer buffer;
		THROW_SL(buffer.ReadFromFile((FILE*)stream, 0))
		THROW(SerializePacket(-1, gp, buffer, &pCtx->SCtx, pCtx->P_DestDbDivPack));
	}
	CATCHZOK
	return ok;
}
//
// Проверка на дублирование кода группы или товара.
// Если код дублируется в принимающей базе, то он заменяется //
// на пустой код.
//
// private
int SLAPI PPObjGoods::RemoveDupBarcodes(PPGoodsPacket * pPack, ObjTransmContext * pCtx)
{
	if(oneof2(pPack->Rec.Kind, PPGDSK_GROUP, PPGDSK_GOODS)) {
		SString fmt_buf, msg_buf, text_buf, code_buf;
		for(int i = pPack->Codes.getCount()-1; i >= 0; i--) {
			code_buf = pPack->Codes.at(i).Code;
			BarcodeTbl::Rec bc_rec;
			if(code_buf.NotEmptyS() && P_Tbl->SearchBarcode(code_buf, &bc_rec) > 0 && bc_rec.GoodsID != pPack->Rec.ID) {
				(text_buf = pPack->Rec.Name).Space().Cat(code_buf.Quot('[', ']'));
				if(fmt_buf.Empty())
					PPLoadText(PPTXT_RMVDUPBARCODE, fmt_buf);
				msg_buf.Printf(fmt_buf, pPack->Rec.ID, (const char *)text_buf);
				CALLPTRMEMB(pCtx, OutReceivingMsg(msg_buf));
				pPack->Codes.atFree(i);
			}
		}
	}
	return 1;
}

int SLAPI PPObjGoods::RemoveDupArCodes(PPGoodsPacket * pPack, ObjTransmContext * pCtx)
{
	if(pPack->Rec.Kind == PPGDSK_GOODS) {
		SString fmt_buf, msg_buf;
		const PPID id = pPack->Rec.ID;
		for(int i = pPack->ArCodes.getCount()-1; i >= 0; i--) {
			ArGoodsCodeTbl::Rec & r_rec = pPack->ArCodes.at(i);
			ArGoodsCodeTbl::Rec temp_rec;
			if(r_rec.Code[0] && P_Tbl->SearchByArCode(r_rec.ArID, r_rec.Code, &temp_rec, 0) > 0 && temp_rec.GoodsID != id) {
				if(fmt_buf.Empty())
					PPLoadText(PPTXT_RMVDUPARCODE, fmt_buf);
				PPFormat(fmt_buf, &msg_buf, pPack->Rec.Name, r_rec.Code, r_rec.ArID);
				CALLPTRMEMB(pCtx, OutReceivingMsg(msg_buf));
				pPack->ArCodes.atFree(i);
			}
		}
	}
	return 1;
}

int SLAPI PPObjGoods::AcceptQuot(PPID * pID, PPGoodsPacket * pPack, ObjTransmContext * pCtx)
{
	int    ok = 1;
	if(*pID) {
		PPQuotArray zero_list;
		PPQuotArray * p_quot_list = NZOR(pPack->P_Quots, &zero_list);
		p_quot_list->GoodsID = *pID ;
		p_quot_list->SetManagedLocList(pCtx->P_SrcDbDivPack ? &pCtx->P_SrcDbDivPack->LocList : 0);
		if(!PutQuotList(*pID, p_quot_list, 1)) {
			pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTGOODSQUOTS, pPack->Rec.ID, pPack->Rec.Name);
			ok = -1;
		}
	}
	return ok;
}

int SLAPI PPObjGoods::AcceptPacket(PPID * pID, PPGoodsPacket * pPack, ObjTransmContext * pCtx)
{
	//
	// Если в пакете установлен флаг GF_TRANSQUOT, то сам пакет не изменяем, а меняем только котировки.
	//
	int    ok = 1, r;
	if(pPack->Rec.Flags & GF_TRANSQUOT) {
		ok = AcceptQuot(pID, pPack, pCtx);
	}
	else {
		int    is_new = BIN(!*pID);
		int    unchanged = 0;
		RemoveDupBarcodes(pPack, pCtx);
		RemoveDupArCodes(pPack, pCtx);
		if(!is_new && GetConfig().Flags & GCF_XCHG_DONTRCVTAXGRPUPD)
			pPack->UpdFlags |= PPGoodsPacket::ufDontChgTaxGrp;
		if(pPack->LinkFiles.GetState() & ObjLinkFiles::stTransmissionNotSupported) // @v8.2.3
			pPack->UpdFlags |= PPGoodsPacket::ufDontChgImgFlag;
		// @v7.2.0 pPack->UpdFlags |= PPGoodsPacket::ufDontChgPltList;
		r = PutPacket(pID, pPack, 1);
		if(!r) {
			pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTGOODS, pPack->Rec.ID, pPack->Rec.Name);
			ok = -1;
		}
		else {
			if(r < 0)
				unchanged = 1;
			ok = AcceptQuot(pID, pPack, pCtx);
			if(ok > 0) {
				const long _gf = pPack->Rec.Flags;
				// @v7.7.12 {
				if(pPack->Rec.Kind == PPGDSK_GOODS && _gf & GF_GENERIC && pPack->GenericList.IsExists()) {
					ok = P_Tbl->SetGenericList(*pID, pPack->GenericList.Get(), 1);
					if(!ok) {
						pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTGOODSGENLIST, pPack->Rec.ID, pPack->Rec.Name);
					}
				}
				// @v8.1.12 {
				else if(pPack->Rec.Kind == PPGDSK_GROUP && (_gf & GF_ALTGROUP) && !(_gf & GF_DYNAMIC)) {
					if(!!pPack->GenericList) {
						ok = P_Tbl->SetAltGrpList(*pID, pPack->GenericList.Get(), 1);
						if(!ok) {
							pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTALTGRPLIST, pPack->Rec.ID, pPack->Rec.Name);
						}
					}
				}
				// } @v8.1.12
				if(ok > 0) {
				// } @v7.7.12
					ok = unchanged ? 1 : (is_new ? 101 : 102); // @ObjectCreated : @ObjectUpdated
				}
			}
		}
	}
	return ok;
}
//
//
//
int SLAPI PPObjGoods::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx) // @srlz
{
	int    ok = 1;
	if(p && p->Data) {
		PPGoodsPacket * gp = (PPGoodsPacket*)p->Data;
		if(stream == 0) {
			Goods2Tbl::Rec temp_rec;
			if((*pID || SearchMaxLike(gp, pID) > 0) && Search(*pID, &temp_rec) > 0) {
				if(gp->Rec.Flags & GF_TRANSQUOT) {
					gp->Rec.ID = *pID;
					AcceptPacket(pID, gp, pCtx);
					ok = -1; // Не фиксируем факт приема товара во избежании неверного отражения в таблице синхронизации
				}
				else if(!(pCtx->Cfg.Flags & DBDXF_NOUPDGOODS)) {
					gp->Rec.ID = *pID;
					//
					// Структуры передаются. Однако, во избежании затирания связки товар-структура
					// при приеме товара из другого раздела, если поступивший товар не содержит структуры,
					// то оставляем ее как есть.
					//
					if(gp->Rec.StrucID == 0 || (!(GetConfig().Flags & GCF_XCHG_RCVSTRUCUPD) && temp_rec.StrucID))
						gp->Rec.StrucID = temp_rec.StrucID;
					if(gp->Rec.StrucID)
						if(GSObj.Get(gp->Rec.StrucID, &gp->GS) > 0)
							gp->GS.GoodsID = *pID;
						else
							gp->Rec.StrucID = 0;
					//
					ok = AcceptPacket(pID, gp, pCtx);
				}
			}
			else {
				gp->Rec.ID = *pID = 0;
				//
				// Если товара нет в базе данных, то придется его принять несмотря на то, что
				// предписано принять только котировки
				//
				gp->Rec.Flags &= ~GF_TRANSQUOT;
				ok = AcceptPacket(pID, gp, pCtx);
			}
		}
		else {
			SBuffer buffer;
			THROW(SerializePacket(+1, gp, buffer, &pCtx->SCtx, pCtx->P_DestDbDivPack));
			THROW_SL(buffer.WriteToFile((FILE*)stream, 0, 0))
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}
//
//
//
SLAPI GoodsStockExt::GoodsStockExt()
{
	Init();
}

int SLAPI GoodsStockExt::IsEmpty() const
{
	int    min_stock_empty = 1;
	uint   min_stock_count = MinStockList.getCount();
	for(uint i = 0; min_stock_empty && i < min_stock_count; i++)
		min_stock_empty = (MinStockList.at(i).Val != 0.0) ? 0 : 1;
	return (min_stock_empty && !Brutto && !PckgDim && !RtlDim && !Package && !ExpiryPeriod && !MinShippmQtty && !PltList.getCount());
}

int FASTCALL GoodsStockExt::IsEqual(const GoodsStockExt & rS) const
{
	if(Brutto != rS.Brutto)
		return 0;
	else if(PckgDim != rS.PckgDim)
		return 0;
	else if(Package != rS.Package)
		return 0;
	else if(ExpiryPeriod != rS.ExpiryPeriod)
		return 0;
	else if(GseFlags != rS.GseFlags)
		return 0;
	else if(MinShippmQtty != rS.MinShippmQtty)
		return 0;
	else if(RtlDim != rS.RtlDim)
		return 0;
	else if(!MinStockList.IsEqual(rS.MinStockList))
		return 0;
	else if(!PltList.IsEqual(rS.PltList))
		return 0;
	else
		return 1;
}

int SLAPI GoodsStockExt::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW_SL(pCtx->Serialize(dir, Brutto, rBuf));
	THROW(PckgDim.Serialize(dir, rBuf, pCtx));
	THROW_SL(pCtx->Serialize(dir, Package, rBuf));
	THROW_SL(pCtx->Serialize(dir, ExpiryPeriod, rBuf));
	// @todo RtlDim, MinShippmQtty
	// @v7.2.7 {
	if(dir < 0) {
		RtlDim.Reset();
		MinShippmQtty = 0.0;
	}
	// } @v7.2.7
	THROW_SL(pCtx->Serialize(dir, &MinStockList, rBuf));
	// @todo PltList @done @v7.0.0
	THROW_SL(pCtx->Serialize(dir, &PltList, rBuf)); // @v7.0.0
	/* @v7.2.0
	if(dir < 0)
		PltList.freeAll();
	*/
	CATCHZOK
	return ok;
}

double SLAPI GoodsStockExt::CalcBrutto(double qtty) const
{
	double count = 0.0;
	if(Package > 0) {
		/* Алгорит расчета объема с выравниванием до полной упаковки
		double fract = R6(modf(round(fabs(qtty) / Package, 6), &count));
		if(fract != 0)
			count += 1L;
		*/
		count = fabs(qtty) / Package;
	}
	else
		count = fabs(qtty);
	return (count * (double)Brutto) / 1000.0;
}

double SLAPI GoodsStockExt::CalcVolume(double qtty) const
{
	double count = 0.0;
	if(Package > 0) {
		/* Алгорит расчета объема с выравниванием до полной упаковки
		double fract = R6(modf(round(fabs(qtty) / Package, 6), &count));
		if(fract != 0)
			count += 1L;
		*/
		count = fabs(qtty) / Package;
	}
	else
		count = fabs(qtty);
	return (count * PckgDim.CalcVolumeM());
}

int SLAPI GoodsStockExt::SetVolume(double volume)
{
	return PckgDim.SetVolumeM(volume);
}

int SLAPI GoodsStockExt::Init()
{
	memzero(this, offsetof(GoodsStockExt, MinStockList));
	MinStockList.freeAll();
	PltList.freeAll();
	return 1;
}

GoodsStockExt & FASTCALL GoodsStockExt::operator = (const GoodsStockExt & rSrc)
{
	memcpy(this, &rSrc, offsetof(GoodsStockExt, MinStockList));
	MinStockList = rSrc.MinStockList;
	PltList = rSrc.PltList;
	return *this;
}

double SLAPI GoodsStockExt::GetMaxMinStock(PPID * pLocID)
{
	uint min_stock_count = MinStockList.getCount();
	double min_stock = 0;
	PPID   loc_id = 0;
	uint   i = 0;
	for(; !min_stock && i < min_stock_count; i++) {
		RAssoc * p_item = &MinStockList.at(i);
		if(min_stock < p_item->Val) {
			min_stock = p_item->Val;
			loc_id    = p_item->Key;
		}
	}
	ASSIGN_PTR(pLocID, MinStockList.at(i).Key);
	return min_stock;
}

double SLAPI GoodsStockExt::GetMinStock(PPID locID, int useZeroLoc /*=1*/)
{
	double min_stock = MinStockList.Get(locID, 0);
	if(min_stock == 0.0 && locID && useZeroLoc)
		min_stock = MinStockList.Get(0, 0);
	return min_stock;
}

int SLAPI GoodsStockExt::SetMinStock(PPID locID, double qtty)
{
	uint pos = 0;
	if(MinStockList.Search(locID, 0, &pos, 1) > 0)
		MinStockList.at(pos).Val = qtty;
	else
		MinStockList.Add(locID, qtty, 0, 1);
	return 1;
}

int SLAPI GoodsStockExt::GetPalletEntry(PPID palletTypeID, Pallet * pEntry) const
{
	int    ok = -1;
	if(palletTypeID <= 0)
		ok = GetSinglePalletEntry(pEntry);
	else {
		memzero(pEntry, sizeof(*pEntry));
		for(uint i = 0; ok < 0 && i < PltList.getCount(); i++) {
			if(PltList.at(i).PalletTypeID == palletTypeID) {
				ASSIGN_PTR(pEntry, PltList.at(i));
				ok = 1;
			}
		}
	}
	return ok;
}

int SLAPI GoodsStockExt::GetSinglePalletEntry(Pallet * pEntry) const
{
	int    ok = -1;
	const  uint c = PltList.getCount();
	if(c) {
		if(c == 1) {
			ASSIGN_PTR(pEntry, PltList.at(0));
			ok = 1;
		}
		else {
			PPLocationConfig loc_cfg;
			PPObjLocation::FetchConfig(&loc_cfg);
			if(loc_cfg.DefPalletID) {
				for(uint i = 0; ok < 0 && i < c; i++) {
					if(PltList.at(i).PalletTypeID == loc_cfg.DefPalletID) {
						ASSIGN_PTR(pEntry, PltList.at(i));
						ok = 1;
					}
				}
			}
			if(ok < 0) {
				ASSIGN_PTR(pEntry, PltList.at(0));
				ok = 1;
			}
		}
	}
	else
		memzero(pEntry, sizeof(*pEntry));
	return ok;
}

int SLAPI GoodsStockExt::ConvertCargoUnits(int fromCargoUnit, int toCargoUnit, PPID palletTypeID, double srcVal, double * pDestVal, long flags) const
{
	int    ok = -1;
	double result = 0.0;
	Pallet plt;
	if(fromCargoUnit == CARGOUNIT_ITEM) {
		if(toCargoUnit == CARGOUNIT_ITEM) {
			result = srcVal;
			ok = 1;
		}
		else if(toCargoUnit == CARGOUNIT_PCKG) {
			if(Package > 0.0) {
				result = srcVal / Package;
				ok = 1;
			}
		}
		else if(toCargoUnit == CARGOUNIT_PALLET) {
			if(GetPalletEntry(palletTypeID, &plt) > 0) {
				double layer = (double)plt.PacksPerLayer;
				double pckg_count = (plt.MaxLayers > 0) ? (layer * plt.MaxLayers) : layer;
				if(pckg_count > 0.0) {
					if(Package > 0.0)
						result = srcVal / (pckg_count * Package);
					else
						result = srcVal / pckg_count;
					ok = 1;
				}
			}
		}
	}
	else if(fromCargoUnit == CARGOUNIT_PCKG) {
		if(toCargoUnit == CARGOUNIT_ITEM) {
			if(Package > 0.0) {
				result = srcVal * Package;
				ok = 1;
			}
		}
		else if(toCargoUnit == CARGOUNIT_PCKG) {
			result = srcVal;
			ok = 1;
		}
		else if(toCargoUnit == CARGOUNIT_PALLET) {
			if(GetPalletEntry(palletTypeID, &plt) > 0) {
				double layer = (double)plt.PacksPerLayer;
				double pckg_count = (plt.MaxLayers > 0) ? (layer * plt.MaxLayers) : layer;
				if(pckg_count > 0.0) {
					result = srcVal / pckg_count;
					ok = 1;
				}
			}
		}
	}
	else if(fromCargoUnit == CARGOUNIT_PALLET) {
		if(toCargoUnit == CARGOUNIT_ITEM) {
			if(GetPalletEntry(palletTypeID, &plt) > 0) {
				double layer = (double)plt.PacksPerLayer;
				double pckg_count = (plt.MaxLayers > 0) ? (layer * plt.MaxLayers) : layer;
				if(pckg_count > 0.0) {
					if(Package > 0.0)
						result = srcVal * (pckg_count * Package);
					else
						result = srcVal * pckg_count;
					ok = 1;
				}
			}
		}
		else if(toCargoUnit == CARGOUNIT_PCKG) {
			if(GetPalletEntry(palletTypeID, &plt) > 0) {
				double layer = (double)plt.PacksPerLayer;
				double pckg_count = (plt.MaxLayers > 0) ? (layer * plt.MaxLayers) : layer;
				if(pckg_count > 0.0) {
					result = srcVal * pckg_count;
					ok = 1;
				}
			}
		}
		else if(toCargoUnit == CARGOUNIT_PALLET) {
			result = srcVal;
			ok = 1;
		}
	}
	ASSIGN_PTR(pDestVal, result);
	return ok;
}

int SLAPI PPObjGoods::GetStockExt(PPID id, GoodsStockExt * pData, int useCache /*=0*/)
{
	return P_Tbl->GetStockExt(id, pData, useCache);
}

int SLAPI PPObjGoods::GetAltGenGoodsList(PPID goodsID, int kind, RAssocArray * pList)
{
	int    ok = -1;
	PPID   gen_id = 0;
	if(BelongToGen(goodsID, &gen_id) > 0 || IsGeneric(gen_id = goodsID)) {
		PPIDArray goods_list;
		GetGenericList(gen_id, &goods_list);
		for(uint i = 0; i < goods_list.getCount(); i++) {
			PPID   id = goods_list.at(i);
			double ratio = 0.0;
			if(id != goodsID && IsGoodsCompatibleByUnit(id, goodsID, &ratio) > 0) {
				if(pList && !pList->Search(id, 0, 0, 0))
					pList->Add(id, ratio);
				ok = 1;
			}
		}
	}
	return ok;
}

int SLAPI PPObjGoods::GetStrucSubstList(PPID goodsID, RAssocArray * pList)
{
	int    ok = -1;
	PPGoodsStruc gs;
	PPGoodsStruc::Ident gsid(goodsID, GSF_SUBST, GSF_COMPL|GSF_DECOMPL|GSF_PARTITIAL);
	if(LoadGoodsStruc(&gsid, &gs) > 0) {
		PPGoodsStrucItem gsi;
		double ratio = 0;
		for(uint j = 0; gs.EnumItemsExt(&j, &gsi, goodsID, 1, &ratio) > 0;)
			if(gsi.GoodsID != goodsID && ratio != 0) {
				if(pList && !pList->Search(gsi.GoodsID, 0, 0, 0))
					pList->Add(gsi.GoodsID, ratio);
				ok = 1;
			}
	}
	return ok;
}

int SLAPI PPObjGoods::GetSubstList(PPID goodsID, int substStrucOnly, RAssocArray & rList)
{
	int    ok = -1;
	PPObjBill * p_bobj = BillObj;
	rList.clear();
	if(substStrucOnly || p_bobj->GetConfig().GoodsSubstMethod == PPBillConfig::gsmSubstStruc) {
		ok = GetStrucSubstList(goodsID, &rList);
	}
	else if(p_bobj->GetConfig().GoodsSubstMethod == PPBillConfig::gsmGeneric) {
		ok = GetAltGenGoodsList(goodsID, 0, &rList);
	}
	return ok;
}

//virtual
int  SLAPI PPObjGoods::RemoveObjV(PPID id, ObjCollection * pObjColl, uint options, void * pExtraParam)
{
	SETFLAG(options, not_addtolog, 1);
	return PPObject::RemoveObjV(id, pObjColl, options, pExtraParam);
}

int SLAPI PPObjGoods::GetRandomIdsAry(int count, PPIDArray * pAry)
{
	int    ok = -1;
	if(pAry) {
		UintHashTable opened_goods_list;
		GetOpenedList(LConfig.Location, 0, opened_goods_list);
		pAry->freeAll();
		int    i = 0;
		for(ulong id = 0; i <= count && opened_goods_list.Enum(&id);) {
			Goods2Tbl::Rec goods_rec;
			if(Fetch((long)id, &goods_rec) > 0 && goods_rec.Kind == PPGDSK_GOODS && !(goods_rec.Flags & (GF_PASSIV|GF_GENERIC))) {
				THROW_SL(pAry->add((long)id));
				i++;
			}
		}
		if(i)
			ok = 1;
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjGoods::CheckSpecQuot(PPID supplID, PPID goodsID, PPID locID, PPID currID)
{
	int    ok = 1;
	if(supplID) {
		PPSupplDeal sd;
		QuotIdent qi(locID, 0, currID, supplID);
		MEMSZERO(sd);
        if(GetSupplDeal(goodsID, qi, &sd, 1) > 0)
			THROW_PP(!sd.IsDisabled, PPERR_GOODSRCPTDISABLED);
	}
	if(LConfig.Flags & CFGFLG_USEGOODSMATRIX) {
		THROW(BelongToMatrix(goodsID, locID));
	}
	CATCHZOK
	return ok;
}

double SLAPI PPObjGoods::CalcLikeness(const Goods2Tbl::Rec * pRec1, const Goods2Tbl::Rec * pRec2, int * pSwap, long extra)
{
	int    swap = (strlen(pRec1->Name) > strlen(pRec2->Name)) ? 0 : 1;
	double rate = 0.0;
	ApproxStrCmp((swap) ? pRec2->Name : pRec1->Name, (swap) ? pRec1->Name : pRec2->Name, 1, &rate);
	ASSIGN_PTR(pSwap, swap);
	return rate;
}

int SLAPI PPObjGoods::GetTagList(PPID goodsID, ObjTagList * pTagList)
{
	return PPRef->Ot.GetList(Obj, goodsID, pTagList);
}

int SLAPI PPObjGoods::SetTagList(PPID goodsID, const ObjTagList * pTagList, int use_ta)
{
	return PPRef->Ot.PutList(Obj, goodsID, pTagList, use_ta);
}

SLAPI PPObjGoodsInfo::PPObjGoodsInfo(void * extraPtr) : PPObjReference(PPOBJ_GOODSINFO, extraPtr)
{
}

int SLAPI PPObjGoodsInfo::Edit(PPID * pID, void * extraPtr)
{
	int    ok = cmCancel, valid_data = 0;
	ushort v = 0;
	int    is_new = (*pID == 0);
	TDialog * dlg = 0;
	PPGoodsInfo rec;

	MEMSZERO(rec);
	THROW(CheckRights(PPR_READ));
	if(!is_new) {
		THROW(GetPacket(*pID, &rec) > 0);
	}
	THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_EGDSINFO)), 0));
	dlg->setCtrlData(CTL_EGDSINFO_NAME, rec.Name);
	SetupPPObjCombo(dlg, CTLSEL_EGDSINFO_LOC, PPOBJ_LOCATION, rec.LocID, 0, 0);
	SetupPPObjCombo(dlg, CTLSEL_EGDSINFO_TOUCHSC, PPOBJ_TOUCHSCREEN, rec.TouchScreenID, 0, 0);
	SetupPPObjCombo(dlg, CTLSEL_EGDSINFO_PRINTER, PPOBJ_BCODEPRINTER, rec.LabelPrinterID, 0, 0);
	dlg->AddClusterAssoc(CTL_EGDSINFO_FLAGS, 0, GIF_SHOWLOTS);
	dlg->SetClusterData(CTL_EGDSINFO_FLAGS, rec.Flags);
	while(!valid_data && (ok = ExecView(dlg)) == cmOK) {
		dlg->getCtrlData(CTL_EGDSINFO_NAME, rec.Name);
		if(strip(rec.Name)[0] == '\0')
			PPError(PPERR_NAMENEEDED);
		else {
			dlg->getCtrlData(CTLSEL_EGDSINFO_LOC, &rec.LocID);
			dlg->getCtrlData(CTLSEL_EGDSINFO_TOUCHSC, &rec.TouchScreenID);
			dlg->getCtrlData(CTLSEL_EGDSINFO_PRINTER, &rec.LabelPrinterID);
			dlg->GetClusterData(CTL_EGDSINFO_FLAGS, &rec.Flags);
			THROW(CheckRightsModByID(pID));
			THROW(PutPacket(pID, &rec, 1));
			valid_data = 1;
		}
	}
	CATCHZOK
	delete dlg;
	return ok;
}

int SLAPI PPObjGoodsInfo::GetPacket(PPID id, PPGoodsInfo * pPack)
{
	int    ok = 1;
	PPGoodsInfo pack;
	THROW((ok = PPRef->GetItem(PPOBJ_GOODSINFO, id, &pack)));
	ASSIGN_PTR(pPack, pack);
	CATCHZOK
	return ok;
}

int SLAPI PPObjGoodsInfo::PutPacket(PPID * pID, PPGoodsInfo * pPack, int use_ta)
{
	int    ok = -1;
	PPID   id = 0, action = 0;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(pPack) {
			pPack->ObjType = PPOBJ_GOODSINFO;
			pPack->ObjID   = 0;
		}
		if(pID && *pID) {
			if(pPack) {
				THROW(CheckRights(PPR_MOD));
				pPack->ObjID = *pID;
				THROW(PPRef->UpdateItem(PPOBJ_GOODSINFO, *pID, pPack, 1, 0));
				action = PPACN_OBJUPD;
			}
			else {
				THROW(CheckRights(PPR_DEL));
				THROW(PPRef->RemoveItem(PPOBJ_GOODSINFO, *pID, 0));
				action = PPACN_OBJRMV;
			}
			id = *pID;
		}
		else if(pPack) {
			THROW(CheckRights(PPR_INS));
			THROW(PPRef->AddItem(PPOBJ_GOODSINFO, &id, pPack, 0));
			action = PPACN_OBJADD;
			ASSIGN_PTR(pID, id);
		}
		DS.LogAction(action, Obj, id, 0, 0);
		THROW(tra.Commit());
	}
	ok = 1;
	CATCHZOK
	return ok;
}
//
//
//

#define GOODS_INDEX_FILE_NAME        "goods_index.xml"
#define GOODS_RAND_SET_FILE_NAME     "goods_random.xml"
#define GOODS_CHANGED_SET_FILE_NAME  "goods_changed.xml"
#define BRANDS_INDEX_FILE_NAME       "brands_index.xml"
#define STORES_INDEX_FILE_NAME       "stores_index.xml"
#define PERSON_INDEX_FILE_NAME       "person_index.xml"

int UhttIndexPersons(uint setSz, PPID personKindID, SString & rOutPath)
{
	int        ok = 1;
	int        closed = 0;
	uint       set_n = 1, addr_n = 0;
	SString    name_buf, context_buf;
	SString    path;
	PPViewPerson psn_view;
	PersonViewItem psn_item;

	THROW(!rOutPath.Empty());
	(path = 0).Cat(rOutPath).SetLastSlash().Cat(PERSON_INDEX_FILE_NAME);
	xmlTextWriterPtr writer = xmlNewTextWriterFilename(path, 0);
	if(writer) {
		PPObjPerson psn_obj;
		PPIDArray addr_list, local_addr_list;
		PersonFilt filt;
		filt.Kind = personKindID;
		THROW(psn_view.Init_(&filt));
		for(psn_view.InitIteration(); psn_view.NextIteration(&psn_item) > 0;) {
			local_addr_list.clear();
			psn_obj.GetDlvrLocList(psn_item.ID, &local_addr_list);
			addr_list.add(&local_addr_list);
		}
		addr_list.sortAndUndup();
		addr_list.shuffle(); // @v8.4.0
		xmlTextWriterSetIndent(writer, 1);
		xmlTextWriterSetIndentString(writer, (const xmlChar*)"\t");
		xmlTextWriterStartDocument(writer, 0, "utf-8", 0);
		xmlTextWriterStartElement(writer, (name_buf = 0).Cat("PersonAddrIndex").ucptr());
		uint c = addr_list.getCount();
		if(c) {
			do {
				const PPID addr_id = addr_list.get(--c);
				closed = 0;
				if(addr_n == 0) {
					xmlTextWriterStartElement(writer, (name_buf = 0).Cat("Set").ucptr());
					xmlTextWriterWriteAttribute(writer, (name_buf = 0).Cat("n").ucptr(), (context_buf = 0).Cat(set_n).ucptr());
				}
				xmlTextWriterWriteElement(writer, (name_buf = 0).Cat("Id").ucptr(), (context_buf = 0).Cat(addr_id).ucptr());
				addr_n++;
				if(addr_n >= setSz) {
					xmlTextWriterEndElement(writer);
					addr_n = 0;
					set_n++;
					closed = 1;
				}
			} while(c);
			if(!closed)
				xmlTextWriterEndElement(writer);
		}
		xmlTextWriterEndElement(writer);
	}
	CATCHZOK
	xmlFreeTextWriter(writer);
	return ok;
}

int UhttIndexGoods(uint setSz, SString & rOutPath)
{
	int        ok = 1;
	int        closed = 0;
	uint       set_n = 1, goods_n = 0;
	PPIDArray  id_list;
	SString    name_buf, context_buf;
	SString    path;
	Goods2Tbl::Rec goods_rec;
	GoodsIterator  goods_iter;
	THROW(!rOutPath.Empty());
	(path = 0).Cat(rOutPath).SetLastSlash().Cat(GOODS_INDEX_FILE_NAME);
	xmlTextWriterPtr writer = xmlNewTextWriterFilename(path, 0);
	if(writer) {
		THROW(goods_iter.Init(0));
		while(goods_iter.Next(&goods_rec) > 0) {
			id_list.add(goods_rec.ID);
		}
		// @v8.4.0 id_list.sort();
		id_list.shuffle(); // @v8.4.0
		xmlTextWriterSetIndent(writer, 1);
		xmlTextWriterSetIndentString(writer, (const xmlChar*)"\t");
		xmlTextWriterStartDocument(writer, 0, "utf-8", 0);
		xmlTextWriterStartElement(writer, (name_buf = 0).Cat("GoodsIndex").ucptr());
		uint c = id_list.getCount();
		if(c) {
			do {
				const PPID id = id_list.get(--c);
				closed = 0;
				if(goods_n == 0) {
					xmlTextWriterStartElement(writer, (name_buf = 0).Cat("Set").ucptr());
					xmlTextWriterWriteAttribute(writer, (name_buf = 0).Cat("n").ucptr(), (context_buf = 0).Cat(set_n).ucptr());
				}
				xmlTextWriterWriteElement(writer, (name_buf = 0).Cat("Id").ucptr(), (context_buf = 0).Cat(id).ucptr());
				goods_n++;
				if(goods_n >= setSz) {
					xmlTextWriterEndElement(writer);
					goods_n = 0;
					set_n++;
					closed = 1;
				}
			} while(c);
			if(!closed)
				xmlTextWriterEndElement(writer);
		}
		xmlTextWriterEndElement(writer);
	}
	CATCHZOK
	xmlFreeTextWriter(writer);
	return ok;
}

int UhttIndexRandomGoods(uint setSz, SString & rOutPath)
{
	int        ok = 1;
	uint       i = 0, iter_total, set_size;
	PPID       last_id = 0;
	Goods2Tbl      goods_tbl;
	Goods2Tbl::Rec goods_rec;
	GoodsIterator  goods_iter;
	PPObjGoods goods_obj;
	PPIDArray  id_ary;
	SString    name_buf, context_buf;
	SString    path;
	THROW(!rOutPath.Empty());
	(path = 0).Cat(rOutPath).SetLastSlash().Cat(GOODS_RAND_SET_FILE_NAME);
	xmlTextWriterPtr writer = xmlNewTextWriterFilename(path, 0);
	if(writer) {
		THROW(goods_tbl.search(0, spLast));
		last_id = goods_tbl.data.ID;
		THROW(goods_iter.Init(0));
		iter_total = goods_iter.GetIterCounter().GetTotal();
		set_size = (iter_total > setSz) ? setSz : iter_total;
		if(iter_total) {
			set_size = (iter_total > setSz) ? setSz : iter_total;
			while(id_ary.getCount() < set_size) {
				PPID id = GetRandom(1, last_id);
				if(goods_obj.Fetch(id, &goods_rec) > 0) {
					if(goods_rec.Kind == PPGDSK_GOODS)
						id_ary.addUnique(goods_rec.ID);
				}
			}
			id_ary.sort();
			xmlTextWriterSetIndent(writer, 1);
			xmlTextWriterSetIndentString(writer, (const xmlChar*)"\t");
			xmlTextWriterStartDocument(writer, 0, "utf-8", 0);
			xmlTextWriterStartElement(writer, (name_buf = 0).Cat("RandomGoodsIndex").ucptr());
			for(uint i = 0, n = id_ary.getCount(); i < n; i++) {
				PPID id = id_ary.get(i);
				xmlTextWriterWriteElement(writer, (name_buf = 0).Cat("Id").ucptr(), (context_buf = 0).Cat(id).ucptr());
			}
			xmlTextWriterEndElement(writer);
		}
	}
	CATCHZOK
	xmlFreeTextWriter(writer);
	return ok;
}

int UhttIndexChangedGoods(SString & rOutPath)
{
	int     ok = 1;
	PPViewGoods   view;
	GoodsViewItem item;
	GoodsFilt     filt;
	PPIDArray     id_ary;
	SString       name_buf, context_buf;
	SString       path;
	THROW(!rOutPath.Empty());
	(path = 0).Cat(rOutPath).SetLastSlash().Cat(GOODS_CHANGED_SET_FILE_NAME);
	xmlTextWriterPtr writer = xmlNewTextWriterFilename(path, 0);
	if(writer) {
		filt.P_SjF = new SysJournalFilt;
		THROW_MEM(filt.P_SjF);
		filt.P_SjF->ObjType = PPOBJ_GOODS;
		filt.P_SjF->BegTm = 0;
		filt.P_SjF->Period.low = getcurdate_();
		filt.P_SjF->ActionIDList.add(PPACN_OBJUPD);
		THROW(view.Init_(&filt));
		THROW(view.InitIteration());
		while(view.NextIteration(&item) > 0) {
			id_ary.add(item.ID);
		}
		id_ary.sort();
		xmlTextWriterSetIndent(writer, 1);
		xmlTextWriterSetIndentString(writer, (const xmlChar*)"\t");
		xmlTextWriterStartDocument(writer, 0, "utf-8", 0);
		xmlTextWriterStartElement(writer, (name_buf = 0).Cat("ChangedGoodsIndex").ucptr());
		for(uint i = 0, n = id_ary.getCount(); i < n; i++) {
			PPID id = id_ary.get(i);
			xmlTextWriterWriteElement(writer, (name_buf = 0).Cat("Id").ucptr(), (context_buf = 0).Cat(id).ucptr());
		}
		xmlTextWriterEndElement(writer);
	}
	CATCHZOK
	ZDELETE(filt.P_SjF);
	xmlFreeTextWriter(writer);
	return ok;
}

int UhttIndexBrands(SString & rOutPath)
{
	int        ok = 1;
	int        goods_n = 0;
	PPBrand    brand_rec;
	PPObjBrand brand_obj;
	PPObjGoods goods_obj;
	PPIDArray  id_list;
	PPIDArray  id_image_list;
	SString    name_buf, context_buf;
	SString    path;
	union {
		Goods2Tbl::Key2 k2;
		Goods2Tbl::Key4 k4;
	} k_;
	THROW(!rOutPath.Empty());
	(path = 0).Cat(rOutPath).SetLastSlash().Cat(BRANDS_INDEX_FILE_NAME);
	xmlTextWriterPtr writer = xmlNewTextWriterFilename(path, 0);
	if(writer) {
		Goods2Tbl * p_tbl = goods_obj.P_Tbl;
		BExtQuery q(p_tbl, 2);
		DBQ * dbq = &(p_tbl->Kind == PPGDSK_BRAND);
		q.select(p_tbl->ID, 0L).where(*dbq);
		MEMSZERO(k_);
		k_.k2.Kind = PPGDSK_BRAND;
		for(q.initIteration(0, &k_, spGe); q.nextIteration() > 0;) {
			const PPID brand_id = goods_obj.P_Tbl->data.ID;
			if(brand_obj.Fetch(brand_id, &brand_rec) > 0) {
				assert(brand_id == brand_rec.ID);
				id_list.add(brand_id);
				{
					ObjLinkFiles lf(PPOBJ_BRAND);
					if(lf.Load(brand_id, 0L) > 0 && lf.At(0, name_buf))
						id_image_list.add(brand_id);
				}
			}
		}
		// @v8.4.0 id_list.sort();
		id_list.shuffle(); // @v8.4.0
		// @v8.4.0 id_image_list.sort();
		id_image_list.shuffle(); // @v8.4.0
		xmlTextWriterSetIndent(writer, 1);
		xmlTextWriterSetIndentString(writer, (const xmlChar*)"\t");
		xmlTextWriterStartDocument(writer, 0, "utf-8", 0);
		xmlTextWriterStartElement(writer, (name_buf = 0).Cat("BrandIndexRoot").ucptr());
		{
			{
				xmlTextWriterStartElement(writer, (name_buf = 0).Cat("BrandsIndex").ucptr());
				for(uint i = 0, n = id_list.getCount(); i < n; i++) {
					PPID id = id_list.get(i);
					xmlTextWriterWriteElement(writer, (name_buf = 0).Cat("Id").ucptr(), (context_buf = 0).Cat(id).ucptr());
				}
				xmlTextWriterEndElement(writer);
			}
			if(id_image_list.getCount()) {
				xmlTextWriterStartElement(writer, (name_buf = 0).Cat("BrandImageIndex").ucptr());
				for(uint i = 0, n = id_image_list.getCount(); i < n; i++) {
					PPID id = id_image_list.get(i);
					xmlTextWriterWriteElement(writer, (name_buf = 0).Cat("Id").ucptr(), (context_buf = 0).Cat(id).ucptr());
				}
				xmlTextWriterEndElement(writer);
			}
		}
		xmlTextWriterEndElement(writer);
	}
	CATCHZOK
	xmlFreeTextWriter(writer);
	return ok;
}

int UhttIndexStores(SString & rOutPath)
{
	int    ok = 1;
	int    foreign = 0;
	PPID   tag_id = 0;
	PPObjUhttStore uhs_obj;
	PPUhttStore    uhs_rec;
	PPIDArray id_list;
	SString name_buf, context_buf;
	SString path;
	THROW(!rOutPath.Empty());
	(path = 0).Cat(rOutPath).SetLastSlash().Cat(STORES_INDEX_FILE_NAME);
	xmlTextWriterPtr writer = xmlNewTextWriterFilename(path, 0);
	if(writer) {
		for(SEnum en = uhs_obj.ref->Enum(PPOBJ_UHTTSTORE, 0); en.Next(&uhs_rec) > 0;) {
			id_list.add(uhs_rec.ID);
		}
		id_list.sort();
		xmlTextWriterSetIndent(writer, 1);
		xmlTextWriterSetIndentString(writer, (const xmlChar*)"\t");
		xmlTextWriterStartDocument(writer, 0, "utf-8", 0);
		xmlTextWriterStartElement(writer, (name_buf = 0).Cat("StoresIndex").ucptr());
		for(uint i = 0, n = id_list.getCount(); i < n; i++) {
			const PPID id = id_list.get(i);
			xmlTextWriterWriteElement(writer, (name_buf = 0).Cat("Id").ucptr(), (context_buf = 0).Cat(id).ucptr());
		}
		xmlTextWriterEndElement(writer);
	}
	CATCHZOK
	xmlFreeTextWriter(writer);
	return ok;
}

#undef GOODS_INDEX_FILE_NAME
#undef GOODS_RAND_SET_FILE_NAME
#undef GOODS_CHANGED_SET_FILE_NAME
#undef BRANDS_INDEX_FILE_NAME
#undef STORES_INDEX_FILE_NAME

//
//
//
int SLAPI PPObjGoods::ProcessName(ProcessNameBlock & rBlk, SString & rResult)
{
	int    ok = -1;
	SString temp_buf;
	temp_buf = rBlk.OrgName;
	temp_buf.ReplaceChar('\t', ' ');
	temp_buf.ReplaceChar('\xD', ' ');
	temp_buf.ReplaceChar('\xA', ' ');
	while(temp_buf.ReplaceStrR("  ", " ", 0))
		;
	temp_buf.Strip();
	while(temp_buf.Last() == '*')
		temp_buf.TrimRight();
	temp_buf.Strip().Space();
	rResult = temp_buf;
	return ok;
}
//
//
//
/*

	class=tire; gcdimx=155; gcdimy=70; gcdimz=13;
	tag(priceseg)=premium

	list := item | list ';' item
	item := key '=' value
	key := ident | "tag" '(' ident ')'
	value := string | ident | number
	string := '"' [.]* '"'
	ident := [A-Za-z][A-Za-z0-9_]*

*/

int SLAPI PPObjGoods::SetupAttrByTextDescr(PPID id, const char * pLine, int use_ta)
{
	int    ok = -1;
	int    updated = 0;
	int    gcext_updated = 0;
	SString temp_buf, attr;
	PPObjTag tag_obj;
	PPObjectTag tag_rec;
	PPObjGoodsClass gc_obj;
	PPObjUnit u_obj;
	PPObjGoodsGroup gg_obj;
	PPObjBrand br_obj;
	PPGoodsPacket pack;
	THROW(GetPacket(id, &pack, 0) > 0);
	{
		SStrScan scan(pLine);
		while(scan.Skip()[0] != 0) {
			PPID   tag_id = 0;
			attr = 0;
			THROW_PP_S(scan.GetIdent(temp_buf), PPERR_ATTRBYTEXT_SYNTAX, pLine);
			scan.Skip();
			if(temp_buf.CmpNC("tag") == 0) {
				THROW_PP_S(scan[0] == '(', PPERR_ATTRBYTEXT_SYNTAX, pLine);
				scan.Incr();
				THROW_PP_S(scan.SearchChar(')'), PPERR_ATTRBYTEXT_SYNTAX, pLine);
				scan.Get(temp_buf);
				scan.IncrLen(1);
				scan.Skip();
				THROW(tag_obj.SearchBySymb(temp_buf.Strip(), &tag_id, 0) > 0);
				THROW(tag_obj.Fetch(tag_id, &tag_rec) > 0);
				THROW(tag_rec.ObjTypeID == PPOBJ_GOODS);
			}
			else
				(attr = temp_buf).ToLower();
			THROW_PP_S(scan[0] == '=', PPERR_ATTRBYTEXT_SYNTAX, pLine);
			scan.Incr();
			scan.Skip();
			if(scan.GetQuotedString(temp_buf)) {
			}
			else if(scan.SearchChar(';')) {
				scan.Get(temp_buf);
				scan.IncrLen(1);
			}
			else {
				for(temp_buf = 0; scan[0]; scan.Incr()) {
					temp_buf.CatChar(scan[0]);
				}
			}
			if(tag_id) {
				THROW(pack.TagL.PutItemStr(tag_id, temp_buf));
				updated = 1;
			}
			else {
				int    cls_prop = 0;
				if(attr == "class") {
					PPID   cls_id = 0;
					THROW(gc_obj.SearchBySymb(temp_buf.Strip(), &cls_id, 0) > 0);
					THROW_PP(oneof2(pack.Rec.GdsClsID, 0, cls_id), PPERR_UNALLDCHG_GOODSCLASS);
					if(pack.Rec.GdsClsID != cls_id) {
						pack.Rec.GdsClsID = cls_id;
						updated = 1;
					}
				}
				else if(attr == "gckind")
					cls_prop = PPGdsCls::eKind;
				else if(attr == "gcgrade")
					cls_prop = PPGdsCls::eGrade;
				else if(attr == "gcaddprop")
					cls_prop = PPGdsCls::eAdd;
				else if(attr == "gcaddprop2")
					cls_prop = PPGdsCls::eAdd2;
				else if(attr == "gcdimx")
					cls_prop = PPGdsCls::eX;
				else if(attr == "gcdimy")
					cls_prop = PPGdsCls::eY;
				else if(attr == "gcdimz")
					cls_prop = PPGdsCls::eZ;
				else if(attr == "gcdimw")
					cls_prop = PPGdsCls::eW;
				else if(attr == "unit") {
					PPID   unit_id = 0;
					THROW(u_obj.SearchBySymb(temp_buf.Strip(), &unit_id, 0) > 0);
					if(pack.Rec.UnitID != unit_id) {
						pack.Rec.UnitID = unit_id;
						updated = 1;
					}
				}
				else if(attr == "phunit") {
					PPID   unit_id = 0;
					PPUnit2 unit_rec;
					THROW(u_obj.SearchBySymb(temp_buf.Strip(), &unit_id, 0) > 0);
					THROW(u_obj.Fetch(unit_id, &unit_rec) > 0);
					THROW_PP(unit_rec.Flags & PPUnit::Phisical, PPERR_PHISICALUNITNEEDED);
					if(pack.Rec.PhUnitID != unit_id) {
						pack.Rec.PhUnitID = unit_id;
						updated = 1;
					}
				}
				else if(attr == "phuperu") {
					const double phuperu = temp_buf.ToReal();
					THROW_PP_S(phuperu > 0.0, PPERR_ATTRBYTEXT_SYNTAX, pLine);
					if(pack.Rec.PhUPerU != phuperu) {
						pack.Rec.PhUPerU = phuperu;
						updated = 1;
					}
				}
				else if(attr == "group") {
					PPID   grp_id = 0;
					Goods2Tbl::Rec gg_rec;
					if(temp_buf.ToLong() > 0) {
						BarcodeTbl::Rec bc_rec;
						THROW(gg_obj.SearchCode(temp_buf, &bc_rec) > 0);
						grp_id = bc_rec.GoodsID;
					}
					else {
						THROW(gg_obj.SearchByName(temp_buf.ToOem(), &grp_id, &gg_rec) > 0);
					}
					THROW(gg_obj.Fetch(grp_id, &gg_rec) > 0);
					THROW(gg_rec.Kind == PPGDSK_GROUP && !(gg_rec.Flags & (GF_ALTGROUP|GF_FOLDER)));
					if(pack.Rec.ParentID != gg_rec.ID) {
						pack.Rec.ParentID = gg_rec.ID;
						updated = 1;
					}
				}
				else if(attr == "brand") {
					PPID   brand_id = 0;
					Goods2Tbl::Rec brand_rec;
					THROW(br_obj.SearchByName(temp_buf, &brand_id, &brand_rec) > 0);
					if(pack.Rec.BrandID != brand_id) {
						pack.Rec.BrandID = brand_id;
						updated = 1;
					}
				}
				else if(attr == "keyword") {
					if(tag_obj.Fetch(PPTAG_GOODS_KEYWORDS, &tag_rec) > 0) {
						const ObjTagItem * p_tag_item = pack.TagL.GetItem_ForceEmpty(PPTAG_GOODS_KEYWORDS);
						ObjTagItem upd_item;
						RVALUEPTR(upd_item, p_tag_item);
						int    r = 0;
						temp_buf.Transf(CTRANSF_OUTER_TO_INNER);
						StringSet ss(',', temp_buf);
						for(uint p = 0; ss.get(&p, temp_buf);) {
							if(upd_item.AddKeyword(PPTAG_GOODS_KEYWORDS, temp_buf.Strip()) > 0) {
								r = 1;
							}
						}
						if(r > 0) {
							THROW(pack.TagL.PutItem(PPTAG_GOODS_KEYWORDS, &upd_item));
							updated = 1;
						}
					}
				}
				if(cls_prop) {
					const double real_prop = temp_buf.ToReal();
					PPGdsClsPacket gc_pack;
					THROW(pack.Rec.GdsClsID != 0);
					THROW(gc_obj.Fetch(pack.Rec.GdsClsID, &gc_pack) > 0);
					switch(cls_prop) {
						case PPGdsCls::eX:
							if(real_prop != 0.0 && gc_pack.Rec.Flags & PPGdsCls::fUseDimX) {
								gc_pack.RealToExtDim(real_prop, cls_prop, &pack.ExtRec.X);
								gcext_updated = 1;
							}
							break;
						case PPGdsCls::eY:
							if(real_prop != 0.0 && gc_pack.Rec.Flags & PPGdsCls::fUseDimY) {
								gc_pack.RealToExtDim(real_prop, cls_prop, &pack.ExtRec.Y);
								gcext_updated = 1;
							}
							break;
						case PPGdsCls::eZ:
							if(real_prop != 0.0 && gc_pack.Rec.Flags & PPGdsCls::fUseDimZ) {
								gc_pack.RealToExtDim(real_prop, cls_prop, &pack.ExtRec.Z);
								gcext_updated = 1;
							}
							break;
						case PPGdsCls::eW:
							if(real_prop != 0.0 && gc_pack.Rec.Flags & PPGdsCls::fUseDimW) {
								gc_pack.RealToExtDim(real_prop, cls_prop, &pack.ExtRec.W);
								gcext_updated = 1;
							}
							break;
						case PPGdsCls::eKind:
							if(!!temp_buf && gc_pack.Rec.Flags & PPGdsCls::fUsePropKind) {
								THROW(gc_pack.PropNameToID(cls_prop, temp_buf, &pack.ExtRec.KindID, 1, 0));
								gcext_updated = 1;
							}
							break;
						case PPGdsCls::eGrade:
							if(!!temp_buf && gc_pack.Rec.Flags & PPGdsCls::fUsePropGrade) {
								THROW(gc_pack.PropNameToID(cls_prop, temp_buf, &pack.ExtRec.GradeID, 1, 0));
								gcext_updated = 1;
							}
							break;
						case PPGdsCls::eAdd:
							if(!!temp_buf && gc_pack.Rec.Flags & PPGdsCls::fUsePropAdd) {
								THROW(gc_pack.PropNameToID(cls_prop, temp_buf, &pack.ExtRec.AddObjID, 1, 0));
								gcext_updated = 1;
							}
							break;
						case PPGdsCls::eAdd2:
							if(!!temp_buf && gc_pack.Rec.Flags & PPGdsCls::fUsePropAdd) {
								THROW(gc_pack.PropNameToID(cls_prop, temp_buf, &pack.ExtRec.AddObj2ID, 1, 0));
								gcext_updated = 1;
							}
							break;
					}

				}
			}
		}
		if(gcext_updated) {
			PPGdsClsPacket gc_pack;
			THROW(gc_obj.Fetch(pack.Rec.GdsClsID, &gc_pack) > 0);
			THROW(gc_pack.CompleteGoodsPacket(&pack));
		}
		if(updated || gcext_updated) {
			THROW(PutPacket(&id, &pack, use_ta));
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

