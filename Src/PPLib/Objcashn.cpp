// OBJCASHN.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017
// @codepage windows-1251
//
#include <pp.h>
#pragma hdrstop

SLAPI PPGenCashNode::PPGenCashNode()
{
	ID = 0;
	memzero(Name, sizeof(Name));
	memzero(Symb, sizeof(Symb));
	CurRestBillID = 0;
	CashType = 0;
	DrvVerMajor = 0;
	DrvVerMinor = 0;
	DisRoundPrec = 0;
	AmtRoundPrec = 0;
	LocID = 0;
	ExtQuotID = 0;
	Flags = 0;
	ExtFlags = 0;
	GoodsLocAssocID = 0;
	ParentID = 0;
	GoodsGrpID = 0;
	P_DivGrpList = 0;
}

SLAPI PPGenCashNode::~PPGenCashNode()
{
	ZDELETE(P_DivGrpList);
}

PPGenCashNode & FASTCALL PPGenCashNode::operator = (const PPGenCashNode & src)
{
	ID = src.ID;
	STRNSCPY(Name, src.Name);
	STRNSCPY(Symb, src.Symb);
	CurRestBillID = src.CurRestBillID;
	CashType      = src.CashType;
	DrvVerMajor   = src.DrvVerMajor;
	DrvVerMinor   = src.DrvVerMinor;
	DisRoundPrec  = src.DisRoundPrec;
	AmtRoundPrec  = src.AmtRoundPrec;
	LocID         = src.LocID;
	ExtQuotID     = src.ExtQuotID;
	Flags         = src.Flags;
	ExtFlags      = src.ExtFlags;
	GoodsLocAssocID = src.GoodsLocAssocID;
	ParentID      = src.ParentID;
	P_DivGrpList  = src.P_DivGrpList ? new SArray(*src.P_DivGrpList) : 0;
	return *this;
}

int SLAPI PPGenCashNode::SetRoundParam(const RoundParam * pParam)
{
	int    ok = 1;
	DisRoundPrec = 0;
	AmtRoundPrec = 0;
	Flags &= ~(CASHF_DISROUNDUP | CASHF_DISROUNDDOWN);
	ExtFlags &= ~(CASHFX_ROUNDAMTUP | CASHFX_ROUNDAMTDOWN);
	if(pParam) {
		if(pParam->DisRoundPrec < 0.0 || pParam->DisRoundPrec > 50.0)
			ok = PPSetError(PPERR_INVROUNDPREC);
		else if(pParam->AmtRoundPrec < 0.0 || pParam->AmtRoundPrec > 50.0)
			ok = PPSetError(PPERR_INVROUNDPREC);
		else {
			if(pParam->DisRoundPrec != 0.0) {
				DisRoundPrec = (uint16)(pParam->DisRoundPrec * 100.0);
				if(pParam->DisRoundDir > 0)
					Flags |= CASHF_DISROUNDUP;
				else if(pParam->DisRoundDir < 0)
					Flags |= CASHF_DISROUNDDOWN;
			}
			if(pParam->AmtRoundPrec != 0.0) {
				AmtRoundPrec = (uint16)(pParam->AmtRoundPrec * 100.0);
				if(pParam->AmtRoundDir > 0)
					ExtFlags |= CASHFX_ROUNDAMTUP;
				else if(pParam->AmtRoundDir < 0)
					ExtFlags |= CASHFX_ROUNDAMTDOWN;
			}
		}
	}
	return ok;
}

int SLAPI PPGenCashNode::GetRoundParam(RoundParam * pParam) const
{
	int    ok = -1;
	if(pParam) {
		memzero(pParam, sizeof(*pParam));
		if(DisRoundPrec) {
			pParam->DisRoundPrec = fdiv100i((long)DisRoundPrec);
			pParam->DisRoundDir = (Flags & CASHF_DISROUNDUP) ? +1 : ((Flags & CASHF_DISROUNDDOWN) ? -1 : 0);
		}
		if(AmtRoundPrec) {
			pParam->AmtRoundPrec = fdiv100i((long)AmtRoundPrec);
			pParam->AmtRoundDir  = (ExtFlags & CASHFX_ROUNDAMTUP) ? +1 : ((ExtFlags & CASHFX_ROUNDAMTDOWN) ? -1 : 0);
		}
		ok = 1;
	}
	return ok;
}
//
//
//
SLAPI PPAsyncCashNode::PPAsyncCashNode() : PPGenCashNode()
{
}

int SLAPI PPAsyncCashNode::GetLogNumList(PPIDArray & rList) const
{
	int    ok = 1;
	rList.clear();
	// @v9.6.0 {
	StringSet ss;
	SString temp_buf;
	LogNumList.Tokenize(",", ss);
	for(uint sp = 0; ss.get(&sp, temp_buf);) {
		temp_buf.Strip();
		IntRange range;
        int r = temp_buf.ToIntRange(range, SString::torfDoubleDot|SString::torfHyphen);
        THROW_PP_S(r == temp_buf.Len(), PPERR_INVPOSNUMBERITEM, temp_buf);
		{
            SETMAX(range.low, 0);
            SETMAX(range.upp, 0);
            ExchangeToOrder(&range.low, &range.upp);
            for(long v = range.low; v <= range.upp; v++) {
                rList.add(v);
				THROW_PP_S(rList.getCount() <= 2000, PPERR_TOOLARGERANGE, temp_buf);
            }
        }
	}
	rList.sortAndUndup();
	// } @v9.6.0
	/*
	//SString log_num_list = LogNumList;
	if(log_num_list.NotEmptyS()) {
		//121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,143,144,145,146,147,148,149,150,151,152,153,154,155,141,142
		char   tempbuf[512]; // @v9.6.0 @fix [128]-->[512]
		char * p;
		long   n, sDELIM = 0x0000202CL; // ", "
		STRNSCPY(tempbuf, log_num_list);
		if((p = strtok(tempbuf, (char*)&sDELIM)) != 0) {
			do {
				if((n = atol(p)) > 0)
					rList.add(n);
			} while((p = strtok(0, (char*)&sDELIM)) != 0);
		}
	}
	*/
	ok = rList.getCount() ? 1 : -1;
	CATCH
		rList.clear();
		ok = 0;
	ENDCATCH
	return ok;
}

struct __PPExtDevices {     // @persistent
	PPID   Tag;             // Const = PPOBJ_CASHNODE
	PPID   CashNodeID;      // ИД кассового узла
	PPID   Prop;            // Const = CNPRP_EXTDEVICES
	PPID   TouchScreenID;   // ИД TouchScreen
	PPID   ExtCashNodeID;   // ИД дополнительного кассового узла @todo{Перенести поле в PPCashNode2}
	long   CustDispType;    // Тип дисплея покупателя //
	char   CustDispPort[8]; // Порт дисплея покупателя (COM)
	uint16 CustDispFlags;	// flXXX

	PPID   PapyrusNodeID;   // ИД кассового узла Папирус
	PPID   ScaleID;         // ИД весов
	int16  ClearCDYTimeout; // Таймаут очистки дисплея покупателя после печати чека
	int16  EgaisMode;       // @v9.0.9 Режим работы с УТМ ЕГАИС. 0 - не использовать, 1 - использовать, 2 - тестовый режим
	PPID   PhnSvcID;        // 

	long   BnkTermType;		// Тип банковского терминала
	uint16 BnkTermLogNum;	// Логический номер банковского терминала
	uint16 BnkTermFlags;	// btfXXX
	char   BnkTermPort[8];	// Порт банковского терминала (COM)

	char   Reserve2[12];    // @reserve
	long   Reserve3[1];     // @reserve
	char   ExtStrBuf[758];  // @anchor
};

struct __PosNodeExt {       // @persistent
	PPID   Tag;             // Const = PPOBJ_CASHNODE
	PPID   CashNodeID;      // ИД кассового узла
	PPID   Prop;            // Const = CNPRP_EXTRA
	uint8  Reserve[60];     // @reserve
	uint16 ScfDaysPeriod;       // Параметр фильтрации отложенных чеков
	int16  ScfDlvrItemsShowTag; // Параметр фильтрации отложенных чеков
	uint16 BonusMaxPart;    // 
	uint16 Reserve3;        // @alignment
	int32  Reserve2;        // @reserve
};

SLAPI PPSyncCashNode::PPSyncCashNode() : PPGenCashNode()
{
	memzero(Port, sizeof(Port));
	DownBill  = 0;
	CurDate   = ZERODATE;
	CurSessID = 0;
	TouchScreenID = 0;
	ExtCashNodeID = 0;
	PapyrusNodeID = 0;
	ScaleID       = 0;
	CustDispType  = 0;
	CustDispFlags = 0;
	memzero(CustDispPort, sizeof(CustDispPort));
	BnkTermType	  = 0;
	BnkTermLogNum = 0;
	memzero(BnkTermPort, sizeof(BnkTermPort));
	BnkTermFlags = 0;
	ClearCDYTimeout = 0;
	EgaisMode = 0; // @v9.0.9
	SleepTimeout  = 0;
	LocalTouchScrID = 0;
}

int SLAPI PPSyncCashNode::SetPropString(int propId, const char * pValue)
{
	return PPPutExtStrData(propId, ExtString, pValue);
}

int SLAPI PPSyncCashNode::GetPropString(int propId, SString & rBuf) const
{
	return PPGetExtStrData(propId, ExtString, rBuf);
}

SString & SLAPI PPSyncCashNode::CTblListToString(SString & rBuf) const
{
	rBuf = 0;
	LongArray temp_list = CTblList;
	const uint c = temp_list.getCount();
	if(c) {
		temp_list.sort();
		for(uint i = 0; i < c;) {
			rBuf.CatDiv(',', 2, 1).Cat(temp_list.get(i));
			uint j = i+1;
			//
			// Пропускаем одинаковые номера
			//
			while(j < c && temp_list.get(j) == temp_list.get(j-1)) {
				j++;
			}
			//
			//
			//
			i = j;
			uint   seq_count = 0;
			while(j < c && temp_list.get(j) == temp_list.get(j-1)+1) {
				seq_count++;
				j++;
			}
			if(seq_count > 1) {
				rBuf.CatCharN('.', 2).Cat(temp_list.get(j-1));
				i = j;
			}
		}
	}
	return rBuf;
}

int SLAPI PPSyncCashNode::CTblListFromString(const char * pBuf)
{
	int    ok = 1;
	LongArray temp_list;
	SString temp_buf;
	SStrScan scan(pBuf);
	scan.Skip();
	while(ok && scan.GetDigits(temp_buf)) {
		long   n = temp_buf.ToLong();
		if(n > 0) {
			scan.Skip();
			if(scan[0] == ',') {
				scan.Incr();
				scan.Skip();
				temp_list.addUnique(n);
			}
			else if(scan[0] == '.' && scan[1] == '.') {
				scan.Incr(2);
				scan.Skip();
				if(scan.GetDigits(temp_buf)) {
					long   n2 = temp_buf.ToLong();
					for(long i = n; i <= n2; i++) {
						temp_list.addUnique(i);
					}
					scan.Skip();
					if(scan[0] == ',') {
						scan.Incr();
						scan.Skip();
					}
					else if(scan[0] == 0) {
						break;
					}
					else {
						ok = PPSetError(PPERR_CTBLPARSEFAULT, pBuf);
					}
				}
			}
			else if(scan[0] == 0) {
				temp_list.addUnique(n);
				break;
			}
			else {
				ok = PPSetError(PPERR_CTBLPARSEFAULT, pBuf);
			}
		}
	}
	if(ok) {
		temp_list.sort();
		CTblList = temp_list;
	}
	return ok;
}
//
// Logic locking
//

// static
int SLAPI PPObjCashNode::IsExtCashNode(PPID nodeID, PPID * pParentID)
{
	int    is_ext_cash_node = 0;
	PPID   parent_id = 0;
	PPCashNode cn_rec;
	for(PPID cn_id = 0; !is_ext_cash_node && PPRef->EnumItems(PPOBJ_CASHNODE, &cn_id, &cn_rec) > 0;) {
		__PPExtDevices  ed;
		if(cn_rec.Flags & CASHF_SYNC && PPRef->GetProp(PPOBJ_CASHNODE, cn_id, CNPRP_EXTDEVICES, &ed, sizeof(ed)) > 0 && ed.ExtCashNodeID == nodeID) {
			parent_id = ed.CashNodeID;
			is_ext_cash_node = 1;
		}
	}
	ASSIGN_PTR(pParentID, parent_id);
	return is_ext_cash_node;
}

// static
int SLAPI PPObjCashNode::IsLocked(PPID id)
{
	int    ok = -1;
	PPID   parent_id = 0;
	PPSyncItem sync_item;
	PPSync & r_sync = DS.GetSync();
	if(!IsExtCashNode(id, &parent_id) && r_sync.CreateMutex(LConfig.SessionID, PPOBJ_CASHNODE, id, 0, &sync_item) > 0) {
		r_sync.ReleaseMutex(PPOBJ_CASHNODE, id);
		ok = 0;
	}
	else if(!r_sync.IsMyLock(LConfig.SessionID, PPOBJ_CASHNODE, id) &&
		(!parent_id || !r_sync.IsMyLock(LConfig.SessionID, PPOBJ_CASHNODE, parent_id)))
		ok = (PPSetError(PPERR_CASHNODE_LOCKED, sync_item.Name), 1);
	return ok;
}

// static
int SLAPI PPObjCashNode::Lock(PPID id)
{
	int    ok = -1;
	int    no_locking = 0;
	PPIniFile ini_file;
	PPSyncItem sync_item;
	ini_file.GetInt(PPINISECT_SYSTEM, PPINIPARAM_CASHNODE_NOLOCKING, &no_locking);
	if(!no_locking)
		ok = (DS.GetSync().CreateMutex(LConfig.SessionID, PPOBJ_CASHNODE, id, 0, &sync_item) > 0) ? 1 : PPSetError(PPERR_CASHNODE_LOCKED, sync_item.Name);
	return ok;
}

// static
int SLAPI PPObjCashNode::Unlock(PPID id)
{
	int    ok = 1;
	SString  added_msg_str = DS.GetTLA().AddedMsgString;
	ok = DS.GetSync().ReleaseMutex(PPOBJ_CASHNODE, id);
	PPSetAddedMsgString(added_msg_str);
	return ok;
}
//
//
//
StrAssocArray * SLAPI PPObjCashNode::MakeStrAssocList(void * extraPtr)
{
	SelFilt f;
	if(!extraPtr)
		MEMSZERO(f);
	else
		f = *(SelFilt *)extraPtr;
	StrAssocArray * p_ary = new StrAssocArray;
	THROW_MEM(p_ary);
	{
		PPIDArray parent_list;
		PPCashNode cn_rec;
		for(SEnum en = ref->Enum(Obj, 0); en.Next(&cn_rec) > 0;) {
			if((!f.LocID || cn_rec.LocID == f.LocID) && (!f.ParentID || cn_rec.ParentID == f.ParentID) &&
				(!f.OnlyGroups || (f.OnlyGroups == 1 && cn_rec.CashType == PPCMT_CASHNGROUP) || (f.OnlyGroups == -1 && cn_rec.CashType != PPCMT_CASHNGROUP)) &&
				(!(f.Flags & SelFilt::fSkipPassive) || !(cn_rec.ExtFlags & CASHFX_PASSIVE)) &&
				((f.Flags & SelFilt::fIgnoreRights) || ObjRts.CheckPosNodeID(cn_rec.ID, 0))) {
				if(cn_rec.CashType != PPCMT_CASHNGROUP && f.SyncGroup)
					if(f.SyncGroup == 1) {
						if(!PPCashMachine::IsSyncCMT(cn_rec.CashType))
							continue;
					}
					else if(f.SyncGroup == 2) {
						if(!PPCashMachine::IsAsyncCMT(cn_rec.CashType))
							continue;
					}
					else
						break;
				if(cn_rec.ParentID)
					parent_list.addUnique(cn_rec.ParentID);
				THROW_SL(p_ary->Add(cn_rec.ID, cn_rec.ParentID, cn_rec.Name));
			}
		}
		{
			SString name_buf;
			for(uint i = 0; i < parent_list.getCount(); i++) {
				const PPID parent_id = parent_list.get(i);
				if(!p_ary->Search(parent_id, 0)) {
					PPCashNode cn_rec;
					if(Fetch(parent_id, &cn_rec) > 0)
						name_buf = cn_rec.Name;
					else
						ideqvalstr(parent_id, name_buf = 0);
					THROW_SL(p_ary->Add(parent_id, 0, name_buf));
				}
			}
		}
		p_ary->SortByText();
	}
	CATCH
		ZDELETE(p_ary);
	ENDCATCH
	return p_ary;
}

// static
PPID SLAPI PPObjCashNode::Select(PPID locID, int syncGroup, int * pIsSingle, int isAny)
{
	int    valid_data = 0;
	PPID   id = -1;
	ListWindow * lw = 0;
	StrAssocArray * p_ary = 0;
	SelFilt f;
	f.LocID = locID;
	f.SyncGroup = syncGroup;
	f.OnlyGroups = -1;
	f.Flags |= SelFilt::fSkipPassive;
	ASSIGN_PTR(pIsSingle, 0);
	PPObjCashNode cn_obj;
	THROW(p_ary = cn_obj.MakeStrAssocList(&f));
	if(isAny) {
		id = p_ary->getCount();
		ZDELETE(p_ary);
	}
	else if(p_ary->getCount() == 0) {
		id = -1;
		ZDELETE(p_ary);
	}
	else if(p_ary->getCount() == 1) {
		id = p_ary->at(0).Id;
		THROW_PP(id != PPCMT_OKA500, PPERR_OKA500NOTSUPPORTED);
		ASSIGN_PTR(pIsSingle, 1);
		ZDELETE(p_ary);
	}
	else {
		SString title;
		THROW(lw = new ListWindow(new StrAssocListBoxDef(p_ary, lbtDisposeData|lbtDblClkNotify), 0, 0));
		THROW(PPLoadText(PPTXT_SELECTCASHNODE, title));
		lw->setTitle(title);
		lw->options |= (ofCenterX | ofCenterY);
		while(!valid_data && ExecView(lw) == cmOK) {
			lw->getResult(&id);
			if(id == PPCMT_OKA500) {
				id = -1;
				PPError(PPERR_OKA500NOTSUPPORTED);
			}
			else
				valid_data = 1;
		}
	}
	CATCH
		id = PPErrorZ();
	ENDCATCH
	delete lw;
	return id;
}

//static
const int PPObjCashNode::SubstCTblID = 999; // Специализированный идентификатор стола, применяемый для замещения не определенного списка столов. =999

//static
int  SLAPI PPObjCashNode::GetCafeTableName(int ctblN, SString & rBuf)
{
	int    ok = 0;
	rBuf = 0;
	if(ctblN > 0) {
		SString symb;
		if(ctblN == SubstCTblID)
			symb.CatChar('#');
		else
			symb.Cat(ctblN);
		PPObjReference ctbl_obj(PPOBJ_CAFETABLE, 0);
		PPID   ctbl_id = 0;
		ReferenceTbl::Rec ctbl_rec;
		if(ctbl_obj.SearchBySymb(symb, &ctbl_id, &ctbl_rec) > 0) {
			rBuf = ctbl_rec.ObjName;
			ok = 1;
		}
		else {
			rBuf = symb;
			ok = 2;
		}
	}
	else
		ok = 0;
	return ok;
}

PPObjCashNode::SelFilt::SelFilt()
{
	LocID = 0;
	SyncGroup = 0;
	OnlyGroups = -1;
	ParentID = 0;
	Flags = 0;
}

SLAPI PPObjCashNode::PPObjCashNode(void * extraPtr) : PPObjReference(PPOBJ_CASHNODE, extraPtr)
{
	ImplementFlags |= (implStrAssocMakeList | implTreeSelector);
}

int SLAPI PPObjCashNode::DeleteObj(PPID id)
{
	const PPCashNode * p_cn_rec = (const PPCashNode *)&ref->data;
	if(!(p_cn_rec->Flags & CASHF_DAYCLOSED) && p_cn_rec->CurDate)
		return PPSetError(PPERR_DAYNOTCLOSED);
	else
		return PPObjReference::DeleteObj(id);
}

int  SLAPI PPObjCashNode::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx) // @srlz
{
	int    ok = 1;
	if(p && p->Data)
		if(stream == 0) {
			PPCashNode * p_rec = (PPCashNode *)p->Data;
			if(*pID == 0) {
				PPID   same_id = 0;
				if((p_rec->ID && p_rec->ID < PP_FIRSTUSRREF) ||
					ref->SearchSymb(Obj, &same_id, p_rec->Symb, offsetof(PPCashNode, Symb)) > 0) {
					PPCashNode same_rec;
					if(Search(same_id, &same_rec) > 0) {
						ASSIGN_PTR(pID, same_id);
					}
					else
						same_id = 0;
				}
				if(same_id == 0) {
					if(p_rec->ID >= PP_FIRSTUSRREF)
						p_rec->ID = 0;
					THROW(EditItem(Obj, *pID, p_rec, 1));
					ASSIGN_PTR(pID, ref->data.ObjID);
				}
				else {
					; // Не изменяем существующий в разделе объект
				}
			}
			else {
				; // Не изменяем существующий в разделе объект
			}
		}
		else {
			THROW(Serialize_(+1, (ReferenceTbl::Rec *)p->Data, stream, pCtx));
		}
	CATCHZOK
	return ok;
}

int SLAPI PPObjCashNode::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	int    ok = -1;
	if(p && p->Data) {
		PPCashNode * p_rec = (PPCashNode *)p->Data; // PPCashNode2
		ProcessObjRefInArray(PPOBJ_LOCATION, &p_rec->LocID, ary, replace);
		ProcessObjRefInArray(PPOBJ_QUOTKIND, &p_rec->ExtQuotID, ary, replace);
		ProcessObjRefInArray(PPOBJ_GOODSGROUP, &p_rec->GoodsGrpID, ary, replace);
		ProcessObjRefInArray(PPOBJ_CASHNODE, &p_rec->ParentID, ary, replace);
		ok = 1;
	}
	return ok;
}

int SLAPI PPObjCashNode::Get(PPID id, PPGenCashNode * pGCN, PPCashNode * pCN)
{
	int    r;
	PPCashNode cn_rec;
	if((r = ref->GetItem(PPOBJ_CASHNODE, id, &cn_rec)) > 0) {
		pGCN->ID          = cn_rec.ID;
		pGCN->CashType    = cn_rec.CashType;
		pGCN->CurRestBillID = cn_rec.CurRestBillID;
		pGCN->DrvVerMajor = cn_rec.DrvVerMajor;
		pGCN->DrvVerMinor = cn_rec.DrvVerMinor;
		pGCN->DisRoundPrec  = cn_rec.DisRoundPrec;
		pGCN->AmtRoundPrec  = cn_rec.AmtRoundPrec;
		pGCN->LocID       = cn_rec.LocID;
		pGCN->ExtQuotID   = cn_rec.ExtQuotID;
		pGCN->Flags       = cn_rec.Flags;
		pGCN->ExtFlags    = cn_rec.ExtFlags;
		pGCN->GoodsLocAssocID = cn_rec.GoodsLocAssocID;
		STRNSCPY(pGCN->Name, cn_rec.Name);
		STRNSCPY(pGCN->Symb, cn_rec.Symb);
		pGCN->ParentID = cn_rec.ParentID;
		pGCN->GoodsGrpID = cn_rec.GoodsGrpID;
		//
		ZDELETE(pGCN->P_DivGrpList);
		SArray temp_list(sizeof(PPGenCashNode::DivGrpAssc));
		if(ref->GetPropArray(Obj, id, CNPRP_DIVGRPASSC, &temp_list) > 0 && temp_list.getCount())
			pGCN->P_DivGrpList = new SArray(temp_list);
		if(pCN)
			memcpy(pCN, &cn_rec, sizeof(PPCashNode));
		r = 1;
	}
	return r;
}

int SLAPI PPObjCashNode::GetSync(PPID id, PPSyncCashNode * pSCN)
{
	int    ok = -1;
	PPCashNode cn_rec;
	__PPExtDevices * p_ed = 0;
	if(Get(id, pSCN, &cn_rec) > 0) {
		SString temp_buf;
		size_t ed_size = 0;
		pSCN->SleepTimeout = cn_rec.SleepTimeout;
		pSCN->DownBill   = cn_rec.DownBill;
		pSCN->CurDate    = cn_rec.CurDate;
		pSCN->CurSessID  = cn_rec.CurSessID;
		pSCN->Speciality = cn_rec.Speciality;
		memcpy(pSCN->Port,  cn_rec.Port,  sizeof(pSCN->Port));
		if(ref->GetPropActualSize(Obj, id, CNPRP_EXTDEVICES, &ed_size) > 0) {
			THROW_MEM(p_ed = (__PPExtDevices *)malloc(ed_size));
			memzero(p_ed, ed_size);
			if(ref->GetProp(Obj, id, CNPRP_EXTDEVICES, p_ed, ed_size) > 0) {
				pSCN->TouchScreenID = p_ed->TouchScreenID;
				pSCN->ExtCashNodeID = p_ed->ExtCashNodeID;
				pSCN->PapyrusNodeID = p_ed->PapyrusNodeID;
				pSCN->ScaleID       = p_ed->ScaleID;
				pSCN->CustDispType  = p_ed->CustDispType;
				STRNSCPY(pSCN->CustDispPort, p_ed->CustDispPort);
				pSCN->CustDispFlags = p_ed->CustDispFlags;
				pSCN->ClearCDYTimeout = p_ed->ClearCDYTimeout;
				if(pSCN->ClearCDYTimeout < 0 || pSCN->ClearCDYTimeout > 60)
					pSCN->ClearCDYTimeout = 0;
				// @v9.0.9 {
				pSCN->EgaisMode    = p_ed->EgaisMode;
				if(!oneof3(pSCN->EgaisMode, 0, 1, 2))
					pSCN->EgaisMode = 0;
				// } @v9.0.9
				pSCN->BnkTermType  = p_ed->BnkTermType;
				pSCN->BnkTermLogNum = p_ed->BnkTermLogNum;
				STRNSCPY(pSCN->BnkTermPort, p_ed->BnkTermPort);
				pSCN->BnkTermFlags = p_ed->BnkTermFlags;
				pSCN->PhnSvcID  = p_ed->PhnSvcID;
				pSCN->ExtString = p_ed->ExtStrBuf;
				pSCN->GetPropString(SCN_PRINTERPORT,        pSCN->PrinterPort);
				pSCN->GetPropString(SCN_CAFETABLE_DGR_PATH, pSCN->TableSelWhatman);
				pSCN->GetPropString(SCN_BNKTERMPATH,        pSCN->BnkTermPath);
				pSCN->GetPropString(SCN_SLIPFMTPATH,        pSCN->SlipFmtPath);
			}
		}
		else {
			pSCN->TouchScreenID   = 0;
			pSCN->ExtCashNodeID   = 0;
			pSCN->PapyrusNodeID   = 0;
			pSCN->ScaleID         = 0;
			pSCN->CustDispType    = 0;
			pSCN->CustDispPort[0] = 0;
			pSCN->CustDispFlags	  = 0;
			pSCN->BnkTermType     = 0;
			pSCN->BnkTermLogNum   = 0;
			pSCN->BnkTermPort[0]  = 0;
			pSCN->BnkTermPath     = 0;
			pSCN->BnkTermFlags	  = 0;
			pSCN->ClearCDYTimeout = 0;
			pSCN->EgaisMode       = 0; // @v9.0.9
			pSCN->PrinterPort     = 0;
			pSCN->TableSelWhatman = 0;
			pSCN->BnkTermPath     = 0;
			pSCN->SlipFmtPath     = 0;
			pSCN->PhnSvcID        = 0;
			pSCN->ExtString       = 0;
		}
		{
			__PosNodeExt pnext;
			MEMSZERO(pnext);
			if(ref->GetProp(Obj, id, CNPRP_EXTRA, &pnext, sizeof(pnext)) > 0) {
				pSCN->Scf.DaysPeriod = pnext.ScfDaysPeriod;
				pSCN->Scf.DlvrItemsShowTag = pnext.ScfDlvrItemsShowTag;
				pSCN->BonusMaxPart = pnext.BonusMaxPart;
			}
			else {
				pSCN->Scf.DaysPeriod = 0;
				pSCN->Scf.DlvrItemsShowTag = 0;
				pSCN->BonusMaxPart = 0;
			}
		}
		pSCN->CTblList.clear();
		ref->GetPropArray(Obj, id, CNPRP_CTBLLIST, &pSCN->CTblList);
		pSCN->LocalTouchScrID = 0;
		{
			WinRegKey reg_key(HKEY_LOCAL_MACHINE, PPRegKeys::PrefSettings, 1); // @v9.2.0 readonly 0-->1
			SString param_buf;
			(param_buf = "LocalTouchScreen").CatChar(':').Cat(id);
			if(reg_key.GetString(param_buf, temp_buf)) {
				pSCN->LocalTouchScrID = temp_buf.ToLong();
				if(pSCN->LocalTouchScrID) {
					PPObjTouchScreen ts_obj;
					PPTouchScreen ts_rec;
					if(ts_obj.Search(pSCN->LocalTouchScrID, &ts_rec) <= 0)
						pSCN->LocalTouchScrID = 0;
				}
			}
		}
		ok = 1;
	}
	else
		ok = -1;
	CATCHZOK
	free(p_ed);
	return ok;
}

int SLAPI PPObjCashNode::GetAsync(PPID id, PPAsyncCashNode * pACN)
{
	int    ok = 1, r;
	PPCashNode cn_rec;
	pACN->ImpFiles = 0;
	pACN->ExpPaths = 0;
	pACN->LogNumList = 0;
	ZDELETE(pACN->P_DivGrpList);
	if((r = Get(id, pACN, &cn_rec)) > 0) {
		SString temp_buf;
		if(!(cn_rec.Flags & CASHF_EXTFRM349)) {
			THROW(r = PPRef->GetPropVlrString(PPOBJ_CASHNODE, id, CNPRP_IMPFILES, temp_buf));
			if(r > 0 && temp_buf.NotEmptyS())
				pACN->ImpFiles = temp_buf;
			THROW(r = PPRef->GetPropVlrString(PPOBJ_CASHNODE, id, CNPRP_EXPPATHS, temp_buf));
			if(r > 0 && temp_buf.NotEmptyS())
				pACN->ExpPaths = temp_buf;
		}
		else if(ref->GetPropVlrString(Obj, id, CNPRP_EXTSTR, temp_buf) > 0) {
			PPGetExtStrData(ACN_EXTSTR_FLD_IMPFILES, temp_buf, pACN->ImpFiles);
			PPGetExtStrData(ACN_EXTSTR_FLD_EXPPATHS, temp_buf, pACN->ExpPaths);
			PPGetExtStrData(ACN_EXTSTR_FLD_LOGNUMS,  temp_buf, pACN->LogNumList);
			PPGetExtStrData(ACN_EXTSTR_FLD_ADDEDMSGSIGN, temp_buf, pACN->AddedMsgSign);
		}
	}
	else
		ok = r;
	CATCHZOK
	return ok;
}

int SLAPI PPObjCashNode::GetListByLoc(PPID locID, PPIDArray & rList)
{
	int    ok = -1;
	PPCashNode cn_rec;
	for(SEnum en = ref->Enum(Obj, 0); en.Next(&cn_rec) > 0;) {
		if(cn_rec.LocID == locID) {
			rList.addUnique(cn_rec.ID);
			ok = 1;
		}
	}
	return ok;
}

int SLAPI PPObjCashNode::GetListByGroup(PPID grpID, PPIDArray & rList)
{
	int    ok = -1;
	PPCashNode cn_rec;
	for(SEnum en = ref->Enum(Obj, 0); en.Next(&cn_rec) > 0;) {
		if(cn_rec.ParentID == grpID) {
			rList.addUnique(cn_rec.ID);
			ok = 1;
		}
	}
	return ok;
}

int SLAPI PPObjCashNode::Helper_ResolveItem(PPID id, PPIDArray & rDestList, LAssocArray & rFullList)
{
	int    ok = -1;
	PPCashNode cn_rec;
	if(id && Fetch(id, &cn_rec) > 0) {
		if(!rDestList.lsearch(id)) {
			rDestList.add(id);
			ok = 1;
			if(cn_rec.CashType == PPCMT_CASHNGROUP) {
				PPIDArray inner_list;
				rFullList.GetListByKey(id, inner_list);
				for(uint i = 0; i < inner_list.getCount(); i++) {
					Helper_ResolveItem(inner_list.get(i), rDestList, rFullList); // @recursion
				}
			}
		}
	}
	return ok;
}

int SLAPI PPObjCashNode::ResolveList(const PPIDArray * pSrcList, PPIDArray & rDestList)
{
	int    ok = -1;
	rDestList.clear();
	if(pSrcList) {
		const uint _c = pSrcList->getCount();
		if(_c) {
			LAssocArray full_list;
			PPCashNode cn_rec;
			for(SEnum en = ref->Enum(Obj, 0); en.Next(&cn_rec) > 0;) {
				full_list.Add(cn_rec.ParentID, cn_rec.ID, 0);
			}
			for(uint i = 0; i < _c; i++) {
				if(Helper_ResolveItem(pSrcList->get(i), rDestList, full_list) > 0)
					ok = 1;
			}
		}
	}
	return ok;
}

int SLAPI PPObjCashNode::Put(PPID * pID, PPGenCashNode * pCN, int use_ta)
{
	int    ok = 1;
	int    is_new = 0;
	long   f = 0L, f1 = 0L;
	SString temp_buf;
	PPCashNode rec;
	__PPExtDevices * p_ed = 0;
	union {
		PPSyncCashNode  * p_scn;
		PPAsyncCashNode * p_acn;
	};
	p_scn = 0;
	THROW_PP(pCN != 0, PPERR_INVPARAM);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID) {
			THROW(ref->GetItem(PPOBJ_CASHNODE, *pID, &rec) > 0);
		}
		else {
			MEMSZERO(rec);
			is_new = 1;
		}
		if(!oneof2(pCN->CashType, PPCMT_CASHNGROUP, PPCMT_DISTRIB)) {
			f = pCN->Flags & (CASHF_SYNC | CASHF_ASYNC);
			THROW_PP_S(f && f != (CASHF_SYNC | CASHF_ASYNC), PPERR_CNUNDEFSYNC, rec.Name);
			f1 = rec.Flags & (CASHF_SYNC | CASHF_ASYNC);
			THROW_PP(!f1 || f1 == (CASHF_SYNC | CASHF_ASYNC) || f1 == f, PPERR_CNTOGGLESYNC);
		}
		STRNSCPY(rec.Name, pCN->Name);
		STRNSCPY(rec.Symb, pCN->Symb);
		rec.CashType    = pCN->CashType;
		rec.Flags       = (pCN->Flags | CASHF_EXTFRM349);
		rec.ExtFlags    = pCN->ExtFlags;
		rec.GoodsLocAssocID = pCN->GoodsLocAssocID;
		rec.LocID       = pCN->LocID;
		rec.CurRestBillID = pCN->CurRestBillID;
		rec.ExtQuotID   = pCN->ExtQuotID;
		rec.DrvVerMajor = pCN->DrvVerMajor;
		rec.DrvVerMinor = pCN->DrvVerMinor;
		rec.DisRoundPrec = pCN->DisRoundPrec;
		rec.AmtRoundPrec = pCN->AmtRoundPrec;
		rec.ParentID     = pCN->ParentID;
		rec.GoodsGrpID   = pCN->GoodsGrpID;
		if(f & CASHF_SYNC) {
			p_scn = (PPSyncCashNode *)pCN;
			rec.SleepTimeout = p_scn->SleepTimeout;
			rec.DownBill     = p_scn->DownBill;
			rec.CurDate      = p_scn->CurDate;
			rec.CurSessID    = p_scn->CurSessID;
			rec.Speciality   = p_scn->Speciality;
			memcpy(rec.Port,  p_scn->Port,  sizeof(rec.Port));
		}
		THROW(EditItem(PPOBJ_CASHNODE, *pID, &rec, 0));
		*pID = rec.ID;
		THROW(ref->PutPropArray(Obj, *pID, CNPRP_DIVGRPASSC, pCN->P_DivGrpList, 0));
		if(!oneof2(pCN->CashType, PPCMT_CASHNGROUP, PPCMT_DISTRIB)) {
			if(f & CASHF_SYNC) {
				p_scn = (PPSyncCashNode *)pCN;
				//p_scn->ExtString = p_ed->ExtStrBuf;
				p_scn->SetPropString(SCN_PRINTERPORT,        p_scn->PrinterPort);
				p_scn->SetPropString(SCN_CAFETABLE_DGR_PATH, p_scn->TableSelWhatman);
				p_scn->SetPropString(SCN_BNKTERMPATH,        p_scn->BnkTermPath);
				p_scn->SetPropString(SCN_SLIPFMTPATH,        p_scn->SlipFmtPath);
				{
					const size_t ed_size = ALIGNSIZE(offsetof(__PPExtDevices, ExtStrBuf) + p_scn->ExtString.Len() + 1, 2);
					THROW_MEM(p_ed = (__PPExtDevices *)malloc(ed_size));
					memzero(p_ed, ed_size);

					p_ed->TouchScreenID = p_scn->TouchScreenID;
					p_ed->ExtCashNodeID = p_scn->ExtCashNodeID;
					p_ed->PapyrusNodeID = p_scn->PapyrusNodeID;
					p_ed->ScaleID       = p_scn->ScaleID;
					p_ed->CustDispType  = p_scn->CustDispType;
					STRNSCPY(p_ed->CustDispPort, p_scn->CustDispPort);
					p_ed->CustDispFlags = p_scn->CustDispFlags;
					p_ed->ClearCDYTimeout = p_scn->ClearCDYTimeout;
					p_ed->EgaisMode     = p_scn->EgaisMode; // @v9.0.9
					p_ed->BnkTermType   = p_scn->BnkTermType;
					p_ed->BnkTermLogNum = p_scn->BnkTermLogNum;
					p_ed->BnkTermFlags = p_scn->BnkTermFlags;
					p_ed->PhnSvcID     = p_scn->PhnSvcID;
					STRNSCPY(p_ed->BnkTermPort, p_scn->BnkTermPort);
					p_scn->ExtString.CopyTo(p_ed->ExtStrBuf, 0); // Размер буфера p_ed точно отмерен для того, чтобы вместить p_scn->ExtString: см. выше
					if(p_ed->TouchScreenID || p_ed->ExtCashNodeID || p_ed->CustDispType || p_ed->BnkTermType ||
						p_ed->PapyrusNodeID || p_ed->ScaleID || p_ed->PhnSvcID || p_ed->ExtStrBuf[0] || p_ed->EgaisMode) {
						THROW(ref->PutProp(Obj, *pID, CNPRP_EXTDEVICES, p_ed, ed_size, 0));
					}
					else {
						THROW(ref->PutProp(Obj, *pID, CNPRP_EXTDEVICES, 0, ed_size, 0));
					}
				}
				{
					__PosNodeExt pnext;
					if(!p_scn->Scf.IsEmpty() || p_scn->BonusMaxPart) {
						MEMSZERO(pnext);
						pnext.ScfDaysPeriod = p_scn->Scf.DaysPeriod;
						pnext.ScfDlvrItemsShowTag = p_scn->Scf.DlvrItemsShowTag;
						pnext.BonusMaxPart = p_scn->BonusMaxPart;
						THROW(ref->PutProp(Obj, *pID, CNPRP_EXTRA, &pnext, sizeof(pnext)));
					}
					else {
						THROW(ref->PutProp(Obj, *pID, CNPRP_EXTRA, 0, sizeof(pnext)));
					}
				}
				THROW(ref->PutPropArray(Obj, *pID, CNPRP_CTBLLIST, &p_scn->CTblList, 0));
				{
					WinRegKey reg_key(HKEY_LOCAL_MACHINE, PPRegKeys::PrefSettings, 0);
					SString param_buf;
					(param_buf = "LocalTouchScreen").CatChar(':').Cat(*pID);
					if(reg_key.GetString(param_buf, temp_buf) && p_scn->LocalTouchScrID == 0) {
						(temp_buf = 0).Cat(0);
						reg_key.PutString(param_buf, temp_buf);
					}
					else if(p_scn->LocalTouchScrID) {
						(temp_buf = 0).Cat(p_scn->LocalTouchScrID);
						reg_key.PutString(param_buf, temp_buf);
					}
				}
			}
			else if(f & CASHF_ASYNC) {
				p_acn = (PPAsyncCashNode *)pCN;
				temp_buf = 0;
				PPPutExtStrData(ACN_EXTSTR_FLD_IMPFILES, temp_buf, p_acn->ImpFiles);
				PPPutExtStrData(ACN_EXTSTR_FLD_EXPPATHS, temp_buf, p_acn->ExpPaths);
				PPPutExtStrData(ACN_EXTSTR_FLD_LOGNUMS,  temp_buf, p_acn->LogNumList);
				PPPutExtStrData(ACN_EXTSTR_FLD_ADDEDMSGSIGN, temp_buf, p_acn->AddedMsgSign);
				THROW(ref->PutPropVlrString(Obj, *pID, CNPRP_EXTSTR, temp_buf));
			}
		}
		THROW(tra.Commit());
	}
	if(!is_new)
		Dirty(*pID);
	CATCHZOK
	free(p_ed);
	return ok;
}
//
//   DivGrpAsscListDialog
//
class DivGrpAsscListDialog : public PPListDialog {
public:
	DivGrpAsscListDialog() : PPListDialog(DLG_DIVGRPASSC, CTL_DIVGRPASSC_LIST)
	{
		P_Data = 0;
		PotentialyInvalid = 0;
	}
	~DivGrpAsscListDialog()
	{
		delete P_Data;
	}
	int    setDTS(const SArray * pData)
	{
		ZDELETE(P_Data);
		P_Data = new SArray(*pData);
		updateList(-1);
		return 1;
	}
	int    getDTS(SArray * pData)
	{
		if(P_Data && pData) {
			pData->copy(*P_Data);
			return 1;
		}
		else
			return 0;
	}
private:
	DECL_HANDLE_EVENT
	{
		PPListDialog::handleEvent(event);
		if(event.isKeyDown(kbCtrlX) && PPMaster) {
			if(P_Data && PotentialyInvalid) {
				uint c = P_Data->getCount();
				//if(c >= 4) {
					uint temp_c = c; //(c / 4) * 3;
					SArray temp_list(P_Data->dataPtr(), sizeof(int32)*2, temp_c, (O_ARRAY & ~aryDataOwner));
					SArray new_list(sizeof(PPGenCashNode::DivGrpAssc));
					for(uint i = 0; i < temp_c; i++) {
						const LAssoc * p_temp_item = (LAssoc *)temp_list.at(i);
						PPGenCashNode::DivGrpAssc new_item;
						new_item.GrpID = p_temp_item->Key;
						new_item.DivN = (short)p_temp_item->Val;
						new_list.insert(&new_item);
					}
					P_Data->copy(new_list);
					updateList(-1);
				//}
			}
			clearEvent(event);
		}
	}
	virtual int setupList();
	virtual int addItem(long * pPos, long * pID);
	virtual int editItem(long pos, long id);
	virtual int delItem(long pos, long id);
	int    editItem(int pos, PPGenCashNode::DivGrpAssc *);

	SArray * P_Data;
	int    PotentialyInvalid;
};

int DivGrpAsscListDialog::editItem(int pos, PPGenCashNode::DivGrpAssc * pItem)
{
	int    ok = -1;
	TDialog * dlg = new TDialog(DLG_DIVGRPASSCITM);
	PPGenCashNode::DivGrpAssc temp_item = *pItem;
	if(CheckDialogPtr(&dlg, 1)) {
		SetupPPObjCombo(dlg, CTLSEL_DIVGRPASSCITM_GRP, PPOBJ_GOODSGROUP, temp_item.GrpID, OLW_CANINSERT, 0);
		dlg->setCtrlData(CTL_DIVGRPASSCITM_DIV, &temp_item.DivN);
		for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
			int  all_ok = 1;
			dlg->getCtrlData(CTLSEL_DIVGRPASSCITM_GRP, &temp_item.GrpID);
			dlg->getCtrlData(CTL_DIVGRPASSCITM_DIV, &temp_item.DivN);
			PPGenCashNode::DivGrpAssc * p_item;
			for(uint i = 0; all_ok && P_Data->enumItems(&i, (void**)&p_item);)
				if(p_item->GrpID == temp_item.GrpID && (i-1) != (uint)pos)
					all_ok = PPErrorByDialog(dlg, CTLSEL_DIVGRPASSCITM_GRP, PPERR_DUPGOODSGROUP);
			if(all_ok) {
				*pItem = temp_item;
				ok = valid_data = 1;
			}
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

int DivGrpAsscListDialog::setupList()
{
	int    ok = 1;
	PPGenCashNode::DivGrpAssc * p_item;
	PPObjGoodsGroup gg_obj;
	for(uint i = 0; ok && P_Data->enumItems(&i, (void**)&p_item);) {
		Goods2Tbl::Rec gg_rec;
		char   sub[64];
		if(p_item->GrpID == 0) {
			sub[0] = '0';
			sub[1] = 0;
		}
		else if(gg_obj.Fetch(p_item->GrpID, &gg_rec) > 0)
			STRNSCPY(sub, gg_rec.Name);
		else {
			if(p_item->GrpID != 0)
				PotentialyInvalid = 1;
			ltoa(p_item->GrpID, sub, 10);
		}
		StringSet ss(SLBColumnDelim);
		ss.add(sub);
		ltoa(p_item->DivN, sub, 10);
		ss.add(sub);
		if(!addStringToList(p_item->GrpID, ss.getBuf()))
			ok = 0;
	}
	return ok;
}

int DivGrpAsscListDialog::addItem(long * pPos, long * pID)
{
	int  ok = -1;
	PPGenCashNode::DivGrpAssc item;
	MEMSZERO(item);
	if(editItem(-1, &item) > 0) {
		P_Data->insert(&item);
		ASSIGN_PTR(pPos, P_Data->getCount()-1);
		ASSIGN_PTR(pID, item.GrpID);
		ok = 1;
	}
	return ok;
}

int DivGrpAsscListDialog::editItem(long pos, long /*id*/)
{
	if(pos >= 0 && pos < (long)P_Data->getCount() &&
		editItem((int)pos, (PPGenCashNode::DivGrpAssc *)P_Data->at((uint)pos)) > 0)
		return 1;
	return -1;
}

int DivGrpAsscListDialog::delItem(long pos, long /*id*/)
{
	return P_Data->atFree((uint)pos) ? 1 : -1;
}
//
// ExtDevicesDialog
//
int SLAPI SelectPrinterFromWinPool(SString & rPrinter)
{
	int    ok = 1;
	long   sel_prn_id = 0, def_prn_id = 0;
	SString prn_port, temp_buf;
	TSArray <SPrinting::PrnInfo> prn_list;
	StrAssocArray * p_list = new StrAssocArray;
	ListWindow * p_lw = 0;

	THROW_MEM(p_list);
	SPrinting::GetListOfPrinters(&prn_list);
	p_list->Clear();
	//
	// Перемещаем принтер по умолчанию на верх списка
	//
	for(uint j = 0; j < prn_list.getCount(); j++) {
		if(prn_list.at(j).Flags & SPrinting::PrnInfo::fDefault) {
			def_prn_id = j+1;
			break;
		}
	}
	if(def_prn_id > 1)
		prn_list.swap(0, (uint)(def_prn_id-1));
	//
	for(uint j = 0; j < prn_list.getCount(); j++) {
		(temp_buf = 0).Cat(prn_list.at(j).PrinterName).ToOem();
		p_list->Add(j+1, temp_buf);
	}
	{
		int    valid_data = 0;
		SString title;
		THROW(p_lw = new ListWindow(new StrAssocListBoxDef(p_list, lbtDisposeData|lbtDblClkNotify), 0, 0));
		THROW(PPLoadText(PPTXT_SELECTPRINTER, title));
		p_lw->setTitle(title);
		p_lw->options |= (ofCenterX | ofCenterY);
		while(!valid_data && ExecView(p_lw) == cmOK) {
			p_lw->getResult(&sel_prn_id);
			if(sel_prn_id && sel_prn_id <= (long)prn_list.getCount())
				prn_port = prn_list.at(sel_prn_id - 1).PrinterName;
			valid_data = 1;
		}
	}
	CATCHZOK
	delete p_lw;
	rPrinter = prn_port;
	return ok;
}

static int EditExtDevices(PPSyncCashNode * pData)
{
	class ExtDevicesDialog : public TDialog {
	public:
		ExtDevicesDialog() : TDialog(DLG_EXTDEV)
		{
			PPSetupCtrlMenu(this, CTL_EXTDEV_PRINTER, CTLMNU_EXTDEV_PRINTER, CTRLMENU_SELPRINTER);
			PPSetupCtrlMenu(this, CTL_EXTDEV_RPTPRNPORT, CTLMNU_EXTDEV_RPTPRNPORT, CTRLMENU_SELPRINTER); // @v8.8.3
		}
		int    setDTS(const PPSyncCashNode * pData)
		{
			if(!RVALUEPTR(Data, pData))
				MEMSZERO(Data);
			SString temp_buf;
			SetupPPObjCombo(this, CTLSEL_EXTDEV_TCHSCREEN, PPOBJ_TOUCHSCREEN, Data.TouchScreenID, OLW_CANINSERT, 0);
			SetupPPObjCombo(this, CTLSEL_EXTDEV_LOCTCHSCR, PPOBJ_TOUCHSCREEN, Data.LocalTouchScrID, OLW_CANINSERT, 0);
			SetupPPObjCombo(this, CTLSEL_EXTDEV_CASHNODE,  PPOBJ_CASHNODE, Data.ExtCashNodeID, 0, 0);
			SetupPPObjCombo(this, CTLSEL_EXTDEV_PAPYRUS,   PPOBJ_CASHNODE, Data.PapyrusNodeID, 0, 0);
			SetupPPObjCombo(this, CTLSEL_EXTDEV_SCALE,     PPOBJ_SCALE, Data.ScaleID, 0, 0);
			SetupPPObjCombo(this, CTLSEL_EXTDEV_PHNSVC,    PPOBJ_PHONESERVICE, Data.PhnSvcID, 0, 0);
			setCtrlString(CTL_EXTDEV_PRINTER, Data.PrinterPort);
			// @v8.8.3 {
			Data.GetPropString(SCN_RPTPRNPORT, temp_buf);
			setCtrlString(CTL_EXTDEV_RPTPRNPORT, temp_buf);
			// } @v8.8.3
			Data.GetPropString(SCN_CASHDRAWER_PORT, temp_buf);
			setCtrlString(CTL_EXTDEV_DRAWERPORT, temp_buf);
			Data.GetPropString(SCN_CASHDRAWER_CMD, temp_buf);
			setCtrlString(CTL_EXTDEV_DRAWERCMD, temp_buf);

			Data.GetPropString(SCN_KITCHENBELL_PORT, temp_buf);
			setCtrlString(CTL_EXTDEV_KBELLPORT, temp_buf);
			Data.GetPropString(SCN_KITCHENBELL_CMD, temp_buf);
			setCtrlString(CTL_EXTDEV_KBELLCMD, temp_buf);
			// @v9.0.11 {
			Data.GetPropString(SCN_MANUFSERIAL, temp_buf);
			setCtrlString(CTL_EXTDEV_MANUFSERIAL, temp_buf);
			// } @v9.0.11
			// @v9.0.9 {
			AddClusterAssoc(CTL_EXTDEV_EGAISMODE,  0, 1);
			AddClusterAssoc(CTL_EXTDEV_EGAISMODE, -1, 0);
			AddClusterAssoc(CTL_EXTDEV_EGAISMODE,  1, 0);
			AddClusterAssoc(CTL_EXTDEV_EGAISMODE,  2, 2);
			SetClusterData(CTL_EXTDEV_EGAISMODE, Data.EgaisMode);
			// } @v9.0.9
			return 1;
		}
		int    getDTS(PPSyncCashNode * pData)
		{
			int    ok  = 1;
			uint   sel = 0;
			SString temp_buf;
			PPObjCashNode cn_obj;
			PPCashNode    cn_rec;
			getCtrlData(CTLSEL_EXTDEV_TCHSCREEN, &Data.TouchScreenID);
			getCtrlData(CTLSEL_EXTDEV_LOCTCHSCR, &Data.LocalTouchScrID);
			getCtrlData(CTLSEL_EXTDEV_CASHNODE,  &Data.ExtCashNodeID);
			getCtrlData(CTLSEL_EXTDEV_PAPYRUS,   &Data.PapyrusNodeID);
			getCtrlData(CTLSEL_EXTDEV_SCALE,     &Data.ScaleID);
			getCtrlData(CTLSEL_EXTDEV_PHNSVC,    &Data.PhnSvcID);
			getCtrlString(CTL_EXTDEV_PRINTER, Data.PrinterPort);
			// @v8.8.3 {
			getCtrlString(CTL_EXTDEV_RPTPRNPORT, temp_buf);
			Data.SetPropString(SCN_RPTPRNPORT, temp_buf);
			// } @v8.8.3
			if(Data.ExtCashNodeID) {
				THROW(cn_obj.Search(Data.ExtCashNodeID, &cn_rec) > 0);
				sel = CTL_EXTDEV_CASHNODE;
				THROW_PP(Data.ID != Data.ExtCashNodeID && PPCashMachine::IsSyncCMT(cn_rec.CashType), PPERR_INVCMT);
			}
			if(Data.PapyrusNodeID) {
				THROW(cn_obj.Search(Data.PapyrusNodeID, &cn_rec) > 0);
				sel = CTL_EXTDEV_PAPYRUS;
				THROW_PP(Data.ID != Data.PapyrusNodeID && cn_rec.CashType == PPCMT_PAPYRUS, PPERR_INVCMT);
			}

			getCtrlString(CTL_EXTDEV_DRAWERPORT, temp_buf);
			Data.SetPropString(SCN_CASHDRAWER_PORT, temp_buf);
			getCtrlString(CTL_EXTDEV_DRAWERCMD, temp_buf);
			Data.SetPropString(SCN_CASHDRAWER_CMD, temp_buf);

			getCtrlString(CTL_EXTDEV_KBELLPORT, temp_buf);
			Data.SetPropString(SCN_KITCHENBELL_PORT, temp_buf);
			getCtrlString(CTL_EXTDEV_KBELLCMD, temp_buf);
			Data.SetPropString(SCN_KITCHENBELL_CMD, temp_buf);
			// @v9.0.11 {
			getCtrlString(CTL_EXTDEV_MANUFSERIAL, temp_buf);
			Data.SetPropString(SCN_MANUFSERIAL, temp_buf);
			// } @v9.0.11
			Data.EgaisMode = (int16)GetClusterData(CTL_EXTDEV_EGAISMODE); // @v9.0.9
			ASSIGN_PTR(pData, Data);
			CATCH
				ok = PPErrorByDialog(this, sel, -1);
			ENDCATCH
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmCustDisp)) {
				EditCustDisp();
			}
			else if(event.isCmd(cmBnkTerminal)) {
				EditBnkTerm();
			}
			else if(TVKEYDOWN) {
				if(TVKEY == kbF2) {
					SString prn_port;
					if(SelectPrinterFromWinPool(prn_port) > 0) {
						if(isCurrCtlID(CTL_EXTDEV_PRINTER))
							setCtrlString(CTL_EXTDEV_PRINTER, prn_port);
						else if(isCurrCtlID(CTL_EXTDEV_RPTPRNPORT))
							setCtrlString(CTL_EXTDEV_RPTPRNPORT, prn_port);
					}
				}
				else
					return;
			}
			else
				return;
			clearEvent(event);
		}
		int    EditCustDisp()
		{
			class CustDispDialog : public TDialog {
			public:
				CustDispDialog() : TDialog(DLG_CUSTDISP)
				{
				}
				int    setDTS(const PPSyncCashNode * pData)
				{
					if(!RVALUEPTR(Data, pData))
						MEMSZERO(Data);
					SetupStringComboDevice(this, CTLSEL_CUSTDISP_DEVICE, DVCCLS_DISPLAY, Data.CustDispType, 0);
					setCtrlData(CTL_CUSTDISP_PORT, Data.CustDispPort);
					setCtrlUInt16(CTL_CUSTDISP_TIMEOUT, Data.ClearCDYTimeout);
					setCtrlData(CTL_CUSTDISP_USB, &Data.CustDispFlags);
					return 1;
				}
				int    getDTS(PPSyncCashNode * pData)
				{
					int    ok = 1;
					uint   sel = 0;
					getCtrlData(CTLSEL_CUSTDISP_DEVICE, &Data.CustDispType);
					if(Data.CustDispType) {
						sel = CTL_CUSTDISP_PORT;
						getCtrlData(CTL_CUSTDISP_PORT, Data.CustDispPort);
						getCtrlData(CTL_CUSTDISP_USB, &Data.CustDispFlags);
						if(Data.CustDispFlags != PPSyncCashNode::cdfUsb) {
							THROW_PP((*strip(Data.CustDispPort) != 0), PPERR_PORTNEEDED);
							THROW_PP(PPCustDisp::IsComPort(Data.CustDispPort), PPERR_CUSTDISP_INVPORT);
						}
						Data.ClearCDYTimeout = getCtrlUInt16(CTL_CUSTDISP_TIMEOUT);
					}
					ASSIGN_PTR(pData, Data);
					CATCH
						ok = PPErrorByDialog(this, sel, -1);
					ENDCATCH
					return ok;
				}
			private:
				PPSyncCashNode Data;
			};
			DIALOG_PROC_BODY(CustDispDialog, &Data);
		}
		int    EditBnkTerm()
		{
			class BnkTermDialog : public TDialog {
			public:
				BnkTermDialog() : TDialog(DLG_BNKTERM)
				{
					FileBrowseCtrlGroup::Setup(this, CTLBRW_BNKTERM_FILENAME, CTL_BNKTERM_PATH, 1, 0, 0, FileBrowseCtrlGroup::fbcgfFile);
				}
				int    setDTS(const PPSyncCashNode * pData)
				{
					if(!RVALUEPTR(Data, pData))
						MEMSZERO(Data);
					SetupStringComboDevice(this, CTLSEL_BNKTERM_TYPE, DVCCLS_BNKTERM, Data.BnkTermType, 0);
					setCtrlData(CTL_BNKTERM_LOGNUM, &Data.BnkTermLogNum);
					setCtrlData(CTL_BNKTERM_PORT, Data.BnkTermPort);
					setCtrlString(CTL_BNKTERM_PATH, Data.BnkTermPath);
					setCtrlData(CTL_BNKTERM_PINPAD, &Data.BnkTermFlags);
					return 1;
				}
				int    getDTS(PPSyncCashNode * pData)
				{
					int    ok = 1;
					uint   sel = 0;
					SString temp_buf;
					getCtrlData(CTLSEL_BNKTERM_TYPE, &Data.BnkTermType);
					sel = CTL_BNKTERM_PORT;
					getCtrlData(CTL_BNKTERM_LOGNUM, &Data.BnkTermLogNum);
					getCtrlData(CTL_BNKTERM_PORT, Data.BnkTermPort);
					getCtrlString(CTL_BNKTERM_PATH, Data.BnkTermPath);
					getCtrlData(CTL_BNKTERM_PINPAD, &Data.BnkTermFlags);
					ASSIGN_PTR(pData, Data);
					return ok;
				}
			private:
				PPSyncCashNode Data;
			};
			DIALOG_PROC_BODY(BnkTermDialog, &Data);
		}
		PPSyncCashNode Data;
	};
	DIALOG_PROC_BODY(ExtDevicesDialog, pData);
}

#define GRP_TBLDGMPATH 1

class SyncCashNodeCfgDialog : public TDialog {
public:
	SyncCashNodeCfgDialog() : TDialog(DLG_CASHNS)
	{
		SetupCalDate(CTLCAL_CASHN_CURDATE, CTL_CASHN_CURDATE);
		FileBrowseCtrlGroup::Setup(this, CTLBRW_CASHN_TBLDGMPATH, CTL_CASHN_TBLDGMPATH, GRP_TBLDGMPATH, 0, PPTXT_FILPAT_WTM,
			FileBrowseCtrlGroup::fbcgfFile);
		FileBrowseCtrlGroup::Setup(this, CTLBRW_CASHN_SLIPFMTPATH, CTL_CASHN_SLIPFMTPATH, GRP_TBLDGMPATH, 0, PPTXT_FILPAT_FMT,
			FileBrowseCtrlGroup::fbcgfFile);
	}
	int    setDTS(const PPSyncCashNode *);
	int    getDTS(PPSyncCashNode *);
private:
	DECL_HANDLE_EVENT;
	int    editDivGrpAssoc(SArray * pList)
	{
		DIALOG_PROC_BODY(DivGrpAsscListDialog, pList);
	}
	int    editRoundParam();
	int    editExt();
	void   SetupCtrls();
	int    EditCashParam();

	PPSyncCashNode Data;
};

int SyncCashNodeCfgDialog::editRoundParam()
{
	int    ok = -1;
	TDialog * dlg = new TDialog(DLG_CNROUND);
	if(CheckDialogPtr(&dlg, 1)) {
		PPGenCashNode::RoundParam param;
		Data.GetRoundParam(&param);
		dlg->setCtrlReal(CTL_CNROUND_DISPREC, param.DisRoundPrec);
		dlg->setCtrlReal(CTL_CNROUND_AMTPREC, param.AmtRoundPrec);
		ushort v = 0;
		if(param.DisRoundDir > 0)
			v = 2;
		else if(param.DisRoundDir < 0)
			v = 1;
		else
			v = 0;
		dlg->setCtrlUInt16(CTL_CNROUND_DISDIR, v);
		if(param.AmtRoundDir > 0)
			v = 2;
		else if(param.AmtRoundDir < 0)
			v = 1;
		else
			v = 0;
		dlg->setCtrlUInt16(CTL_CNROUND_AMTDIR, v);
		while(ok < 0 && ExecView(dlg) == cmOK) {
			param.DisRoundPrec = dlg->getCtrlReal(CTL_CNROUND_DISPREC);
			param.AmtRoundPrec = dlg->getCtrlReal(CTL_CNROUND_AMTPREC);
			v = dlg->getCtrlUInt16(CTL_CNROUND_DISDIR);
			if(v == 2)
				param.DisRoundDir = +1;
			else if(v == 1)
				param.DisRoundDir = -1;
			else
				param.DisRoundDir = 0;
			v = dlg->getCtrlUInt16(CTL_CNROUND_AMTDIR);
			if(v == 2)
				param.AmtRoundDir = +1;
			else if(v == 1)
				param.AmtRoundDir = -1;
			else
				param.AmtRoundDir = 0;
			if(Data.SetRoundParam(&param))
				ok = 1;
			else
				PPError();
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

int SyncCashNodeCfgDialog::editExt()
{
	int    ok = -1;
	TDialog * dlg = new TDialog(DLG_CASHNSEXT);
	if(CheckDialogPtr(&dlg, 1)) {
		SString ctbl_list_buf;
		long   dlvr_items_show_tag = 0; //DlvrItemsShowTag
		if(Data.Scf.DlvrItemsShowTag > 0)
			dlvr_items_show_tag = 1;
		else if(Data.Scf.DlvrItemsShowTag < 0)
			dlvr_items_show_tag = -1;
		dlg->setCtrlLong(CTL_CASHNSEXT_SCF_DAYS, (long)Data.Scf.DaysPeriod);
		dlg->AddClusterAssoc(CTL_CASHNSEXT_SCF_DLVRT, 0, 0);
		dlg->AddClusterAssoc(CTL_CASHNSEXT_SCF_DLVRT, -1, 0);
		dlg->AddClusterAssoc(CTL_CASHNSEXT_SCF_DLVRT, 1, +1);
		dlg->AddClusterAssoc(CTL_CASHNSEXT_SCF_DLVRT, 2, -1);
		dlg->SetClusterData(CTL_CASHNSEXT_SCF_DLVRT, dlvr_items_show_tag);
		dlg->setCtrlReal(CTL_CASHNSEXT_BONUSMAX, (double)Data.BonusMaxPart / 10.0);
		dlg->setCtrlString(CTL_CASHNSEXT_CTBLLIST, Data.CTblListToString(ctbl_list_buf));

		dlg->AddClusterAssoc(CTL_CASHNSEXT_FLAGS, 0, CASHFX_INPGUESTCFTBL); // @v8.0.12
		dlg->SetClusterData(CTL_CASHNSEXT_FLAGS, Data.ExtFlags); // @v8.0.12

		while(ok < 0 && ExecView(dlg) == cmOK) {
			long   days_period = dlg->getCtrlLong(CTL_CASHNSEXT_SCF_DAYS);
			if(days_period < 0 || days_period > 1000) {
				PPErrorByDialog(dlg, CTL_CASHNSEXT_SCF_DAYS, PPERR_USERINPUT);
			}
			else {
				dlg->getCtrlString(CTL_CASHNSEXT_CTBLLIST, ctbl_list_buf);
				if(!Data.CTblListFromString(ctbl_list_buf)) {
					PPErrorByDialog(dlg, CTL_CASHNSEXT_CTBLLIST, -1);
				}
				else {
					Data.BonusMaxPart = (uint16)(dlg->getCtrlReal(CTL_CASHNSEXT_BONUSMAX) * 10.0);
					if(Data.BonusMaxPart < 0 || Data.BonusMaxPart > 1000)
						Data.BonusMaxPart = 0;
					Data.Scf.DaysPeriod = (uint16)days_period;
					Data.Scf.DlvrItemsShowTag = (int16)dlg->GetClusterData(CTL_CASHNSEXT_SCF_DLVRT);
					dlg->GetClusterData(CTL_CASHNSEXT_FLAGS, &Data.ExtFlags); // @v8.0.12
					ok = 1;
				}
			}
		}
	}
	delete dlg;
	return ok;
}

int SyncCashNodeCfgDialog::setDTS(const PPSyncCashNode * pData)
{
	PPObjLocation loc_obj;
	if(!RVALUEPTR(Data, pData))
		MEMSZERO(Data);
	setCtrlData(CTL_CASHN_NAME, Data.Name);
	setCtrlData(CTL_CASHN_SYMB, Data.Symb);
	setCtrlLong(CTL_CASHN_ID, Data.ID);
	// @vmiller SetupStringCombo(this, CTLSEL_CASHN_DEVICE, PPTXT_CMT, Data.CashType, 0);
	SetupStringComboDevice(this, CTLSEL_CASHN_DEVICE, DVCCLS_SYNCPOS, Data.CashType, 0); // @vmiller
	SetupStringCombo(this, CTLSEL_CASHN_SPECIALITY, PPTXT_POSNODE_SPECIALITY, Data.Speciality);
	setCtrlData(CTL_CASHN_DRVVERMAJOR, &Data.DrvVerMajor);
	setCtrlData(CTL_CASHN_DRVVERMINOR, &Data.DrvVerMinor);
	setCtrlData(CTL_CASHN_PORT,    Data.Port);
	//setCtrlData(CTL_CASHN_OVRFLW,  &pSCN->OvrflwDate);
	setCtrlData(CTL_CASHN_CURDATE, &Data.CurDate);
	setCtrlData(CTL_CASHN_CSESS,   &Data.CurSessID);
	SETIFZ(Data.LocID, loc_obj.GetSingleWarehouse());
	SetupPPObjCombo(this, CTLSEL_CASHN_LOC, PPOBJ_LOCATION, Data.LocID, 0, 0);
	SetupPPObjCombo(this, CTLSEL_CASHN_NOA, PPOBJ_NAMEDOBJASSOC, Data.GoodsLocAssocID, OLW_CANINSERT, 0);
	AddClusterAssoc(CTL_CASHN_NOA_PRINTONLY, 0, CASHFX_GLASSOCPRINTONLY);
	SetClusterData(CTL_CASHN_NOA_PRINTONLY, Data.ExtFlags);
	DisableClusterItem(CTL_CASHN_NOA_PRINTONLY, 0, !Data.GoodsLocAssocID);

	AddClusterAssoc(CTL_CASHN_FLAGS,  0, CASHF_CHKPAN);
	AddClusterAssoc(CTL_CASHN_FLAGS,  1, CASHF_NAFCL);
	AddClusterAssoc(CTL_CASHN_FLAGS,  2, CASHF_OPENBOX);
	AddClusterAssoc(CTL_CASHN_FLAGS,  3, CASHF_ROUNDINT);
	AddClusterAssoc(CTL_CASHN_FLAGS,  4, CASHF_SELALLGOODS);
	AddClusterAssoc(CTL_CASHN_FLAGS,  5, CASHF_AUTO_PRINTCOPY);
	AddClusterAssoc(CTL_CASHN_FLAGS,  6, CASHF_USEQUOT);
	AddClusterAssoc(CTL_CASHN_FLAGS,  7, CASHF_NOASKPAYMTYPE);
	AddClusterAssoc(CTL_CASHN_FLAGS,  8, CASHF_SHOWREST);
	AddClusterAssoc(CTL_CASHN_FLAGS,  9, CASHF_KEYBOARDWKEY);
	AddClusterAssoc(CTL_CASHN_FLAGS, 10, CASHF_WORKWHENLOCK);
	AddClusterAssoc(CTL_CASHN_FLAGS, 11, CASHF_NOMODALCHECKVIEW);
	AddClusterAssoc(CTL_CASHN_FLAGS, 12, CASHF_DISABLEZEROAGENT);
	AddClusterAssoc(CTL_CASHN_FLAGS, 13, CASHF_SKIPUNPRINTEDCHECKS);
	AddClusterAssoc(CTL_CASHN_FLAGS, 14, CASHF_CHECKFORPRESENT);
	SetClusterData(CTL_CASHN_FLAGS, Data.Flags);
	{
		AddClusterAssoc(CTL_CASHN_EXTFLAGS,  0, 0x0001);
		AddClusterAssoc(CTL_CASHN_EXTFLAGS,  1, 0x0002);
		AddClusterAssoc(CTL_CASHN_EXTFLAGS,  2, 0x0004);
		AddClusterAssoc(CTL_CASHN_EXTFLAGS,  3, 0x0008);
		AddClusterAssoc(CTL_CASHN_EXTFLAGS,  4, 0x0010);
		AddClusterAssoc(CTL_CASHN_EXTFLAGS,  5, 0x0020);
		AddClusterAssoc(CTL_CASHN_EXTFLAGS,  6, 0x0040);
		AddClusterAssoc(CTL_CASHN_EXTFLAGS,  7, 0x0080);
		AddClusterAssoc(CTL_CASHN_EXTFLAGS,  8, 0x0100);
		AddClusterAssoc(CTL_CASHN_EXTFLAGS,  9, 0x0200);
		AddClusterAssoc(CTL_CASHN_EXTFLAGS, 10, 0x0400); // @v9.5.10

		long   ef = 0;
		SETFLAG(ef, 0x0001, Data.Flags & CASHF_UNIFYGDSATCHECK);
		SETFLAG(ef, 0x0002, Data.Flags & CASHF_UNIFYGDSTOPRINT);
		SETFLAG(ef, 0x0004, Data.Flags & CASHF_NOTUSECHECKCUTTER);
		SETFLAG(ef, 0x0008, Data.ExtFlags & CASHFX_SELSERIALBYGOODS);
		SETFLAG(ef, 0x0010, Data.ExtFlags & CASHFX_FORCEDIVISION);
		SETFLAG(ef, 0x0020, Data.Flags & CASHF_ABOVEZEROSALE);
		SETFLAG(ef, 0x0040, Data.ExtFlags & CASHFX_EXTSCARDSEL);
		SETFLAG(ef, 0x0080, Data.ExtFlags & CASHFX_KEEPORGCCUSER);
		SETFLAG(ef, 0x0100, Data.ExtFlags & CASHFX_DISABLEZEROSCARD); // @v8.3.2
		SETFLAG(ef, 0x0200, Data.ExtFlags & CASHFX_UHTTORDIMPORT); // @v8.3.2
		SETFLAG(ef, 0x0400, Data.ExtFlags & CASHFX_ABSTRGOODSALLOWED); // @v9.5.10
		SetClusterData(CTL_CASHN_EXTFLAGS, ef);
		{
			PPAlbatrosConfig acfg;
			PPAlbatrosCfgMngr::Get(&acfg);
			DisableClusterItem(CTL_CASHN_EXTFLAGS, 9, acfg.UhttAccount.Empty());
		}
	}
	AddClusterAssoc(CTL_CASHN_PASSIVE, 0, CASHFX_PASSIVE);
	SetClusterData(CTL_CASHN_PASSIVE, Data.ExtFlags);
	//disableCtrl(CTL_CASHN_OVRFLW, 1);
	setCtrlData(CTL_CASHN_SLEEPTIMEOUT, &Data.SleepTimeout);
	if(!Data.TableSelWhatman.NotEmptyS()) {
		FileBrowseCtrlGroup * p_fbg = (FileBrowseCtrlGroup*)getGroup(GRP_TBLDGMPATH);
		if(p_fbg) {
			SString path;
			PPGetPath(PPPATH_WTM, path);
			p_fbg->setInitPath(path);
		}
	}
	setCtrlString(CTL_CASHN_TBLDGMPATH,  Data.TableSelWhatman);
	setCtrlString(CTL_CASHN_SLIPFMTPATH, Data.SlipFmtPath);
	{
		PPObjCashNode::SelFilt f;
		f.OnlyGroups = 1;
		SetupPPObjCombo(this, CTLSEL_CASHN_GROUP, PPOBJ_CASHNODE, Data.ParentID, 0, &f);
	}
	disableCtrl(CTL_CASHN_CSESS, !PPMaster);
	SetupCtrls();
	return 1;
}

int SyncCashNodeCfgDialog::getDTS(PPSyncCashNode * pData)
{
	int    ok = 1;
	uint   sel = 0;
	getCtrlData(sel = CTL_CASHN_NAME, Data.Name);
	THROW_PP(*strip(Data.Name) != 0, PPERR_NAMENEEDED);
	getCtrlData(sel = CTL_CASHN_SYMB, Data.Symb);
	sel = CTL_CASHN_DEVICE;
	getCtrlData(CTLSEL_CASHN_DEVICE, &Data.CashType);
	THROW_PP(Data.CashType != 0, PPERR_CMTNEEDED);
	THROW_PP(Data.CashType != PPCMT_OKA500, PPERR_OKA500NOTSUPPORTED);
	THROW_PP(PPCashMachine::IsSyncCMT(Data.CashType), PPERR_CMSYNCNOTSUPP);
	Data.Speciality = (uint16)getCtrlLong(CTLSEL_CASHN_SPECIALITY);
	getCtrlData(CTL_CASHN_DRVVERMAJOR, &Data.DrvVerMajor);
	getCtrlData(CTL_CASHN_DRVVERMINOR, &Data.DrvVerMinor);
	sel = CTL_CASHN_LOC;
	getCtrlData(CTLSEL_CASHN_LOC, &Data.LocID);
	THROW_PP(Data.LocID != 0, PPERR_LOCNEEDED);
	getCtrlData(sel = CTLSEL_CASHN_NOA, &Data.GoodsLocAssocID);
	if(Data.GoodsLocAssocID) {
		PPNamedObjAssoc noa_rec;
		THROW(SearchObject(PPOBJ_NAMEDOBJASSOC, Data.GoodsLocAssocID, &noa_rec) > 0);
		THROW_PP_S(noa_rec.PrmrObjType == PPOBJ_GOODS && noa_rec.ScndObjType == PPOBJ_LOCATION &&
			oneof2(noa_rec.ScndObjGrp, 0, LOCTYP_WAREHOUSE), PPERR_INVPOSGOODSLOCASSOC, noa_rec.Name);
		GetClusterData(CTL_CASHN_NOA_PRINTONLY, &Data.ExtFlags);
	}
	else
		Data.ExtFlags &= ~CASHFX_GLASSOCPRINTONLY;
	getCtrlData(CTL_CASHN_PORT,     Data.Port);
	getCtrlData(CTL_CASHN_CURDATE, &Data.CurDate);
	if(PPMaster)
		getCtrlData(CTL_CASHN_CSESS, &Data.CurSessID);
	GetClusterData(CTL_CASHN_FLAGS, &Data.Flags);
	{
		const long ef = GetClusterData(CTL_CASHN_EXTFLAGS);
		SETFLAG(Data.Flags,    CASHF_UNIFYGDSATCHECK,    ef & 0x0001);
		SETFLAG(Data.Flags,    CASHF_UNIFYGDSTOPRINT,    ef & 0x0002);
		SETFLAG(Data.Flags,    CASHF_NOTUSECHECKCUTTER,  ef & 0x0004);
		SETFLAG(Data.ExtFlags, CASHFX_SELSERIALBYGOODS,  ef & 0x0008);
		SETFLAG(Data.ExtFlags, CASHFX_FORCEDIVISION,     ef & 0x0010);
		SETFLAG(Data.Flags,    CASHF_ABOVEZEROSALE,      ef & 0x0020);
		SETFLAG(Data.ExtFlags, CASHFX_EXTSCARDSEL,       ef & 0x0040);
		SETFLAG(Data.ExtFlags, CASHFX_KEEPORGCCUSER,     ef & 0x0080);
		SETFLAG(Data.ExtFlags, CASHFX_DISABLEZEROSCARD,  ef & 0x0100); // @v8.3.2
		SETFLAG(Data.ExtFlags, CASHFX_UHTTORDIMPORT,     ef & 0x0200); // @v8.3.2
		SETFLAG(Data.ExtFlags, CASHFX_ABSTRGOODSALLOWED, ef & 0x0400); // @v9.5.10
	}
	GetClusterData(CTL_CASHN_PASSIVE, &Data.ExtFlags);
	getCtrlData(CTL_CASHN_SLEEPTIMEOUT, &Data.SleepTimeout);
	getCtrlString(CTL_CASHN_TBLDGMPATH,  Data.TableSelWhatman);
	getCtrlString(CTL_CASHN_SLIPFMTPATH, Data.SlipFmtPath);
	getCtrlData(CTLSEL_CASHN_GROUP, &Data.ParentID);
	{
		PPObjCashNode cn_obj;
		THROW(cn_obj.Validate(&Data, 0));
	}
	ASSIGN_PTR(pData, Data);
	CATCH
		ok = PPErrorByDialog(this, sel, -1);
	ENDCATCH
	return ok;
}

int SyncCashNodeCfgDialog::EditCashParam()
{
	int    ok = -1;
	if(Data.ID) {
		PPCashMachine * p_cm = PPCashMachine::CreateInstance(Data.ID);
		PPSyncCashSession * p_si = (p_cm) ? p_cm->SyncInterface() : 0;
		ZDELETE(p_cm);
		if(p_si) {
			getCtrlData(CTL_CASHN_PORT, Data.Port);
			long port = atol(Data.Port);
			if((ok = p_si->EditParam(&port)) <= 0)
				PPError();
			else {
				ltoa(port, Data.Port, 10);
				setCtrlData(CTL_CASHN_PORT, Data.Port);
			}
		}
		ZDELETE(p_si);
	}
	return ok;
}

void SyncCashNodeCfgDialog::SetupCtrls()
{
	long   f = 0;
	GetClusterData(CTL_CASHN_FLAGS, &f);
	DisableClusterItem(CTL_CASHN_FLAGS, 4, !(f & CASHF_CHKPAN));
	if(!(f & CASHF_CHKPAN))
		f &= ~CASHF_SELALLGOODS;
	SetClusterData(CTL_CASHN_FLAGS, f);
	DisableClusterItem(CTL_CASHN_EXTFLAGS, 5, !(f & CASHF_SHOWREST));
	if(Data.CashType == PPCMT_ATOLDRV) {
		SString buf;
		getLabelText(CTL_CASHN_PORT, buf);
		PPGetWord(PPWORD_DEVNUM, 0, buf);
		setLabelText(CTL_CASHN_PORT, buf);
	}
	enableCommand(cmEditCashParam, BIN(Data.ID && Data.CashType == PPCMT_ATOLDRV));
	showButton(cmEditCashParam, BIN(Data.ID && Data.CashType == PPCMT_ATOLDRV));
}

IMPL_HANDLE_EVENT(SyncCashNodeCfgDialog)
{
	TDialog::handleEvent(event);
	if(event.isCmd(cmExtDevices))
		EditExtDevices(&Data);
	else if(event.isCmd(cmaMore))
		editExt();
	else if(event.isCmd(cmDivGrpAssc)) {
		SArray div_grp_list(sizeof(PPGenCashNode::DivGrpAssc));
		if(Data.P_DivGrpList)
			div_grp_list.copy(*Data.P_DivGrpList);
		if(editDivGrpAssoc(&div_grp_list) > 0)
			if(Data.P_DivGrpList)
				Data.P_DivGrpList->copy(div_grp_list);
			else
				Data.P_DivGrpList = new SArray(div_grp_list);
	}
	else if(event.isCmd(cmRounding))
		editRoundParam();
	else if(event.isClusterClk(CTL_CASHN_FLAGS))
		SetupCtrls();
	else if(event.isCbSelected(CTLSEL_CASHN_NOA))
		DisableClusterItem(CTL_CASHN_NOA_PRINTONLY, 0, !getCtrlLong(CTLSEL_CASHN_NOA));
	else if(event.isCmd(cmEditCashParam))
		EditCashParam();
	else
		return;
	clearEvent(event);
}

int SLAPI PPObjCashNode::EditSync(PPSyncCashNode * pSCN)
{
	DIALOG_PROC_BODY(SyncCashNodeCfgDialog, pSCN);
}

int SLAPI PPObjCashNode::EditGroup(PPGenCashNode * pGroup)
{
	class GroupCashNodeCfgDialog : public TDialog {
	public:
		GroupCashNodeCfgDialog() : TDialog(DLG_CASHNG)
		{
		}
		int setDTS(const PPGenCashNode * pData)
		{
			RVALUEPTR(Data, pData);
			setCtrlData(CTL_CASHN_NAME, Data.Name);
			setCtrlData(CTL_CASHN_SYMB, Data.Symb);
			setCtrlLong(CTL_CASHN_ID, Data.ID);
			SetupPPObjCombo(this, CTLSEL_CASHN_LOC, PPOBJ_LOCATION, Data.LocID, 0, 0);
			AddClusterAssoc(CTL_CASHN_GRPFLAGS, 0, CASHFX_UNITEGRPWROFF);
			SetClusterData(CTL_CASHN_GRPFLAGS, Data.ExtFlags);
			return 1;
		}
		int getDTS(PPGenCashNode * pData)
		{
			int    ok = 1;
			uint   sel = 0;
			getCtrlData(sel = CTL_CASHN_NAME, Data.Name);
			THROW_PP(*strip(Data.Name) != 0, PPERR_NAMENEEDED);
			getCtrlData(sel = CTL_CASHN_SYMB, Data.Symb);
			Data.CashType = PPCMT_CASHNGROUP;
			Data.LocID = getCtrlLong(CTLSEL_CASHN_LOC);
			Data.Flags &= ~(CASHF_SYNC|CASHF_ASYNC);
			GetClusterData(CTL_CASHN_GRPFLAGS, &Data.ExtFlags);
			{
				PPObjCashNode cn_obj;
				THROW(cn_obj.Validate(&Data, 0));
			}
			ASSIGN_PTR(pData, Data);
			CATCH
				ok = PPErrorByDialog(this, sel, -1);
			ENDCATCH
			return ok;
		}
	private:
		PPGenCashNode Data;
	};
	DIALOG_PROC_BODY(GroupCashNodeCfgDialog, pGroup);
}

int SLAPI PPObjCashNode::EditDistrib(PPGenCashNode * pData)
{
	class DistribCashNodeCfgDialog : public TDialog {
	public:
		DistribCashNodeCfgDialog() : TDialog(DLG_CASHND)
		{
		}
		int setDTS(const PPGenCashNode * pData)
		{
			RVALUEPTR(Data, pData);
			setCtrlData(CTL_CASHN_NAME, Data.Name);
			setCtrlData(CTL_CASHN_SYMB, Data.Symb);
			setCtrlLong(CTL_CASHN_ID, Data.ID);
			return 1;
		}
		int getDTS(PPGenCashNode * pData)
		{
			int    ok = 1;
			uint   sel = 0;
			getCtrlData(sel = CTL_CASHN_NAME, Data.Name);
			THROW_PP(*strip(Data.Name) != 0, PPERR_NAMENEEDED);
			getCtrlData(sel = CTL_CASHN_SYMB, Data.Symb);
			Data.CashType = PPCMT_DISTRIB;
			Data.Flags &= ~(CASHF_SYNC|CASHF_ASYNC);
			{
				PPObjCashNode cn_obj;
				THROW(cn_obj.Validate(&Data, 0));
			}
			ASSIGN_PTR(pData, Data);
			CATCH
				ok = PPErrorByDialog(this, sel, -1);
			ENDCATCH
			return ok;
		}
	private:
		PPGenCashNode Data;
	};
	DIALOG_PROC_BODY(DistribCashNodeCfgDialog, pData);
}
//
//
//
class AsyncCashNodeDialog : public TDialog {
public:
	AsyncCashNodeDialog(PPAsyncCashNode * pData) : TDialog(DLG_CASHNA)
	{
		PPIniFile  ini_file;
		ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_ACSCLOSE_USEALTIMPORT, &UseAltImport);
		P_Data = pData;
		FileBrowseCtrlGroup::Setup(this, CTLBRW_CASHN_IMPFILES, CTL_CASHN_IMPFILES, 1, 0, 0, FileBrowseCtrlGroup::fbcgfPath);
		FileBrowseCtrlGroup::Setup(this, CTLBRW_CASHN_EXPPATHS, CTL_CASHN_EXPPATHS, 2, 0, 0, FileBrowseCtrlGroup::fbcgfPath);
	}
private:
	DECL_HANDLE_EVENT;
	int    editDivGrpAssoc(SArray *);
	PPAsyncCashNode * P_Data; // class AsyncCahsNodeDialog not owns P_Data
	int    UseAltImport;      //
};

int AsyncCashNodeDialog::editDivGrpAssoc(SArray * pList)
{
	DIALOG_PROC_BODY(DivGrpAsscListDialog, pList);
}

IMPL_HANDLE_EVENT(AsyncCashNodeDialog)
{
	TDialog::handleEvent(event);
	if(event.isCmd(cmDivGrpAssc)) {
		SArray div_grp_list(sizeof(PPGenCashNode::DivGrpAssc));
		if(P_Data->P_DivGrpList)
			div_grp_list.copy(*P_Data->P_DivGrpList);
		if(editDivGrpAssoc(&div_grp_list) > 0)
			if(P_Data->P_DivGrpList)
				P_Data->P_DivGrpList->copy(div_grp_list);
			else
				P_Data->P_DivGrpList = new SArray(div_grp_list);
	}
	else if(event.isCmd(cmInputUpdated) && event.isCtlEvent(CTL_CASHN_DEVICE)) {
		PPID  cash_type = getCtrlLong(CTLSEL_CASHN_DEVICE);
		showCtrl(CTL_CASHN_IMPPARAM, BIN(cash_type == PPCMT_CRCSHSRV && UseAltImport));
	}
	else if(event.isCmd(cmImpParam)) {
		// @v9.1.3 EditCSessImpExpParams(1);
	}
	else if(event.isClusterClk(CTL_CASHN_FLAGS)) {
		ushort v = getCtrlUInt16(CTL_CASHN_FLAGS);
		enableCommand(cmDivGrpAssc, BIN(v & 0x04));
	}
	else if(event.isKeyDown(kbCtrlF8) && PPMaster) {
		PPEquipConfig eq_cfg;
		ReadEquipConfig(&eq_cfg);
		if(eq_cfg.OpOnTempSess) {
			getCtrlData(CTLSEL_CASHN_LOC, &P_Data->LocID);
			if(P_Data->LocID) {
				BillFilt bill_filt;
				bill_filt.Bbt = bbtDraftBills;
				bill_filt.OpID = eq_cfg.OpOnTempSess;
				bill_filt.Flags |= BillFilt::fAsSelector;
				bill_filt.LocList.Add(P_Data->LocID);
				PPViewBill bill_view;
				if(bill_view.Init_(&bill_filt)) {
					if(bill_view.Browse(0) > 0) {
						PPID   bill_id = ((BillFilt*)bill_view.GetBaseFilt())->Sel;
						BillTbl::Rec bill_rec;
						if(bill_id && BillObj->Search(bill_id, &bill_rec) > 0) {
							P_Data->CurRestBillID = bill_id;
							SString temp_buf;
							PPObjBill::MakeCodeString(&bill_rec, 0, temp_buf);
							setStaticText(CTL_CASHN_ST_CRBILL, temp_buf);
						}
					}
				}
			}
		}
	}
	else
		return;
	clearEvent(event);
}

int SLAPI PPObjCashNode::Validate(PPGenCashNode * pRec, long)
{
	int    ok = 1;
	THROW_PP(*strip(pRec->Name) != 0, PPERR_NAMENEEDED);
	THROW(CheckDupName(pRec->ID, pRec->Name));
	THROW(ref->CheckUniqueSymb(Obj, pRec->ID, pRec->Symb, offsetof(PPCashNode, Symb)));
	THROW_PP(pRec->CashType, PPERR_CMTNEEDED);
	if(pRec->LocID == 0) {
		if(pRec->CashType == PPCMT_CASHNGROUP) {
			if(pRec->ExtFlags & CASHFX_UNITEGRPWROFF) {
				CALLEXCEPT_PP(PPERR_LOCNEEDED);
			}
		}
		else if(pRec->CashType != PPCMT_DISTRIB)
			CALLEXCEPT_PP(PPERR_LOCNEEDED);
	}
	else {
		if(pRec->CashType == PPCMT_CASHNGROUP) {
			if(pRec->ID && pRec->ExtFlags & CASHFX_UNITEGRPWROFF) {
				PPIDArray child_list;
				GetListByGroup(pRec->ID, child_list);
				for(uint i = 0; i < child_list.getCount(); i++) {
					PPCashNode child_rec;
					if(Search(child_list.get(i), &child_rec) > 0) {
						THROW_PP_S(child_rec.LocID == pRec->LocID, PPERR_UNEQLOCINUNITEDPOSGROUP, child_rec.Name);
					}
				}
			}
		}
		else if(pRec->ParentID) {
			PPCashNode par_rec;
			THROW(Search(pRec->ParentID, &par_rec) > 0);
			if(par_rec.ExtFlags & CASHFX_UNITEGRPWROFF) {
				THROW_PP_S(par_rec.LocID == pRec->LocID, PPERR_UNEQLOCINUNITEDPOSGROUP, pRec->Name);
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjCashNode::EditAsync(PPAsyncCashNode * pACN)
{
	int    ok = -1, r = cmCancel, valid_data = 0;
	PPIDArray test_log_num_list;
	SString temp_buf;
	PPObjLocation loc_obj;
	AsyncCashNodeDialog * dlg = 0;
	THROW(CheckDialogPtr(&(dlg = new AsyncCashNodeDialog(pACN)), 0));
	dlg->setCtrlData(CTL_CASHN_NAME, pACN->Name);
	dlg->setCtrlData(CTL_CASHN_SYMB, pACN->Symb);
	dlg->setCtrlLong(CTL_CASHN_ID, pACN->ID);
	// @vmiller SetupStringCombo(dlg, CTLSEL_CASHN_DEVICE, PPTXT_CMT, pACN->CashType, 0);
	SetupStringComboDevice(dlg, CTLSEL_CASHN_DEVICE, DVCCLS_SYNCPOS, pACN->CashType, 0); // @vmiller
	dlg->setCtrlData(CTL_CASHN_DRVVERMAJOR, &pACN->DrvVerMajor);
	dlg->setCtrlData(CTL_CASHN_DRVVERMINOR, &pACN->DrvVerMinor);
	dlg->setCtrlString(CTL_CASHN_IMPFILES, pACN->ImpFiles);
	dlg->setCtrlString(CTL_CASHN_EXPPATHS, pACN->ExpPaths);
	dlg->setCtrlString(CTL_CASHN_LOGNUMS,  pACN->LogNumList);
	dlg->setCtrlString(CTL_CASHN_ADDEDMSGSIGN, pACN->AddedMsgSign);
	SETIFZ(pACN->LocID, loc_obj.GetSingleWarehouse());
	SetupPPObjCombo(dlg, CTLSEL_CASHN_LOC, PPOBJ_LOCATION, pACN->LocID, 0, 0);
	SetupPPObjCombo(dlg, CTLSEL_CASHN_NOA, PPOBJ_NAMEDOBJASSOC, pACN->GoodsLocAssocID, OLW_CANINSERT, 0);
	if(oneof2(pACN->CashType, PPCMT_CRUKM, PPCMT_CRCSHSRV))
		SetupPPObjCombo(dlg, CTLSEL_CASHN_EXTQUOT, PPOBJ_QUOTKIND, pACN->ExtQuotID, OLW_CANINSERT, 0);
	else
		dlg->disableCtrls(1, CTLSEL_CASHN_EXTQUOT, CTL_CASHN_EXTQUOT, 0);
	long   temp_flags = (pACN->Flags&(CASHF_EXPCHECKD|CASHF_IMPORTCHECKSWOZR|CASHF_EXPDIVN|CASHF_EXPGOODSREST|CASHF_EXPGOODSGROUPS)) |
		(pACN->ExtFlags & (CASHFX_EXPLOCPRNASSOC|CASHFX_APPLYUNITRND|CASHFX_RESTRUSERGGRP|CASHFX_RMVPASSIVEGOODS|
		CASHFX_CREATEOBJSONIMP|CASHFX_SEPARATERCPPRN|CASHFX_IGNLOOKBACKPRICES));
	dlg->AddClusterAssoc(CTL_CASHN_FLAGS,  0, CASHF_EXPCHECKD);
	dlg->AddClusterAssoc(CTL_CASHN_FLAGS,  1, CASHF_IMPORTCHECKSWOZR);
	dlg->AddClusterAssoc(CTL_CASHN_FLAGS,  2, CASHF_EXPDIVN);
	dlg->AddClusterAssoc(CTL_CASHN_FLAGS,  3, CASHF_EXPGOODSREST);
	dlg->AddClusterAssoc(CTL_CASHN_FLAGS,  4, CASHF_EXPGOODSGROUPS);
	dlg->AddClusterAssoc(CTL_CASHN_FLAGS,  5, CASHFX_EXPLOCPRNASSOC);
	dlg->AddClusterAssoc(CTL_CASHN_FLAGS,  6, CASHFX_APPLYUNITRND);
	dlg->AddClusterAssoc(CTL_CASHN_FLAGS,  7, CASHFX_RESTRUSERGGRP);
	dlg->AddClusterAssoc(CTL_CASHN_FLAGS,  8, CASHFX_RMVPASSIVEGOODS);
	dlg->AddClusterAssoc(CTL_CASHN_FLAGS,  9, CASHFX_CREATEOBJSONIMP);
	dlg->AddClusterAssoc(CTL_CASHN_FLAGS, 10, CASHFX_SEPARATERCPPRN);
	dlg->AddClusterAssoc(CTL_CASHN_FLAGS, 11, CASHFX_IGNLOOKBACKPRICES); // @v8.9.10
	dlg->SetClusterData(CTL_CASHN_FLAGS, temp_flags);
	dlg->enableCommand(cmDivGrpAssc, BIN(pACN->Flags & CASHF_EXPDIVN));

	dlg->AddClusterAssoc(CTL_CASHN_NOA_PRINTONLY, 0, CASHFX_GLASSOCPRINTONLY);
	dlg->SetClusterData(CTL_CASHN_NOA_PRINTONLY, pACN->ExtFlags);
	dlg->AddClusterAssoc(CTL_CASHN_PASSIVE, 0, CASHFX_PASSIVE);
	dlg->SetClusterData(CTL_CASHN_PASSIVE, pACN->ExtFlags);
	{
		PPObjCashNode::SelFilt f;
		f.OnlyGroups = 1;
		SetupPPObjCombo(dlg, CTLSEL_CASHN_GROUP, PPOBJ_CASHNODE, pACN->ParentID, 0, &f);
	}
	SetupPPObjCombo(dlg, CTLSEL_CASHN_GOODSGRP, PPOBJ_GOODSGROUP, pACN->GoodsGrpID, OLW_CANSELUPLEVEL);
	{
		temp_buf = 0;
		if(pACN->CurRestBillID) {
			BillTbl::Rec bill_rec;
			if(BillObj->Search(pACN->CurRestBillID, &bill_rec) > 0)
				PPObjBill::MakeCodeString(&bill_rec, 0, temp_buf);
			else
				ideqvalstr(pACN->CurRestBillID, temp_buf);
		}
		dlg->setStaticText(CTL_CASHN_ST_CRBILL, temp_buf);
	}
	while(!valid_data && (r = ExecView(dlg)) == cmOK) {
		dlg->getCtrlData(CTL_CASHN_NAME, pACN->Name);
		dlg->getCtrlData(CTL_CASHN_SYMB, pACN->Symb);
		dlg->getCtrlData(CTLSEL_CASHN_DEVICE, &pACN->CashType);
		dlg->getCtrlData(CTL_CASHN_DRVVERMAJOR, &pACN->DrvVerMajor);
		dlg->getCtrlData(CTL_CASHN_DRVVERMINOR, &pACN->DrvVerMinor);
		dlg->getCtrlData(CTLSEL_CASHN_EXTQUOT, &pACN->ExtQuotID);
		dlg->getCtrlData(CTLSEL_CASHN_LOC, &pACN->LocID);
		dlg->getCtrlData(CTLSEL_CASHN_NOA, &pACN->GoodsLocAssocID);
		dlg->getCtrlString(CTL_CASHN_IMPFILES, pACN->ImpFiles);
		dlg->getCtrlString(CTL_CASHN_EXPPATHS, pACN->ExpPaths);
		dlg->getCtrlString(CTL_CASHN_LOGNUMS, pACN->LogNumList);
		if(!pACN->GetLogNumList(test_log_num_list)) {
			PPErrorByDialog(dlg, CTL_CASHN_LOGNUMS, -1);
		}
		else {
			dlg->getCtrlString(CTL_CASHN_ADDEDMSGSIGN, pACN->AddedMsgSign);
			if(pACN->AddedMsgSign.NotEmptyS() && !PPGoodsPacket::ValidateAddedMsgSign(pACN->AddedMsgSign, 0))
				PPErrorByDialog(dlg, CTL_CASHN_ADDEDMSGSIGN, PPERR_INVPOSADDEDMSGSIGN);
			else {
				dlg->getCtrlData(CTL_CASHN_GROUP, &pACN->ParentID);
				dlg->getCtrlData(CTLSEL_CASHN_GOODSGRP, &pACN->GoodsGrpID);
				if(*strip(pACN->Name) == 0)
					PPErrorByDialog(dlg, CTL_CASHN_NAME, PPERR_NAMENEEDED);
				else if(pACN->CashType == 0)
					PPErrorByDialog(dlg, CTL_CASHN_DEVICE, PPERR_CMTNEEDED);
				else if(!PPCashMachine::IsAsyncCMT(pACN->CashType))
					PPErrorByDialog(dlg, CTL_CASHN_DEVICE, PPERR_CMSYNCNOTSUPP);
				else if(pACN->LocID == 0)
					PPErrorByDialog(dlg, CTL_CASHN_LOC, PPERR_LOCNEEDED);
				else {
					int    r2 = 1;
					if(pACN->GoodsLocAssocID) {
						PPNamedObjAssoc noa_rec;
						if(SearchObject(PPOBJ_NAMEDOBJASSOC, pACN->GoodsLocAssocID, &noa_rec) <= 0)
							r2 = PPErrorByDialog(dlg, CTLSEL_CASHN_NOA, -1);
						else if(!(noa_rec.PrmrObjType == PPOBJ_GOODS && noa_rec.ScndObjType == PPOBJ_LOCATION &&
							oneof2(noa_rec.ScndObjGrp, 0, LOCTYP_WAREHOUSE))) {
							PPSetAddedMsgString(noa_rec.Name);
							r2 = PPErrorByDialog(dlg, CTLSEL_CASHN_NOA, PPERR_INVPOSGOODSLOCASSOC);
						}
						dlg->GetClusterData(CTL_CASHN_NOA_PRINTONLY, &pACN->ExtFlags);
					}
					else
						pACN->ExtFlags &= ~CASHFX_GLASSOCPRINTONLY;
					if(r2) {
						if(!Validate(pACN, 0))
							PPError();
						else {
							dlg->GetClusterData(CTL_CASHN_FLAGS, &temp_flags);
							pACN->Flags = (temp_flags & ~(CASHFX_EXPLOCPRNASSOC|CASHFX_APPLYUNITRND|CASHFX_RESTRUSERGGRP|CASHFX_RMVPASSIVEGOODS|CASHFX_CREATEOBJSONIMP|CASHFX_SEPARATERCPPRN));
							pACN->Flags |= CASHF_ASYNC;
							SETFLAGBYSAMPLE(pACN->ExtFlags, CASHFX_EXPLOCPRNASSOC,  temp_flags);
							SETFLAGBYSAMPLE(pACN->ExtFlags, CASHFX_APPLYUNITRND,    temp_flags);
							SETFLAGBYSAMPLE(pACN->ExtFlags, CASHFX_RESTRUSERGGRP,   temp_flags);
							SETFLAGBYSAMPLE(pACN->ExtFlags, CASHFX_RMVPASSIVEGOODS, temp_flags);
							SETFLAGBYSAMPLE(pACN->ExtFlags, CASHFX_CREATEOBJSONIMP, temp_flags);
							SETFLAGBYSAMPLE(pACN->ExtFlags, CASHFX_SEPARATERCPPRN,  temp_flags);
							SETFLAGBYSAMPLE(pACN->ExtFlags, CASHFX_IGNLOOKBACKPRICES, temp_flags); // @v8.9.10
							dlg->GetClusterData(CTL_CASHN_PASSIVE, &pACN->ExtFlags);
							ok = valid_data = 1;
						}
					}
				}
			}
		}
	}
	CATCHZOK
	delete dlg;
	return ok;
}

int SLAPI PPObjCashNode::Edit(PPID * pID, void * extraPtr)
{
	class SelCashTypeDialog : public TDialog {
	public:
		SelCashTypeDialog() : TDialog(DLG_SELCASHNS)
		{
			//SetupStringCombo(this, CTLSEL_SELCASHNS_DEVICE, PPTXT_CMT, 0, 0); // vmiller comment
			SetupStringComboDevice(this, CTLSEL_SELCASHNS_DEVICE, DVCCLS_SYNCPOS, 0, 0); // @vmiller
		}
		int getDTS(PPID * pCmtID, int * pSA)
		{
			getCtrlData(CTLSEL_SELCASHNS_DEVICE, pCmtID);
			*pSA = getCtrlUInt16(CTL_SELCASHNS_SA) ? 2 : 1;
			return 1;
		}
	private:
		int replyCashTypeSelection()
		{
			PPID   cmt = getCtrlLong(CTLSEL_SELCASHNS_DEVICE);
			int    s = BIN(oneof2(cmt, PPCMT_CASHNGROUP, PPCMT_DISTRIB) || PPCashMachine::IsSyncCMT(cmt));
			int    a = PPCashMachine::IsAsyncCMT(cmt);
			disableCtrl(CTL_SELCASHNS_SA, !s || !a);
			if((!s && !a) || (s && a))
				return (PPError(PPERR_CNUNDEFSYNC, 0), 0);
			else if(s)
				return (setCtrlUInt16(CTL_SELCASHNS_SA, 0), 1);
			else
				return (setCtrlUInt16(CTL_SELCASHNS_SA, 1), 2);
		}
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCbSelected(CTLSEL_SELCASHNS_DEVICE)) {
				if(replyCashTypeSelection() == 0)
					messageToCtrl(CTLSEL_SELCASHNS_DEVICE, cmCBActivate, 0);
				clearEvent(event);
			}
		}
	};
	int    ok = 1, s = 0, r = cmCancel;
	long   f = 0;
	SelCashTypeDialog * dlg = 0;
	PPGenCashNode * p_cn = 0;
	PPCashNode cn_rec;
	if(*pID) {
		THROW(Search(*pID, &cn_rec) > 0);
		if(!oneof2(cn_rec.CashType, PPCMT_CASHNGROUP, PPCMT_DISTRIB)) {
			f = (cn_rec.Flags & (CASHF_SYNC|CASHF_ASYNC));
			if(f == 0 || f == (CASHF_SYNC|CASHF_ASYNC)) {
				if(PPCashMachine::IsSyncCMT(cn_rec.CashType))
					s = 1;
				else if(PPCashMachine::IsAsyncCMT(cn_rec.CashType))
					s = 2;
				else {
					CALLEXCEPT_PP_S(PPERR_CNUNDEFSYNC, cn_rec.Name);
				}
			}
			else
				s = (f & CASHF_SYNC) ? 1 : 2;
		}
	}
	else {
		THROW(CheckDialogPtr(&(dlg = new SelCashTypeDialog)));
		if(ExecView(dlg) == cmOK) {
			dlg->getDTS(&cn_rec.CashType, &s);
		}
		else
			ok = -1;
	}
	if(ok > 0) {
		if(oneof2(cn_rec.CashType, PPCMT_CASHNGROUP, PPCMT_DISTRIB))
			p_cn = new PPGenCashNode;
		else
			p_cn = (s == 1) ? (PPGenCashNode *)(new PPSyncCashNode) : (PPGenCashNode *)(new PPAsyncCashNode);
		THROW_MEM(p_cn);
		if(*pID) {
			if(oneof2(cn_rec.CashType, PPCMT_CASHNGROUP, PPCMT_DISTRIB)) {
				THROW(Get(*pID, p_cn) > 0);
			}
			else if(s == 1) {
				THROW(GetSync(*pID, (PPSyncCashNode*)p_cn) > 0);
			}
			else {
				THROW(GetAsync(*pID, (PPAsyncCashNode*)p_cn) > 0);
			}
		}
		else {
			p_cn->CashType = cn_rec.CashType;
			if(s == 1) {
				p_cn->Flags |= (CASHF_CHKPAN|CASHF_NOMODALCHECKVIEW);
			}
		}
		SETFLAG(p_cn->Flags, CASHF_SYNC,  s == 1);
		SETFLAG(p_cn->Flags, CASHF_ASYNC, s == 2);
		if(p_cn->CashType == PPCMT_CASHNGROUP) {
			THROW(r = EditGroup(p_cn));
		}
		else if(p_cn->CashType == PPCMT_DISTRIB) {
			THROW(r = EditDistrib(p_cn));
		}
		else
			THROW(r = (s == 1) ? EditSync((PPSyncCashNode*)p_cn) : EditAsync((PPAsyncCashNode*)p_cn));
		if(r > 0) {
			THROW(Put(pID, p_cn, 1));
			r = cmOK;
		}
		else
			r = cmCancel;
	}
	CATCHZOKPPERR
	delete p_cn;
	delete dlg;
	return ok ? r : 0;
}

int SLAPI ViewCashNodes()
{
	int       ok = 1;
	SString   host_name, impexp_path;
	PPIniFile ini_file;
	ini_file.Get(PPINISECT_POS_HOST, PPINIPARAM_NAME, host_name);
	ini_file.Get(PPINISECT_POS_HOST, PPINIPARAM_PATH, impexp_path);
	if(host_name.NotEmpty() || impexp_path.NotEmpty()) {
		PPID  acn_id = 0;
		PPAsyncCashNode  acn;
		PPObjCashNode    cn_obj;
		if(host_name.NotEmpty())
			host_name.Trim(sizeof(acn.Name) - 1);
		else
			host_name = "poshost";
		if(cn_obj.SearchByName(host_name, &acn_id, 0) < 0) {
			host_name.CopyTo(acn.Name, sizeof(acn.Name));
			acn.CashType = PPCMT_PAPYRUS;
			acn.Flags    = CASHF_ASYNC;
			acn.LocID    = LConfig.Location;
			if(impexp_path.NotEmptyS()) {
				acn.ImpFiles = impexp_path;
				acn.ExpPaths = impexp_path;
			}
			else {
				PPGetPath(PPPATH_IN, acn.ImpFiles);
				PPGetPath(PPPATH_OUT, acn.ExpPaths);
			}
			cn_obj.Put(&acn_id, (PPGenCashNode *)&acn, 1);
		}
	}
	ShowObjects(PPOBJ_CASHNODE, 0);
	return ok;
}
//
//
//
class CashNodeCache : public ObjCache {
public:
	SLAPI CashNodeCache() : ObjCache(PPOBJ_CASHNODE, sizeof(Data))
	{
	}
private:
	virtual int SLAPI FetchEntry(PPID, ObjCacheEntry * pEntry, long);
	virtual int SLAPI EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
public:
	struct Data : public ObjCacheEntry {
		PPID   CashType;
		PPID   ExtQuotID;
		PPID   LocID;
		PPID   ParentID;
		long   Flags;
		long   ExtFlags;
	};
};

int SLAPI CashNodeCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, long)
{
	int    ok = 1;
	Data * p_cache_rec = (Data *)pEntry;
	PPObjCashNode cn_obj;
	PPCashNode2 rec;
	if(cn_obj.Search(id, &rec) > 0) {
#define CPY_FLD(Fld) p_cache_rec->Fld=rec.Fld
		CPY_FLD(CashType);
		CPY_FLD(ExtQuotID);
		CPY_FLD(LocID);
		CPY_FLD(ParentID);
		CPY_FLD(Flags);
		CPY_FLD(ExtFlags);
#undef CPY_FLD
		MultTextBlock b;
		b.Add(rec.Name);
		b.Add(rec.Symb);
		ok = PutTextBlock(b, p_cache_rec);
	}
	else
		ok = -1;
	return ok;
}

int SLAPI CashNodeCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	PPCashNode * p_data_rec = (PPCashNode *)pDataRec;
	const Data * p_cache_rec = (const Data *)pEntry;
	memzero(p_data_rec, sizeof(*p_data_rec));
#define CPY_FLD(Fld) p_data_rec->Fld=p_cache_rec->Fld
	p_data_rec->Tag = PPOBJ_CASHNODE;
	CPY_FLD(ID);
	CPY_FLD(CashType);
	CPY_FLD(ExtQuotID);
	CPY_FLD(LocID);
	CPY_FLD(ParentID);
	CPY_FLD(Flags);
	CPY_FLD(ExtFlags);
#undef CPY_FLD
	MultTextBlock b(this, pEntry);
	b.Get(p_data_rec->Name, sizeof(p_data_rec->Name));
	b.Get(p_data_rec->Symb, sizeof(p_data_rec->Symb));
	return 1;
}

IMPL_OBJ_FETCH(PPObjCashNode, PPCashNode, CashNodeCache);
//
//
//
SLAPI PPEquipConfig::PPEquipConfig()
{
	THISZERO();
}

PPID SLAPI PPEquipConfig::GetCashierTabNumberRegTypeID()
{
	PPID   result = 0;
	PPIniFile ini_file;
	SString rt_symb;
	if(ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_CSHRTABNUMREGSYMB, rt_symb) && rt_symb.NotEmpty()) {
		PPObjRegisterType obj_regt;
		PPRegisterType rt_rec;
		for(PPID regt_id = 0; obj_regt.EnumItems(&regt_id, &rt_rec) > 0;)
			if(oneof2(rt_rec.PersonKindID, 0, CshrsPsnKindID) && stricmp866(rt_rec.Symb, rt_symb) == 0) {
				result = regt_id;
				break;
			}
	}
	return result;
}
//
//
//
static const char * RpCheckScaleInput = "CheckScaleInput";

int SLAPI ReadEquipConfig(PPEquipConfig * pCfg)
{
	int    use_scale_input = 0;
	{
		WinRegKey reg_key(HKEY_CURRENT_USER, PPRegKeys::SysSettings, 1);
		uint32 val = 0;
		if(reg_key.GetDWord(RpCheckScaleInput, &val))
			use_scale_input = BIN(val);
	}
	int    r = PPRef->GetProp(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_EQUIPCFG, pCfg, sizeof(*pCfg));
	if(pCfg)
		if(r > 0) {
			SETFLAG(pCfg->Flags, PPEquipConfig::fCheckScaleInput, use_scale_input);
		}
		else
			memzero(pCfg, sizeof(*pCfg));
	return r;
}

class EquipConfigDlg : public TDialog {
public:
	EquipConfigDlg() : TDialog(DLG_EQUIPCFG)
	{
	}
	int    setDTS(const PPEquipConfig *);
	int    getDTS(PPEquipConfig *);
private:
	DECL_HANDLE_EVENT;
	void   SetupCtrls();
	PPEquipConfig Data;
};

int EquipConfigDlg::setDTS(const PPEquipConfig * pData)
{
	PPIDArray op_type_list;
	if(!RVALUEPTR(Data, pData))
		MEMSZERO(Data);
	SetupPPObjCombo(this, CTLSEL_EQCFG_PSNKNDCSHRS, PPOBJ_PRSNKIND, Data.CshrsPsnKindID, 0, 0);
	SetupPPObjCombo(this, CTLSEL_EQCFG_DEFCASHNODE, PPOBJ_CASHNODE, Data.DefCashNodeID, 0, 0);
	// @v9.0.4 SetupPPObjCombo(this, CTLSEL_EQCFG_SCALE,       PPOBJ_SCALE,    Data.ScaleID,       0, 0);
	//
	op_type_list.addzlist(PPOPT_ACCTURN, 0L);
	SetupOprKindCombo(this, CTLSEL_EQCFG_WROFFACCOP, Data.WrOffAccOpID, 0, &op_type_list, 0);
	op_type_list.clear();
	//
	op_type_list.addzlist(PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, 0L);
	SetupOprKindCombo(this, CTLSEL_EQCFG_OPDTHISLOC, Data.OpOnDfctThisLoc, 0, &op_type_list, 0);
	SetupOprKindCombo(this, CTLSEL_EQCFG_OPDOTHRLOC, Data.OpOnDfctOthrLoc, 0, &op_type_list, 0);
	SetupOprKindCombo(this, CTLSEL_EQCFG_TEMPSESSOP, Data.OpOnTempSess,    0, &op_type_list, 0);
	SetupPPObjCombo(this, CTLSEL_EQCFG_QUOT, PPOBJ_QUOTKIND, Data.QuotKindID,  0);
	SetupPPObjCombo(this, CTLSEL_EQCFG_FTPACCT, PPOBJ_INTERNETACCOUNT, Data.FtpAcctID, 0, (void *)INETACCT_ONLYFTP);
	SetupPPObjCombo(this, CTLSEL_EQCFG_SALESGRP, PPOBJ_GOODSGROUP, Data.SalesGoodsGrp, OLW_CANSELUPLEVEL, (void *)GGRTYP_SEL_ALT);
	setCtrlData(CTL_EQCFG_AGENTCODELEN, &Data.AgentCodeLen);
	setCtrlData(CTL_EQCFG_AGENTPREFIX,  &Data.AgentPrefix);
	setCtrlData(CTL_EQCFG_SUSPCPFX, Data.SuspCcPrefix); // @v8.1.9
	AddClusterAssoc(CTL_EQCFG_FLAGS,  0, PPEquipConfig::fCheckScaleInput);
	AddClusterAssoc(CTL_EQCFG_FLAGS,  1, PPEquipConfig::fComplDeficit);
	AddClusterAssoc(CTL_EQCFG_FLAGS,  2, PPEquipConfig::fCloseSessTo10Level);
	AddClusterAssoc(CTL_EQCFG_FLAGS,  3, PPEquipConfig::fIgnAcsReadyTags);
	AddClusterAssoc(CTL_EQCFG_FLAGS,  4, PPEquipConfig::fIgnGenGoodsOnDeficit);
	AddClusterAssoc(CTL_EQCFG_FLAGS,  5, PPEquipConfig::fIntrPriceByRetailRules);
	AddClusterAssoc(CTL_EQCFG_FLAGS,  6, PPEquipConfig::fValidateChecksOnSessClose);
	AddClusterAssoc(CTL_EQCFG_FLAGS,  7, PPEquipConfig::fWriteToChkOpJrnl);
	AddClusterAssoc(CTL_EQCFG_FLAGS,  8, PPEquipConfig::fRecognizeCode);
	AddClusterAssoc(CTL_EQCFG_FLAGS,  9, PPEquipConfig::fUnifiedPayment);
	AddClusterAssoc(CTL_EQCFG_FLAGS, 10, PPEquipConfig::fUnifiedPaymentCfmBank); // @v8.6.6
	AddClusterAssoc(CTL_EQCFG_FLAGS, 11, PPEquipConfig::fIgnoreNoDisGoodsTag);
	AddClusterAssoc(CTL_EQCFG_FLAGS, 12, PPEquipConfig::fRestrictQttyByUnitRnd);
	AddClusterAssoc(CTL_EQCFG_FLAGS, 13, PPEquipConfig::fDisableManualSCardInput);
	AddClusterAssoc(CTL_EQCFG_FLAGS, 14, PPEquipConfig::fDisableAdjWrOffAmount); // @v8.6.6

	SetClusterData(CTL_EQCFG_FLAGS, Data.Flags);
	AddClusterAssoc(CTL_EQCFG_FLAGS2, 0, PPEquipConfig::fUseQuotAsPrice);
	AddClusterAssoc(CTL_EQCFG_FLAGS2, 1, PPEquipConfig::fUncondAsyncBasePrice);
	AddClusterAssoc(CTL_EQCFG_FLAGS2, 2, PPEquipConfig::fAutosaveSyncChecks); // @v8.7.7

	SetClusterData(CTL_EQCFG_FLAGS2, Data.Flags);
	{
		RealRange subst_range;
		SetRealRangeInput(this, CTL_EQCFG_DFCTCOSTRNG, &(subst_range = Data.DeficitSubstPriceDevRange).Scale(0.1), 1);
	}
	{
		double rng_lim = ((double)Data.BHTRngLimWgtGoods) / 100;
		setCtrlData(CTL_EQCFG_RNGLIMGOODSBHT, &rng_lim);

		rng_lim = ((double)Data.BHTRngLimPrice) / 100;
		setCtrlData(CTL_EQCFG_RNGLIMPRICEBHT, &rng_lim);
	}
	SetupCtrls();
	return 1;
}

int EquipConfigDlg::getDTS(PPEquipConfig * pData)
{
	int    ok = 1;
	uint   sel = 0;
	getCtrlData(CTLSEL_EQCFG_WROFFACCOP,  &Data.WrOffAccOpID);
	getCtrlData(CTLSEL_EQCFG_PSNKNDCSHRS, &Data.CshrsPsnKindID);
	getCtrlData(CTLSEL_EQCFG_DEFCASHNODE, &Data.DefCashNodeID);
	// @v9.0.4 getCtrlData(CTLSEL_EQCFG_SCALE,       &Data.ScaleID);
	getCtrlData(CTLSEL_EQCFG_OPDTHISLOC,  &Data.OpOnDfctThisLoc);
	getCtrlData(CTLSEL_EQCFG_OPDOTHRLOC,  &Data.OpOnDfctOthrLoc);
	getCtrlData(CTLSEL_EQCFG_TEMPSESSOP,  &Data.OpOnTempSess);
	getCtrlData(CTLSEL_EQCFG_QUOT,        &Data.QuotKindID);
	getCtrlData(CTLSEL_EQCFG_FTPACCT,     &Data.FtpAcctID);
	getCtrlData(CTLSEL_EQCFG_SALESGRP,    &Data.SalesGoodsGrp);
	getCtrlData(sel = CTL_EQCFG_AGENTCODELEN, &Data.AgentCodeLen);
	getCtrlData(sel = CTL_EQCFG_AGENTPREFIX,  &Data.AgentPrefix);
	getCtrlData(sel = CTL_EQCFG_SUSPCPFX, Data.SuspCcPrefix); // @v8.1.9
	if(Data.SalesGoodsGrp) {
		Goods2Tbl::Rec ggrec;
		PPObjGoodsGroup ggobj;
		MEMSZERO(ggrec);
		ggobj.Search(Data.SalesGoodsGrp, &ggrec);
		if(!(ggrec.Flags & GF_EXCLALTFOLD))
			ok = PPErrorByDialog(this, CTLSEL_EQCFG_SALESGRP, PPERR_INVSALESGRP);
	}
	{
		RealRange subst_range;
		GetRealRangeInput(this, CTL_EQCFG_DFCTCOSTRNG, &subst_range);
		if(!subst_range.IsZero()) {
			if(subst_range.low == subst_range.upp) {
				subst_range.low = -fabs(subst_range.low);
				subst_range.upp = +fabs(subst_range.upp);
			}
			subst_range.Scale(10.0);
			Data.DeficitSubstPriceDevRange.Set((int)subst_range.low, (int)subst_range.upp);
		}
		else
			Data.DeficitSubstPriceDevRange = 0;
	}
	{
		double rng_lim = 0;
		getCtrlData(sel = CTL_EQCFG_RNGLIMGOODSBHT, &rng_lim);
		THROW_PP(rng_lim >= 0 && rng_lim <= 100, PPERR_PERCENTINPUT);
		Data.BHTRngLimWgtGoods = (long)(rng_lim * 100);

		getCtrlData(sel = CTL_EQCFG_RNGLIMPRICEBHT, &(rng_lim = 0));
		THROW_PP(rng_lim >= 0 && rng_lim <= 100, PPERR_PERCENTINPUT);
		Data.BHTRngLimPrice = (long)(rng_lim * 100);
	}
	const int prefix_len = strlen(Data.AgentPrefix);
	if(Data.AgentCodeLen < 0 || (prefix_len && prefix_len > (Data.AgentCodeLen-1))) {
		ok = PPErrorByDialog(this, sel, PPERR_USERINPUT);
	}
	else {
		GetClusterData(CTL_EQCFG_FLAGS,  &Data.Flags);
		GetClusterData(CTL_EQCFG_FLAGS2, &Data.Flags);
		ASSIGN_PTR(pData, Data);
	}
	CATCH
		selectCtrl(sel);
		ok = 0;
	ENDCATCH
	return ok;
}

void EquipConfigDlg::SetupCtrls()
{
	const PPID quotk = getCtrlLong(CTLSEL_EQCFG_QUOT);
	DisableClusterItem(CTL_EQCFG_FLAGS, 6, quotk);
	if(quotk) {
		long   flags = 0;
		GetClusterData(CTL_EQCFG_FLAGS, &flags);
		flags &= ~PPEquipConfig::fIntrPriceByRetailRules;
		SetClusterData(CTL_EQCFG_FLAGS, flags);
	}
}

IMPL_HANDLE_EVENT(EquipConfigDlg)
{
	TDialog::handleEvent(event);
	if(event.isCbSelected(CTLSEL_EQCFG_QUOT)) {
		SetupCtrls();
		clearEvent(event);
	}
}

int SLAPI EditEquipConfig()
{
	int    ok = -1, is_new = 0;
	EquipConfigDlg * dlg = 0;
	PPEquipConfig  eq_cfg;
	THROW(CheckCfgRights(PPCFGOBJ_EQUIP, PPR_READ, 0));
	THROW(is_new = ReadEquipConfig(&eq_cfg));
	THROW(CheckDialogPtr(&(dlg = new EquipConfigDlg())));
	dlg->setDTS(&eq_cfg);
	while(ok < 0 && ExecView(dlg) == cmOK) {
		THROW(CheckCfgRights(PPCFGOBJ_EQUIP, PPR_MOD, 0));
		if(dlg->getDTS(&eq_cfg)) {
			{
				WinRegKey reg_key(HKEY_CURRENT_USER, PPRegKeys::SysSettings, 0);
				uint32 val = BIN(eq_cfg.Flags & PPEquipConfig::fCheckScaleInput);
				reg_key.PutDWord(RpCheckScaleInput, val);
			}
			eq_cfg.Flags &= ~PPEquipConfig::fCheckScaleInput;
			{
				PPTransaction tra(1);
				THROW(tra);
				THROW(PPRef->PutProp(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_EQUIPCFG, &eq_cfg, sizeof(eq_cfg), 0));
				DS.LogAction(is_new == -1 ? PPACN_CONFIGCREATED : PPACN_CONFIGUPDATED, PPCFGOBJ_EQUIP, 0, 0, 0);
				THROW(tra.Commit());
			}
			ok = 1;
		}
		else
			PPError();
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

PPID SLAPI GetCashiersPsnKindID()
{
	PPEquipConfig  eq_cfg;
	return (ReadEquipConfig(&eq_cfg) > 0) ? eq_cfg.CshrsPsnKindID : 0L;
}
//
// @ModuleDef(PPObjTouchScreen)
//
SLAPI PPTouchScreenPacket::PPTouchScreenPacket()
{
}

PPTouchScreenPacket & FASTCALL PPTouchScreenPacket::operator = (const PPTouchScreenPacket & s)
{
	Rec = s.Rec;
	GrpIDList.copy(s.GrpIDList);
	return *this;
}

SLAPI PPObjTouchScreen::PPObjTouchScreen(void * extraPtr) : PPObjReference(PPOBJ_TOUCHSCREEN, extraPtr)
{
}

int SLAPI PPObjTouchScreen::GetPacket(PPID id, PPTouchScreenPacket * pPack)
{
	int    ok = 1;
	PPTouchScreenPacket  ts_pack;
	THROW(ref->GetItem(Obj, id, &ts_pack.Rec) > 0);
	THROW(ref->GetPropArray(Obj, id, DBDPRP_LOCLIST, &ts_pack.GrpIDList));
	CATCH
		MEMSZERO(ts_pack.Rec);
		ts_pack.GrpIDList.freeAll();
		ok = 0;
	ENDCATCH
	ASSIGN_PTR(pPack, ts_pack);
	return ok;
}

int SLAPI PPObjTouchScreen::PutPacket(PPID * pID, PPTouchScreenPacket * pPack, int use_ta)
{
	int    ok = 1;
	PPIDArray * p_grp_list = (pPack && pPack->GrpIDList.getCount()) ? &pPack->GrpIDList : 0;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(pPack) {
			THROW(CheckDupName(*pID, pPack->Rec.Name));
			if(*pID) {
				THROW(ref->UpdateItem(Obj, *pID, &pPack->Rec, 1, 0));
			}
			else {
				THROW(ref->AddItem(Obj, pID, &pPack->Rec, 0));
			}
		}
		else if(*pID) {
			THROW(ref->RemoveItem(Obj, *pID, 0));
		}
		THROW(ref->PutPropArray(Obj, *pID, TSCPRP_GRPLIST, p_grp_list, 0));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}
//
//   TouchScreenDlg
//
class TouchScreenDlg : public TDialog {
public:
	TouchScreenDlg() : TDialog(DLG_TCHSCR)
	{
	}
	int    getDTS(PPTouchScreenPacket * pPack);
	int    setDTS(const PPTouchScreenPacket * pPack);
private:
	int    SelGdsFont();
	int    SelGrpList();
	DECL_HANDLE_EVENT;
	PPTouchScreenPacket Data;
};

int TouchScreenDlg::SelGdsFont()
{
	int    ok = 1;
	CHOOSEFONT font;
	LOGFONT    log_font;
	MEMSZERO(font);
	MEMSZERO(log_font);
	font.hwndOwner = H();
	font.Flags = CF_FORCEFONTEXIST|CF_SCREENFONTS|CF_INITTOLOGFONTSTRUCT|CF_NOVERTFONTS|CF_NOSCRIPTSEL;
	log_font.lfCharSet = RUSSIAN_CHARSET;
	STRNSCPY(log_font.lfFaceName, Data.Rec.GdsListFontName); // @unicodeproblem
	log_font.lfHeight = Data.Rec.GdsListFontHight;
	font.lpLogFont   = &log_font;
	font.lStructSize = sizeof(font);
	if(::ChooseFont(&font)) { // @unicodeproblem
		STRNSCPY(Data.Rec.GdsListFontName, font.lpLogFont->lfFaceName);
		Data.Rec.GdsListFontHight = font.lpLogFont->lfHeight;
	}
	else if(CommDlgExtendedError() != 0)
		ok = (PPError(PPERR_DLGLOADFAULT), 0);
	return ok;
}

int TouchScreenDlg::SelGrpList()
{
	ListToListData ll_data(PPOBJ_GOODSGROUP, 0, &Data.GrpIDList);
	ll_data.Flags |= ListToListData::fIsTreeList;
	ll_data.TitleStrID = PPTXT_SELGOODSGRPS;
	return ListToListDialog(&ll_data) ? 1 : PPErrorZ();
}

IMPL_HANDLE_EVENT(TouchScreenDlg)
{
	TDialog::handleEvent(event);
	if(event.isCmd(cmSelFont))
		SelGdsFont();
	else if(event.isCmd(cmSelGrpList))
		SelGrpList();
	else
		return;
	clearEvent(event);
}

int TouchScreenDlg::setDTS(const PPTouchScreenPacket * pData)
{
	Data = *pData;
	setCtrlData(CTL_TCHSCR_NAME, Data.Rec.Name);
	setCtrlData(CTL_TCHSCR_ID,  &Data.Rec.ID);
	SetupStringCombo(this, CTLSEL_TCHSCR_TYPE, PPTXT_TOUCHSCREEN, Data.Rec.TouchScreenType);
	SetupPPObjCombo(this, CTLSEL_TCHSCR_GDSGRP, PPOBJ_GOODSGROUP, Data.Rec.AltGdsGrpID, OLW_CANINSERT, (void *)GGRTYP_SEL_ALT);
	setCtrlData(CTL_TCHSCR_GOODSLISTGAP, &Data.Rec.GdsListEntryGap);
	AddClusterAssoc(CTL_TCHSCR_FLAGS, 0, TSF_PRINTSLIPDOC);
	AddClusterAssoc(CTL_TCHSCR_FLAGS, 1, TSF_TXTSTYLEBTN);
	SetClusterData(CTL_TCHSCR_FLAGS, Data.Rec.Flags);
	return 1;
}

int TouchScreenDlg::getDTS(PPTouchScreenPacket * pData)
{
	getCtrlData(CTL_TCHSCR_NAME, Data.Rec.Name);
	getCtrlData(CTL_TCHSCR_ID,  &Data.Rec.ID);
	getCtrlData(CTLSEL_TCHSCR_TYPE,   &Data.Rec.TouchScreenType);
	getCtrlData(CTLSEL_TCHSCR_GDSGRP, &Data.Rec.AltGdsGrpID);
	getCtrlData(CTL_TCHSCR_GOODSLISTGAP, &Data.Rec.GdsListEntryGap);
	GetClusterData(CTL_TCHSCR_FLAGS,  &Data.Rec.Flags);
	ASSIGN_PTR(pData, Data);
	return 1;
}

int SLAPI PPObjTouchScreen::Edit(PPID * pID, void * extraPtr)
{
	int    ok = 1, r = cmCancel, valid_data = 0, is_new = 0;
	TouchScreenDlg * dlg = 0;
	PPTouchScreenPacket  ts_pack;
	THROW(CheckDialogPtr(&(dlg = new TouchScreenDlg)));
	THROW(EditPrereq(pID, dlg, &is_new));
	if(!is_new)
		THROW(GetPacket(*pID, &ts_pack) > 0);
	dlg->setDTS(&ts_pack);
	while(!valid_data && (r = ExecView(dlg)) == cmOK) {
		dlg->getDTS(&ts_pack);
		if(PutPacket(pID, &ts_pack, 1))
			valid_data = 1;
		else
			PPError();
	}
	CATCHZOKPPERR
	delete dlg;
	return ok ? r : 0;
}
//
// @ModuleDef(PPLocPrinter)
//
SLAPI PPObjLocPrinter::PPObjLocPrinter(void * extraPtr) : PPObjReference(PPOBJ_LOCPRINTER, extraPtr)
{
}

int SLAPI PPObjLocPrinter::GetPacket(PPID id, PPLocPrinter * pPack)
{
	PPLocPrinter  loc_prn;
	int   ok = Search(id, &loc_prn);
	ASSIGN_PTR(pPack, loc_prn);
	return ok;
}

int SLAPI PPObjLocPrinter::PutPacket(PPID * pID, PPLocPrinter * pPack, int use_ta)
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
			}
		}
		else {
			*pID = pPack->ID;
			THROW(CheckDupName(*pID, pPack->Name));
			THROW(ref->AddItem(Obj, pID, pPack, 0));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

// virtual
int SLAPI PPObjLocPrinter::Edit(PPID * pID, void * extraPtr)
{
	class LocPrinterDialog : public TDialog {
	public:
		LocPrinterDialog() : TDialog(DLG_LOCPRN)
		{
			PPSetupCtrlMenu(this, CTL_LOCPRN_PORT, CTLMNU_LOCPRN_PORT, CTRLMENU_SELPRINTER);
		}
		int    setDTS(const PPLocPrinter * pData)
		{
			RVALUEPTR(Data, pData);
			setCtrlData(CTL_LOCPRN_NAME, Data.Name);
			setCtrlData(CTL_LOCPRN_SYMB, Data.Symb);
			setCtrlLong(CTL_LOCPRN_ID,   Data.ID);
			setCtrlData(CTL_LOCPRN_PORT, Data.Port);
			SetupPPObjCombo(this, CTLSEL_LOCPRN_LOCATION, PPOBJ_LOCATION, Data.LocID, OLW_CANINSERT, 0);
			AddClusterAssoc(CTL_LOCPRN_FLAGS, 0, PPLocPrinter::fHasKitchenBell);
			SetClusterData(CTL_LOCPRN_FLAGS, Data.Flags);
			return 1;
		}
		int    getDTS(PPLocPrinter * pData)
		{
			getCtrlData(CTL_LOCPRN_NAME, Data.Name);
			getCtrlData(CTL_LOCPRN_SYMB, Data.Symb);
			getCtrlData(CTL_LOCPRN_PORT, Data.Port);
			Data.LocID = getCtrlLong(CTLSEL_LOCPRN_LOCATION);
			Data.Flags = GetClusterData(CTL_LOCPRN_FLAGS);
			ASSIGN_PTR(pData, Data);
			return 1;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(TVKEYDOWN) {
				if(TVKEY == kbF2) {
					SString prn_port;
					if(SelectPrinterFromWinPool(prn_port) > 0)
						setCtrlString(CTL_LOCPRN_PORT, prn_port);
					clearEvent(event);
				}
			}
		}
		PPLocPrinter Data;
	};
	int    ok = 1, r = cmCancel, valid_data = 0, is_new = 0;
	ushort v = 0;
	LocPrinterDialog * p_dlg = 0;
	PPLocPrinter  loc_prn;
	THROW(CheckDialogPtr(&(p_dlg = new LocPrinterDialog)));
	THROW(EditPrereq(pID, p_dlg, &is_new));
	if(!is_new)
		THROW(GetPacket(*pID, &loc_prn) > 0);
	p_dlg->setDTS(&loc_prn);
	while(!valid_data && (r = ExecView(p_dlg)) == cmOK) {
		p_dlg->getDTS(&loc_prn);
		if(PutPacket(pID, &loc_prn, 1))
			valid_data = 1;
		else
			PPError();
	}
	CATCHZOKPPERR
	delete p_dlg;
	return ok ? r : 0;
}
//
// Implementation of PPALDD_LocPrnTest
//
PPALDD_CONSTRUCTOR(LocPrnTest)
{
	InitFixData(rscDefHdr,  &H, sizeof(H));
	InitFixData(rscDefIter, &I, sizeof(I));
}

PPALDD_DESTRUCTOR(LocPrnTest)
{
	Destroy();
}

int PPALDD_LocPrnTest::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = 1;
	PPLocPrinter * p_locprn = 0;
	if(rsrv)
		Extra[1].Ptr = p_locprn = (PPLocPrinter *)rFilt.Ptr;
	if(p_locprn) {
		H.LocID = p_locprn->LocID;
		STRNSCPY(H.Name, p_locprn->Name);
	}
	else
		ok = -1;
	return (ok > 0) ? DlRtm::InitData(rFilt, rsrv) : ok;
}

int PPALDD_LocPrnTest::InitIteration(PPIterID iterId, int sortId, long rsrv)
{
	IterProlog(iterId, 1);
	MEMSZERO(I);
	if(sortId >= 0)
		SortIdx = sortId;
	return 1;
}

#define TEST_ITERCOUNT 3

int PPALDD_LocPrnTest::NextIteration(PPIterID iterId, long rsrv)
{
	IterProlog(iterId, 0);
	if(I.IterNo < TEST_ITERCOUNT) {
		SString buf;
		I.IterNo++;
		PPGetWord(PPWORD_TEST, 0, buf);
		buf.Cat(I.IterNo);
		buf.CopyTo(I.TestIterText, sizeof(I.TestIterText));
	}
	else
		return -1;
	FINISH_PPVIEW_ALDD_ITER();
}

int PPALDD_LocPrnTest::Destroy()
{
	Extra[1].Ptr = 0;
	return 1;
}

int SLAPI PPObjLocPrinter::Browse(void * extraPtr)
{
	class LocPrinterView : public ObjViewDialog {
	public:
		LocPrinterView::LocPrinterView(PPObjLocPrinter * pObj, void * extraPtr) : ObjViewDialog(DLG_LOCPRNVIEW, pObj, extraPtr)
		{
		}
	private:
		DECL_HANDLE_EVENT
		{
			ObjViewDialog::handleEvent(event);
			if(event.isCmd(cmTest)) {
				PPLocPrinter loc_prn;
				PPID id = getCurrID();
				if(P_Obj->Search(getCurrID(), &loc_prn) > 0) {
					DS.GetTLA().PrintDevice = loc_prn.Port;
					uint   rpt_id = REPORT_LOCPRNTEST;
					PView  pv(&loc_prn);
					PPReportEnv env;
					env.PrnFlags = SReport::PrintingNoAsk;
					if(!PPAlddPrint(rpt_id, &pv, &env))
						PPError();
					DS.GetTLA().PrintDevice = 0;
				}
				clearEvent(event);
			}
		}
	};
	int    ok = 1;
	if(CheckRights(PPR_READ)) {
		TDialog * dlg = new LocPrinterView(this, extraPtr);
		if(CheckDialogPtr(&dlg, 1))
			ExecViewAndDestroy(dlg);
		else
			ok = 0;
	}
	else
		ok = PPErrorZ();
	return ok;
}

int SLAPI PPObjLocPrinter::GetPrinterByLocation(PPID locID, SString & rPrnPort, PPLocPrinter * pRec)
{
	int    ok = -1;
	PPLocPrinter lp_rec;
	rPrnPort = 0;
	for(SEnum en = ref->Enum(Obj, 0); ok < 0 && en.Next(&lp_rec) > 0;) {
		if(lp_rec.LocID == locID) {
			rPrnPort = lp_rec.Port;
			ASSIGN_PTR(pRec, lp_rec);
			ok = 1;
		}
	}
	return ok;
}

int SLAPI PPObjLocPrinter::GetLocPrnAssoc(LAssocArray & rList)
{
	int    ok = -1;
	rList.clear();
	PPLocPrinter lp_rec;
	for(SEnum en = ref->Enum(Obj, 0); en.Next(&lp_rec) > 0;) {
		rList.AddUnique(lp_rec.LocID, lp_rec.ID, 0);
		ok = 1;
	}
	return ok;
}

int SLAPI PPObjLocPrinter::IsPrinter()
{
	PPID   lp_id = 0;
	PPLocPrinter lp_rec;
	return BIN(EnumItems(&lp_id, &lp_rec) > 0);
}
//
// Implementation of PPALDD_CashNode
//
PPALDD_CONSTRUCTOR(CashNode)
{
	if(Valid)
		AssignHeadData(&H, sizeof(H));
}

PPALDD_DESTRUCTOR(CashNode)
{
	Destroy();
}

int PPALDD_CashNode::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		PPCashNode rec;
		if(SearchObject(PPOBJ_CASHNODE, rFilt.ID, &rec) > 0) {
			H.ID        = rec.ID;
			H.CashType  = rec.CashType;
			H.CurSessID = rec.CurSessID;
			H.CurDate   = rec.CurDate;
			H.LocID     = rec.LocID;
			STRNSCPY(H.Name, rec.Name);
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}
