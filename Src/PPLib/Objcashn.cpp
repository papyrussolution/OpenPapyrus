// OBJCASHN.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop

PPCashNode2::PPCashNode2()
{
	THISZERO();
}

PPGenCashNode::PosIdentEntry::PosIdentEntry() : N_(0)
{
}

int PPGenCashNode::PosIdentEntry::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
    int    ok = 1;
    THROW(pSCtx->Serialize(dir, N_, rBuf));
    THROW(pSCtx->Serialize(dir, Uuid, rBuf));
    THROW(pSCtx->Serialize(dir, Name, rBuf));
    CATCHZOK
    return ok;
}

PPGenCashNode::PPGenCashNode() : ID(0), CurRestBillID(0), CashType(0), DrvVerMajor(0), DrvVerMinor(0), DisRoundPrec(0), AmtRoundPrec(0),
	P_DivGrpList(0), LocID(0), ExtQuotID(0), Flags(0), ExtFlags(0), GoodsLocAssocID(0), ParentID(0), GoodsGrpID(0)
{
	memzero(Name, sizeof(Name));
	memzero(Symb, sizeof(Symb));
}

PPGenCashNode::~PPGenCashNode()
{
	ZDELETE(P_DivGrpList);
}

int FASTCALL PPGenCashNode::Copy(const PPGenCashNode & rS)
{
	ID = rS.ID;
	STRNSCPY(Name, rS.Name);
	STRNSCPY(Symb, rS.Symb);
	CurRestBillID = rS.CurRestBillID;
	CashType      = rS.CashType;
	DrvVerMajor   = rS.DrvVerMajor;
	DrvVerMinor   = rS.DrvVerMinor;
	DisRoundPrec  = rS.DisRoundPrec;
	AmtRoundPrec  = rS.AmtRoundPrec;
	LocID         = rS.LocID;
	ExtQuotID     = rS.ExtQuotID;
	Flags         = rS.Flags;
	ExtFlags      = rS.ExtFlags;
	GoodsLocAssocID = rS.GoodsLocAssocID;
	ParentID      = rS.ParentID;
	P_DivGrpList  = rS.P_DivGrpList ? new SArray(*rS.P_DivGrpList) : 0;
	TagL = rS.TagL; // @v9.6.5
	return 1;
}

PPGenCashNode & FASTCALL PPGenCashNode::operator = (const PPGenCashNode & rS)
{
	Copy(rS);
	return *this;
}

int PPGenCashNode::SetRoundParam(const RoundParam * pParam)
{
	int    ok = 1;
	DisRoundPrec = 0;
	AmtRoundPrec = 0;
	Flags &= ~(CASHF_DISROUNDUP | CASHF_DISROUNDDOWN);
	ExtFlags &= ~(CASHFX_ROUNDAMTUP | CASHFX_ROUNDAMTDOWN);
	if(pParam) {
		if(pParam->DisRoundPrec < 0.0 || pParam->DisRoundPrec > 50.0)
			ok = PPSetError(PPERR_INVROUNDPREC);
		else if(pParam->AmtRoundPrec < 0.0 || pParam->AmtRoundPrec > 100.0) // @v10.5.1 (>50.0)-->(>100.0)
			ok = PPSetError(PPERR_INVROUNDPREC);
		else {
			if(pParam->DisRoundPrec != 0.0) {
				DisRoundPrec = static_cast<uint16>(pParam->DisRoundPrec * 100.0);
				if(pParam->DisRoundDir > 0)
					Flags |= CASHF_DISROUNDUP;
				else if(pParam->DisRoundDir < 0)
					Flags |= CASHF_DISROUNDDOWN;
			}
			if(pParam->AmtRoundPrec != 0.0) {
				AmtRoundPrec = static_cast<uint16>(pParam->AmtRoundPrec * 100.0);
				if(pParam->AmtRoundDir > 0)
					ExtFlags |= CASHFX_ROUNDAMTUP;
				else if(pParam->AmtRoundDir < 0)
					ExtFlags |= CASHFX_ROUNDAMTDOWN;
			}
		}
//@erik v10.6.13 {
		if(pParam->IgnPennyFromBCardFlag){
			ExtFlags |= CASHFX_IGNPENNYFROMBCARD;
		}
		else {
			ExtFlags &= (~CASHFX_IGNPENNYFROMBCARD);
		}
// } @erik
	}
	return ok;
}

int PPGenCashNode::GetRoundParam(RoundParam * pParam) const
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
		pParam->IgnPennyFromBCardFlag = BIN(ExtFlags & CASHFX_IGNPENNYFROMBCARD); // @erik v10.6.13 
		ok = 1;
	}
	return ok;
}

void PPGenCashNode::DrvVerToStr(SString & rS) const
{
	rS.Z();
	if(DrvVerMajor >= 0 || DrvVerMinor >= 0)
        rS.Cat(DrvVerMajor).Dot().Cat(DrvVerMinor);
}

int PPGenCashNode::DrvVerFromStr(const char * pS)
{
	int    ok = 1;
	if(isempty(pS)) {
		DrvVerMajor = 0;
		DrvVerMinor = 0;
	}
	else {
		SString temp_buf(pS);
		STokenRecognizer tr;
		SNaturalTokenArray nta;
		tr.Run(temp_buf.Strip().ucptr(), -1, nta, 0);
		if(nta.Has(SNTOK_SOFTWAREVER) > 0.0f) {
			SString major, minor;
			temp_buf.Divide('.', major, minor);
			DrvVerMajor = static_cast<int16>(major.ToLong());
			DrvVerMinor = static_cast<int16>(minor.ToLong());
		}
		else {
			ok = PPSetError(PPERR_INVVERSIONTEXT, pS);
		}
	}
	return ok;
}
//
//
//
PPAsyncCashNode::PPAsyncCashNode() : PPGenCashNode()
{
}

PPAsyncCashNode & FASTCALL PPAsyncCashNode::operator = (const PPAsyncCashNode & rS)
{
	Copy(rS);
	return *this;
}

int FASTCALL PPAsyncCashNode::Copy(const PPAsyncCashNode & rS)
{
	int    ok = 1;
    PPGenCashNode::Copy(rS);
	ExpPaths = rS.ExpPaths;
	ImpFiles = rS.ImpFiles;
	LogNumList = rS.LogNumList;
	AddedMsgSign = rS.AddedMsgSign;
	TSCollection_Copy(ApnCorrList, rS.ApnCorrList);
	return ok;
}

int PPAsyncCashNode::GetLogNumList(PPIDArray & rList) const
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
		if((p = strtok(tempbuf, (char *)&sDELIM)) != 0) {
			do {
				if((n = atol(p)) > 0)
					rList.add(n);
			} while((p = strtok(0, (char *)&sDELIM)) != 0);
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

const PPGenCashNode::PosIdentEntry * FASTCALL PPAsyncCashNode::SearchPosIdentEntryByGUID(const S_GUID & rUuid) const
{
	const PPGenCashNode::PosIdentEntry * p_result = 0;
	if(!rUuid.IsZero()) {
		for(uint j = 0; !p_result && j < ApnCorrList.getCount(); j++) {
			const PPGenCashNode::PosIdentEntry * p_pie = ApnCorrList.at(j);
			if(p_pie && p_pie->Uuid == rUuid)
				p_result = p_pie;
		}
	}
	return p_result;
}

const PPGenCashNode::PosIdentEntry * FASTCALL PPAsyncCashNode::SearchPosIdentEntryByName(const char * pName) const
{
	const PPGenCashNode::PosIdentEntry * p_result = 0;
	if(!isempty(pName)) {
		for(uint j = 0; !p_result && j < ApnCorrList.getCount(); j++) {
			const PPGenCashNode::PosIdentEntry * p_pie = ApnCorrList.at(j);
			if(p_pie && p_pie->Name.CmpNC(pName) == 0)
				p_result = p_pie;
		}
	}
	return p_result;
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
	// @v9.7.10 PPID   PapyrusNodeID;   // ИД кассового узла Папирус
	PPID   AlternateRegID;  // @v9.7.10
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
	uint8  Reserve[56];     // @reserve @v9.7.5 [60]-->[56]
	int32  ScfFlags;        // @v9.7.5
	uint16 ScfDaysPeriod;       // Параметр фильтрации отложенных чеков
	int16  ScfDlvrItemsShowTag; // Параметр фильтрации отложенных чеков
	uint16 BonusMaxPart;    //
	uint16 Reserve3;        // @alignment
	int32  Reserve2;        // @reserve
};

PPSyncCashNode::SuspCheckFilt::SuspCheckFilt()
{
	THISZERO();
}

int PPSyncCashNode::SuspCheckFilt::IsEmpty() const
{
	return BIN(DaysPeriod == 0 && DlvrItemsShowTag == 0 && Flags == 0);
}

PPSyncCashNode::PPSyncCashNode() : PPGenCashNode(), DownBill(0), CurDate(ZERODATE), CurSessID(0), TouchScreenID(0), ExtCashNodeID(0),
	AlternateRegID(0), ScaleID(0), CustDispType(0), CustDispFlags(0), BnkTermType(0), BnkTermLogNum(0),
	BnkTermFlags(0), ClearCDYTimeout(0), EgaisMode(0), SleepTimeout(0), LocalTouchScrID(0)
{
	memzero(Port, sizeof(Port));
	// @v9.7.10 PapyrusNodeID_unused = 0;
	memzero(CustDispPort, sizeof(CustDispPort));
	memzero(BnkTermPort, sizeof(BnkTermPort));
}

int PPSyncCashNode::SetPropString(int propId, const char * pValue) { return PPPutExtStrData(propId, ExtString, pValue); }
int PPSyncCashNode::GetPropString(int propId, SString & rBuf) const { return PPGetExtStrData(propId, ExtString, rBuf); }

SString & PPSyncCashNode::CTblListToString(SString & rBuf) const
{
	rBuf.Z();
	LongArray temp_list = CTblList;
	const uint c = temp_list.getCount();
	if(c) {
		temp_list.sort();
		for(uint i = 0; i < c;) {
			rBuf.CatDivIfNotEmpty(',', 2).Cat(temp_list.get(i));
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

int PPSyncCashNode::CTblListFromString(const char * pBuf)
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
/*static*/int PPObjCashNode::IsExtCashNode(PPID nodeID, PPID * pParentID)
{
	Reference * p_ref = PPRef;
	int    is_ext_cash_node = 0;
	PPID   parent_id = 0;
	PPCashNode cn_rec;
	for(PPID cn_id = 0; !is_ext_cash_node && p_ref->EnumItems(PPOBJ_CASHNODE, &cn_id, &cn_rec) > 0;) {
		__PPExtDevices  ed;
		if(cn_rec.Flags & CASHF_SYNC && p_ref->GetProperty(PPOBJ_CASHNODE, cn_id, CNPRP_EXTDEVICES, &ed, sizeof(ed)) > 0 && ed.ExtCashNodeID == nodeID) {
			parent_id = ed.CashNodeID;
			is_ext_cash_node = 1;
		}
	}
	ASSIGN_PTR(pParentID, parent_id);
	return is_ext_cash_node;
}

/*static*/int PPObjCashNode::IsLocked(PPID id)
{
	int    ok = -1;
	PPID   parent_id = 0;
	PPSyncItem sync_item;
	PPSync & r_sync = DS.GetSync();
	if(!IsExtCashNode(id, &parent_id) && r_sync.CreateMutex_(LConfig.SessionID, PPOBJ_CASHNODE, id, 0, &sync_item) > 0) {
		r_sync.ReleaseMutex(PPOBJ_CASHNODE, id);
		ok = 0;
	}
	else if(!r_sync.IsMyLock(LConfig.SessionID, PPOBJ_CASHNODE, id) &&
		(!parent_id || !r_sync.IsMyLock(LConfig.SessionID, PPOBJ_CASHNODE, parent_id)))
		ok = (PPSetError(PPERR_CASHNODE_LOCKED, sync_item.Name), 1);
	return ok;
}

/*static*/int PPObjCashNode::Lock(PPID id)
{
	int    ok = -1;
	int    no_locking = 0;
	PPIniFile ini_file;
	PPSyncItem sync_item;
	ini_file.GetInt(PPINISECT_SYSTEM, PPINIPARAM_CASHNODE_NOLOCKING, &no_locking);
	if(!no_locking)
		ok = (DS.GetSync().CreateMutex_(LConfig.SessionID, PPOBJ_CASHNODE, id, 0, &sync_item) > 0) ? 1 : PPSetError(PPERR_CASHNODE_LOCKED, sync_item.Name);
	return ok;
}

/*static*/int PPObjCashNode::Unlock(PPID id)
{
	SString  added_msg_str = DS.GetTLA().AddedMsgString;
	int    ok = DS.GetSync().ReleaseMutex(PPOBJ_CASHNODE, id);
	PPSetAddedMsgString(added_msg_str);
	return ok;
}
//
//
//
StrAssocArray * PPObjCashNode::MakeStrAssocList(void * extraPtr)
{
	SelFilt f;
	if(!RVALUEPTR(f, static_cast<const SelFilt *>(extraPtr)))
		MEMSZERO(f);
	StrAssocArray * p_ary = new StrAssocArray;
	THROW_MEM(p_ary);
	{
		PPIDArray parent_list;
		PPCashNode cn_rec;
		for(SEnum en = P_Ref->Enum(Obj, 0); en.Next(&cn_rec) > 0;) {
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
				if(!p_ary->Search(parent_id)) {
					PPCashNode cn_rec;
					if(Fetch(parent_id, &cn_rec) > 0)
						name_buf = cn_rec.Name;
					else
						ideqvalstr(parent_id, name_buf.Z());
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

/*static*/PPID PPObjCashNode::Select(PPID locID, int syncGroup, int * pIsSingle, int isAny)
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
		id = p_ary->Get(0).Id;
		THROW_PP(id != PPCMT_OKA500, PPERR_OKA500NOTSUPPORTED);
		ASSIGN_PTR(pIsSingle, 1);
		ZDELETE(p_ary);
	}
	else {
		SString title;
		THROW(lw = new ListWindow(new StrAssocListBoxDef(p_ary, lbtDisposeData|lbtDblClkNotify), 0, 0));
		THROW(PPLoadText(PPTXT_SELECTCASHNODE, title));
		lw->setTitle(title);
		lw->ViewOptions |= (ofCenterX | ofCenterY);
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

/*static*/const int PPObjCashNode::SubstCTblID = 999; // Специализированный идентификатор стола, применяемый для замещения не определенного списка столов. =999

/*static*/int  PPObjCashNode::GetCafeTableName(int ctblN, SString & rBuf)
{
	int    ok = 0;
	rBuf.Z();
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

PPObjCashNode::SelFilt::SelFilt() : LocID(0), SyncGroup(0), OnlyGroups(-1), ParentID(0), Flags(0)
{
}

PPObjCashNode::PPObjCashNode(void * extraPtr) : PPObjReference(PPOBJ_CASHNODE, extraPtr)
{
	ImplementFlags |= (implStrAssocMakeList | implTreeSelector);
}

int PPObjCashNode::DeleteObj(PPID id)
{
	const PPCashNode * p_cn_rec = reinterpret_cast<const PPCashNode *>(&P_Ref->data);
	if(!(p_cn_rec->Flags & CASHF_DAYCLOSED) && p_cn_rec->CurDate)
		return PPSetError(PPERR_DAYNOTCLOSED);
	else
		return PPObjReference::DeleteObj(id);
}

int  PPObjCashNode::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx) // @srlz
{
	int    ok = 1;
	if(p && p->Data) {
		if(stream == 0) {
			PPCashNode * p_rec = static_cast<PPCashNode *>(p->Data);
			if(*pID == 0) {
				PPID   same_id = 0;
				if((p_rec->ID && p_rec->ID < PP_FIRSTUSRREF) ||
					P_Ref->SearchSymb(Obj, &same_id, p_rec->Symb, offsetof(PPCashNode, Symb)) > 0) {
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
					ASSIGN_PTR(pID, P_Ref->data.ObjID);
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
			THROW(Serialize_(+1, static_cast<ReferenceTbl::Rec *>(p->Data), stream, pCtx));
		}
	}
	CATCHZOK
	return ok;
}

int PPObjCashNode::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	int    ok = -1;
	if(p && p->Data) {
		PPCashNode * p_rec = static_cast<PPCashNode *>(p->Data); // PPCashNode2
		ProcessObjRefInArray(PPOBJ_LOCATION, &p_rec->LocID, ary, replace);
		ProcessObjRefInArray(PPOBJ_QUOTKIND, &p_rec->ExtQuotID, ary, replace);
		ProcessObjRefInArray(PPOBJ_GOODSGROUP, &p_rec->GoodsGrpID, ary, replace);
		ProcessObjRefInArray(PPOBJ_CASHNODE, &p_rec->ParentID, ary, replace);
		ok = 1;
	}
	return ok;
}

int PPObjCashNode::Get(PPID id, PPGenCashNode * pGCN, PPCashNode * pCN)
{
	int    r;
	PPCashNode cn_rec;
	if((r = P_Ref->GetItem(PPOBJ_CASHNODE, id, &cn_rec)) > 0) {
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
		if(P_Ref->GetPropArray(Obj, id, CNPRP_DIVGRPASSC, &temp_list) > 0 && temp_list.getCount())
			pGCN->P_DivGrpList = new SArray(temp_list);
		if(pCN)
			memcpy(pCN, &cn_rec, sizeof(PPCashNode));
		P_Ref->Ot.GetList(Obj, id, &pGCN->TagL);
		r = 1;
	}
	return r;
}

int PPObjCashNode::GetSync(PPID id, PPSyncCashNode * pSCN)
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
		if(P_Ref->GetPropActualSize(Obj, id, CNPRP_EXTDEVICES, &ed_size) > 0) {
			THROW_MEM(p_ed = static_cast<__PPExtDevices *>(SAlloc::M(ed_size)));
			memzero(p_ed, ed_size);
			if(P_Ref->GetProperty(Obj, id, CNPRP_EXTDEVICES, p_ed, ed_size) > 0) {
				pSCN->TouchScreenID = p_ed->TouchScreenID;
				pSCN->ExtCashNodeID = p_ed->ExtCashNodeID;
				pSCN->AlternateRegID = p_ed->AlternateRegID;
				pSCN->ScaleID       = p_ed->ScaleID;
				pSCN->CustDispType  = p_ed->CustDispType;
				STRNSCPY(pSCN->CustDispPort, p_ed->CustDispPort);
				pSCN->CustDispFlags = p_ed->CustDispFlags;
				pSCN->ClearCDYTimeout = p_ed->ClearCDYTimeout;
				if(pSCN->ClearCDYTimeout < 0 || pSCN->ClearCDYTimeout > 60)
					pSCN->ClearCDYTimeout = 0;
				// @v9.0.9 {
				pSCN->EgaisMode    = p_ed->EgaisMode;
				if(!oneof4(pSCN->EgaisMode, 0, 1, 2, 3))
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
			// @v9.7.10 pSCN->PapyrusNodeID_unused = 0;
			pSCN->AlternateRegID  = 0; // @v9.7.10
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
			pSCN->EgaisMode       = 0;
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
			if(P_Ref->GetProperty(Obj, id, CNPRP_EXTRA, &pnext, sizeof(pnext)) > 0) {
				pSCN->Scf.DaysPeriod = pnext.ScfDaysPeriod;
				pSCN->Scf.DlvrItemsShowTag = pnext.ScfDlvrItemsShowTag;
				pSCN->Scf.Flags = pnext.ScfFlags;
				pSCN->BonusMaxPart = pnext.BonusMaxPart;
				// @v10.9.6 {
				if(pSCN->BonusMaxPart == 1)
					pSCN->BonusMaxPart = 1000;
				// } @v10.9.6 
			}
			else {
				pSCN->Scf.DaysPeriod = 0;
				pSCN->Scf.DlvrItemsShowTag = 0;
				pSCN->BonusMaxPart = 0;
			}
		}
		pSCN->CTblList.clear();
		P_Ref->GetPropArray(Obj, id, CNPRP_CTBLLIST, &pSCN->CTblList);
		pSCN->LocalTouchScrID = 0;
		{
			WinRegKey reg_key(HKEY_LOCAL_MACHINE, PPRegKeys::PrefSettings, 1/*readonly*/);
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
	SAlloc::F(p_ed);
	return ok;
}

int PPObjCashNode::GetAsync(PPID id, PPAsyncCashNode * pACN)
{
	int    ok = 1, r;
	PPCashNode cn_rec;
	pACN->ImpFiles.Z();
	pACN->ExpPaths.Z();
	pACN->LogNumList.Z();
	ZDELETE(pACN->P_DivGrpList);
	pACN->ApnCorrList.freeAll();
	if((r = Get(id, pACN, &cn_rec)) > 0) {
		SString temp_buf;
		if(!(cn_rec.Flags & CASHF_EXTFRM349)) {
			THROW(r = P_Ref->GetPropVlrString(PPOBJ_CASHNODE, id, CNPRP_IMPFILES, temp_buf));
			if(r > 0 && temp_buf.NotEmptyS())
				pACN->ImpFiles = temp_buf;
			THROW(r = P_Ref->GetPropVlrString(PPOBJ_CASHNODE, id, CNPRP_EXPPATHS, temp_buf));
			if(r > 0 && temp_buf.NotEmptyS())
				pACN->ExpPaths = temp_buf;
		}
		else if(P_Ref->GetPropVlrString(Obj, id, CNPRP_EXTSTR, temp_buf) > 0) {
			PPGetExtStrData(ACN_EXTSTR_FLD_IMPFILES, temp_buf, pACN->ImpFiles);
			PPGetExtStrData(ACN_EXTSTR_FLD_EXPPATHS, temp_buf, pACN->ExpPaths);
			PPGetExtStrData(ACN_EXTSTR_FLD_LOGNUMS,  temp_buf, pACN->LogNumList);
			PPGetExtStrData(ACN_EXTSTR_FLD_ADDEDMSGSIGN, temp_buf, pACN->AddedMsgSign);
		}
		{
			SBuffer sbuf;
			if(P_Ref->GetPropSBuffer(Obj, id, CNPRP_APNCORRLIST, sbuf) > 0) {
				SSerializeContext sctx;
				THROW(TSCollection_Serialize(pACN->ApnCorrList, -1, sbuf, &sctx));
			}
		}
	}
	else
		ok = r;
	CATCHZOK
	return ok;
}

int PPObjCashNode::GetListByLoc(PPID locID, PPIDArray & rList)
{
	int    ok = -1;
	PPCashNode cn_rec;
	for(SEnum en = P_Ref->Enum(Obj, 0); en.Next(&cn_rec) > 0;) {
		if(cn_rec.LocID == locID) {
			rList.addUnique(cn_rec.ID);
			ok = 1;
		}
	}
	return ok;
}

int PPObjCashNode::GetListByGroup(PPID grpID, PPIDArray & rList)
{
	int    ok = -1;
	PPCashNode cn_rec;
	for(SEnum en = P_Ref->Enum(Obj, 0); en.Next(&cn_rec) > 0;) {
		if(cn_rec.ParentID == grpID) {
			rList.addUnique(cn_rec.ID);
			ok = 1;
		}
	}
	return ok;
}

int PPObjCashNode::Helper_ResolveItem(PPID id, PPIDArray & rDestList, LAssocArray & rFullList)
{
	int    ok = -1;
	PPCashNode cn_rec;
	if(id && !rDestList.lsearch(id) && Fetch(id, &cn_rec) > 0) {
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
	return ok;
}

int PPObjCashNode::ResolveList(const PPIDArray * pSrcList, PPIDArray & rDestList)
{
	int    ok = -1;
	rDestList.clear();
	if(pSrcList) {
		const uint _c = pSrcList->getCount();
		if(_c) {
			LAssocArray full_list;
			PPCashNode cn_rec;
			for(SEnum en = P_Ref->Enum(Obj, 0); en.Next(&cn_rec) > 0;) {
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

int PPObjCashNode::GetTaxSystem(PPID id, LDATE dt, PPID * pTaxSysID)
{
	int    ok = -1;
	PPID   tax_sys_id = TAXSYSK_GENERAL;
	const  LDATE actual_date = checkdate(dt) ? dt : getcurdate_();
	PPObjPerson psn_obj;
	RegisterTbl::Rec reg_rec;
	if(id) {
        PPCashNode cn_rec;
		if(Fetch(id, &cn_rec) > 0) {
			// @v10.6.12 {
			ObjTagList tag_list;
			P_Ref->Ot.GetList(PPOBJ_CASHNODE, id, &tag_list);
			for(uint tagidx = 0; ok < 0 && tagidx < tag_list.GetCount(); tagidx++) {
				const ObjTagItem * p_tag_item = tag_list.GetItemByPos(tagidx);
				if(p_tag_item && p_tag_item->TagDataType == OTTYP_OBJLINK && p_tag_item->TagEnumID == PPOBJ_TAXSYSTEMKIND) {
					PPID   temp_id = 0;
					if(p_tag_item->GetInt(&temp_id) && temp_id && SearchObject(PPOBJ_TAXSYSTEMKIND, temp_id, 0) > 0) {
						tax_sys_id = temp_id;
						ok = 1;
					}
				}
			}
			// } @v10.6.12 
			if(ok < 0 && cn_rec.LocID && psn_obj.LocObj.GetRegister(cn_rec.LocID, PPREGT_TAXSYSTEM, actual_date, 0, &reg_rec) > 0 && reg_rec.ExtID > 0) {
				tax_sys_id = reg_rec.ExtID;
				ok = 1;
			}
		}
	}
	if(ok < 0) {
        PPID   main_org_id = 0;
        GetMainOrgID(&main_org_id);
        if(main_org_id && psn_obj.GetRegister(main_org_id, PPREGT_TAXSYSTEM, actual_date, &reg_rec) > 0 && reg_rec.ExtID > 0) {
			tax_sys_id = reg_rec.ExtID;
			ok = 1;
		}
	}
	ASSIGN_PTR(pTaxSysID, tax_sys_id);
	return ok;
}

int PPObjCashNode::IsVatFree(PPID id)
{
	int    result = -1;
	PPObjPerson psn_obj;
	PersonTbl::Rec psn_rec;
	LocationTbl::Rec loc_rec;
	if(id) {
        PPCashNode cn_rec;
		if(Fetch(id, &cn_rec) > 0 && cn_rec.LocID) {
			if(psn_obj.LocObj.Fetch(cn_rec.LocID, &loc_rec) > 0 && loc_rec.Flags & LOCF_VATFREE)
				result = 1;
		}
	}
	if(result < 0) {
        PPID   main_org_id = 0;
        GetMainOrgID(&main_org_id);
        if(main_org_id && psn_obj.Fetch(main_org_id, &psn_rec) > 0 && psn_rec.Flags & PSNF_NOVATAX)
			result = 1;
	}
	return result;
}

int PPObjCashNode::Put(PPID * pID, PPGenCashNode * pCN, int use_ta)
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
	THROW_INVARG(pCN != 0);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID) {
			THROW(P_Ref->GetItem(PPOBJ_CASHNODE, *pID, &rec) > 0);
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
			p_scn = static_cast<PPSyncCashNode *>(pCN);
			rec.SleepTimeout = p_scn->SleepTimeout;
			rec.DownBill     = p_scn->DownBill;
			rec.CurDate      = p_scn->CurDate;
			rec.CurSessID    = p_scn->CurSessID;
			rec.Speciality   = p_scn->Speciality;
			memcpy(rec.Port,  p_scn->Port,  sizeof(rec.Port));
		}
		THROW(EditItem(PPOBJ_CASHNODE, *pID, &rec, 0));
		*pID = rec.ID;
		THROW(P_Ref->PutPropArray(Obj, *pID, CNPRP_DIVGRPASSC, pCN->P_DivGrpList, 0));
		THROW(P_Ref->Ot.PutList(Obj, *pID, &pCN->TagL, 0)); // @v9.6.5
		if(!oneof2(pCN->CashType, PPCMT_CASHNGROUP, PPCMT_DISTRIB)) {
			if(f & CASHF_SYNC) {
				p_scn = static_cast<PPSyncCashNode *>(pCN);
				//p_scn->ExtString = p_ed->ExtStrBuf;
				p_scn->SetPropString(SCN_PRINTERPORT,        p_scn->PrinterPort);
				p_scn->SetPropString(SCN_CAFETABLE_DGR_PATH, p_scn->TableSelWhatman);
				p_scn->SetPropString(SCN_BNKTERMPATH,        p_scn->BnkTermPath);
				p_scn->SetPropString(SCN_SLIPFMTPATH,        p_scn->SlipFmtPath);
				{
					const size_t ed_size = ALIGNSIZE(offsetof(__PPExtDevices, ExtStrBuf) + p_scn->ExtString.Len() + 1, 2);
					THROW_MEM(p_ed = static_cast<__PPExtDevices *>(SAlloc::M(ed_size)));
					memzero(p_ed, ed_size);

					p_ed->TouchScreenID = p_scn->TouchScreenID;
					p_ed->ExtCashNodeID = p_scn->ExtCashNodeID;
					// @v9.7.10 p_ed->PapyrusNodeID = 0; // @v9.6.9 p_scn->PapyrusNodeID-->0
					p_ed->AlternateRegID = p_scn->AlternateRegID; // @v9.7.10
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
						/*p_ed->PapyrusNodeID*/p_ed->AlternateRegID || p_ed->ScaleID || p_ed->PhnSvcID || p_ed->ExtStrBuf[0] || p_ed->EgaisMode) {
						THROW(P_Ref->PutProp(Obj, *pID, CNPRP_EXTDEVICES, p_ed, ed_size, 0));
					}
					else {
						THROW(P_Ref->PutProp(Obj, *pID, CNPRP_EXTDEVICES, 0, ed_size, 0));
					}
				}
				{
					__PosNodeExt pnext;
					if(!p_scn->Scf.IsEmpty() || p_scn->BonusMaxPart) {
						MEMSZERO(pnext);
						pnext.ScfDaysPeriod = p_scn->Scf.DaysPeriod;
						pnext.ScfDlvrItemsShowTag = p_scn->Scf.DlvrItemsShowTag;
						pnext.ScfFlags = p_scn->Scf.Flags; // @v9.7.5
						pnext.BonusMaxPart = p_scn->BonusMaxPart;
						THROW(P_Ref->PutProp(Obj, *pID, CNPRP_EXTRA, &pnext, sizeof(pnext)));
					}
					else {
						THROW(P_Ref->PutProp(Obj, *pID, CNPRP_EXTRA, 0, sizeof(pnext)));
					}
				}
				THROW(P_Ref->PutPropArray(Obj, *pID, CNPRP_CTBLLIST, &p_scn->CTblList, 0));
				{
					WinRegKey reg_key(HKEY_LOCAL_MACHINE, PPRegKeys::PrefSettings, 0);
					SString param_buf;
					(param_buf = "LocalTouchScreen").CatChar(':').Cat(*pID);
					if(reg_key.GetString(param_buf, temp_buf) && p_scn->LocalTouchScrID == 0) {
						temp_buf.Z().Cat(0);
						reg_key.PutString(param_buf, temp_buf);
					}
					else if(p_scn->LocalTouchScrID) {
						temp_buf.Z().Cat(p_scn->LocalTouchScrID);
						reg_key.PutString(param_buf, temp_buf);
					}
				}
			}
			else if(f & CASHF_ASYNC) {
				p_acn = static_cast<PPAsyncCashNode *>(pCN);
				temp_buf.Z();
				PPPutExtStrData(ACN_EXTSTR_FLD_IMPFILES, temp_buf, p_acn->ImpFiles);
				PPPutExtStrData(ACN_EXTSTR_FLD_EXPPATHS, temp_buf, p_acn->ExpPaths);
				PPPutExtStrData(ACN_EXTSTR_FLD_LOGNUMS,  temp_buf, p_acn->LogNumList);
				PPPutExtStrData(ACN_EXTSTR_FLD_ADDEDMSGSIGN, temp_buf, p_acn->AddedMsgSign);
				THROW(P_Ref->PutPropVlrString(Obj, *pID, CNPRP_EXTSTR, temp_buf));
				{
					SBuffer sbuf;
					if(p_acn->ApnCorrList.getCount()) {
						SSerializeContext sctx;
						THROW(TSCollection_Serialize(p_acn->ApnCorrList, +1, sbuf, &sctx));
					}
                    THROW(P_Ref->PutPropSBuffer(Obj, *pID, CNPRP_APNCORRLIST, sbuf, 0));
				}
			}
		}
		THROW(tra.Commit());
	}
	if(!is_new)
		Dirty(*pID);
	CATCHZOK
	SAlloc::F(p_ed);
	return ok;
}
//
//   DivGrpAsscListDialog
//
class DivGrpAsscListDialog : public PPListDialog {
public:
	DivGrpAsscListDialog() : PPListDialog(DLG_DIVGRPASSC, CTL_DIVGRPASSC_LIST), P_Data(0), PotentialyInvalid(0)
	{
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
						const LAssoc * p_temp_item = static_cast<const LAssoc *>(temp_list.at(i));
						PPGenCashNode::DivGrpAssc new_item;
						new_item.GrpID = p_temp_item->Key;
						new_item.DivN = static_cast<short>(p_temp_item->Val);
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
	if(CheckDialogPtrErr(&dlg)) {
		SetupPPObjCombo(dlg, CTLSEL_DIVGRPASSCITM_GRP, PPOBJ_GOODSGROUP, temp_item.GrpID, OLW_CANINSERT|OLW_CANSELUPLEVEL, 0); // @v10.0.02 OLW_CANSELUPLEVEL
		dlg->setCtrlData(CTL_DIVGRPASSCITM_DIV, &temp_item.DivN);
		for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
			int  all_ok = 1;
			dlg->getCtrlData(CTLSEL_DIVGRPASSCITM_GRP, &temp_item.GrpID);
			dlg->getCtrlData(CTL_DIVGRPASSCITM_DIV, &temp_item.DivN);
			PPGenCashNode::DivGrpAssc * p_item;
			for(uint i = 0; all_ok && P_Data->enumItems(&i, (void **)&p_item);)
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
	for(uint i = 0; ok && P_Data->enumItems(&i, (void **)&p_item);) {
		Goods2Tbl::Rec gg_rec;
		char   sub[64];
		if(p_item->GrpID == 0) {
			sub[0] = '0';
			sub[1] = 0;
		}
		else if(gg_obj.Fetch(p_item->GrpID, &gg_rec) > 0)
			STRNSCPY(sub, gg_rec.Name);
		else {
			// @v10.3.0 (always true) if(p_item->GrpID != 0)
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
	if(pos >= 0 && pos < (long)P_Data->getCount() && editItem((int)pos, (PPGenCashNode::DivGrpAssc *)P_Data->at(static_cast<uint>(pos))) > 0)
		return 1;
	return -1;
}

int DivGrpAsscListDialog::delItem(long pos, long /*id*/)
{
	return P_Data->atFree(static_cast<uint>(pos)) ? 1 : -1;
}
//
// ExtDevicesDialog
//
int SelectPrinterFromWinPool(SString & rPrinter)
{
	int    ok = 1;
	long   sel_prn_id = 0, def_prn_id = 0;
	SString prn_port, temp_buf;
	TSVector <SPrinting::PrnInfo> prn_list;
	StrAssocArray * p_list = new StrAssocArray;
	ListWindow * p_lw = 0;
	THROW_MEM(p_list);
	SPrinting::GetListOfPrinters(&prn_list);
	p_list->Z();
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
		temp_buf.Z().Cat(prn_list.at(j).PrinterName).ToOem();
		p_list->Add(j+1, temp_buf);
	}
	{
		int    valid_data = 0;
		SString title;
		THROW(p_lw = new ListWindow(new StrAssocListBoxDef(p_list, lbtDisposeData|lbtDblClkNotify), 0, 0));
		THROW(PPLoadText(PPTXT_SELECTPRINTER, title));
		p_lw->setTitle(title);
		p_lw->ViewOptions |= (ofCenterX | ofCenterY);
		while(!valid_data && ExecView(p_lw) == cmOK) {
			p_lw->getResult(&sel_prn_id);
			if(sel_prn_id && sel_prn_id <= prn_list.getCountI())
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
			PPSetupCtrlMenu(this, CTL_EXTDEV_RPTPRNPORT, CTLMNU_EXTDEV_RPTPRNPORT, CTRLMENU_SELPRINTER);
			FileBrowseCtrlGroup::Setup(this, CTLBRW_EXTDEV_HOSTICURL, CTL_EXTDEV_HOSTICURL, 1, 0, 0,
				FileBrowseCtrlGroup::fbcgfFile|FileBrowseCtrlGroup::fbcgfPath);
		}
		int    setDTS(const PPSyncCashNode * pData)
		{
			if(!RVALUEPTR(Data, pData))
				MEMSZERO(Data);
			SString temp_buf;
			SetupPPObjCombo(this, CTLSEL_EXTDEV_TCHSCREEN, PPOBJ_TOUCHSCREEN, Data.TouchScreenID, OLW_CANINSERT, 0);
			SetupPPObjCombo(this, CTLSEL_EXTDEV_LOCTCHSCR, PPOBJ_TOUCHSCREEN, Data.LocalTouchScrID, OLW_CANINSERT, 0);
			SetupPPObjCombo(this, CTLSEL_EXTDEV_CASHNODE,  PPOBJ_CASHNODE, Data.ExtCashNodeID, 0, 0);
			SetupPPObjCombo(this, CTLSEL_EXTDEV_ALTREG,    PPOBJ_CASHNODE, Data.AlternateRegID, 0, 0);
			AddClusterAssoc(CTL_EXTDEV_EXTNODEASALT, 0, CASHFX_EXTNODEASALT);
			SetClusterData(CTL_EXTDEV_EXTNODEASALT, Data.ExtFlags);
			DisableClusterItem(CTL_EXTDEV_EXTNODEASALT, 0, (!Data.ExtCashNodeID || Data.AlternateRegID));
			SetupPPObjCombo(this, CTLSEL_EXTDEV_SCALE,     PPOBJ_SCALE, Data.ScaleID, 0, 0);
			SetupPPObjCombo(this, CTLSEL_EXTDEV_PHNSVC,    PPOBJ_PHONESERVICE, Data.PhnSvcID, 0, 0);
			setCtrlString(CTL_EXTDEV_PRINTER, Data.PrinterPort);
			Data.GetPropString(SCN_RPTPRNPORT, temp_buf);
			setCtrlString(CTL_EXTDEV_RPTPRNPORT, temp_buf);
			Data.GetPropString(SCN_CASHDRAWER_PORT, temp_buf);
			setCtrlString(CTL_EXTDEV_DRAWERPORT, temp_buf);
			Data.GetPropString(SCN_CASHDRAWER_CMD, temp_buf);
			setCtrlString(CTL_EXTDEV_DRAWERCMD, temp_buf);

			Data.GetPropString(SCN_KITCHENBELL_PORT, temp_buf);
			setCtrlString(CTL_EXTDEV_KBELLPORT, temp_buf);
			Data.GetPropString(SCN_KITCHENBELL_CMD, temp_buf);
			setCtrlString(CTL_EXTDEV_KBELLCMD, temp_buf);
			Data.GetPropString(SCN_MANUFSERIAL, temp_buf);
			setCtrlString(CTL_EXTDEV_MANUFSERIAL, temp_buf);
			AddClusterAssoc(CTL_EXTDEV_EGAISMODE,  0, 1);
			AddClusterAssocDef(CTL_EXTDEV_EGAISMODE, 1, 0);
			AddClusterAssoc(CTL_EXTDEV_EGAISMODE,  2, 2);
			AddClusterAssoc(CTL_EXTDEV_EGAISMODE,  3, 3);
			SetClusterData(CTL_EXTDEV_EGAISMODE, Data.EgaisMode);
			AddClusterAssoc(CTL_EXTDEV_CHKEGMUNIQ, 0, CASHFX_CHECKEGAISMUNIQ); // @v10.1.1
			AddClusterAssoc(CTL_EXTDEV_CHKEGMUNIQ, 1, CASHFX_BNKSLIPAFTERRCPT); // @v10.9.11
			SetClusterData(CTL_EXTDEV_CHKEGMUNIQ, Data.ExtFlags); // @v10.1.1
			Data.GetPropString(ACN_EXTSTR_FLD_IMPFILES, temp_buf);
			setCtrlString(CTL_EXTDEV_HOSTICURL, temp_buf);
			Data.DrvVerToStr(temp_buf); // @v10.0.03
			setCtrlString(CTL_EXTDEV_DRVVER, temp_buf); // @v10.0.03
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
			getCtrlData(CTLSEL_EXTDEV_ALTREG,    &Data.AlternateRegID);
			if(Data.ExtCashNodeID && !Data.AlternateRegID)
				GetClusterData(CTL_EXTDEV_EXTNODEASALT, &Data.ExtFlags);
			else
				Data.ExtFlags &= ~CASHFX_EXTNODEASALT;
			getCtrlData(CTLSEL_EXTDEV_SCALE,     &Data.ScaleID);
			getCtrlData(CTLSEL_EXTDEV_PHNSVC,    &Data.PhnSvcID);
			getCtrlString(CTL_EXTDEV_PRINTER, Data.PrinterPort);
			getCtrlString(CTL_EXTDEV_RPTPRNPORT, temp_buf);
			Data.SetPropString(SCN_RPTPRNPORT, temp_buf);
			if(Data.ExtCashNodeID) {
				THROW(cn_obj.Search(Data.ExtCashNodeID, &cn_rec) > 0);
				sel = CTL_EXTDEV_CASHNODE;
				THROW_PP(Data.ID != Data.ExtCashNodeID && PPCashMachine::IsSyncCMT(cn_rec.CashType), PPERR_INVCMT);
			}
			getCtrlString(CTL_EXTDEV_DRAWERPORT, temp_buf);
			Data.SetPropString(SCN_CASHDRAWER_PORT, temp_buf);
			getCtrlString(CTL_EXTDEV_DRAWERCMD, temp_buf);
			Data.SetPropString(SCN_CASHDRAWER_CMD, temp_buf);

			getCtrlString(CTL_EXTDEV_KBELLPORT, temp_buf);
			Data.SetPropString(SCN_KITCHENBELL_PORT, temp_buf);
			getCtrlString(CTL_EXTDEV_KBELLCMD, temp_buf);
			Data.SetPropString(SCN_KITCHENBELL_CMD, temp_buf);
			getCtrlString(CTL_EXTDEV_MANUFSERIAL, temp_buf);
			Data.SetPropString(SCN_MANUFSERIAL, temp_buf);
			getCtrlString(CTL_EXTDEV_HOSTICURL, temp_buf);
			Data.SetPropString(ACN_EXTSTR_FLD_IMPFILES, temp_buf);
			Data.EgaisMode = (int16)GetClusterData(CTL_EXTDEV_EGAISMODE);
			GetClusterData(CTL_EXTDEV_CHKEGMUNIQ, &Data.ExtFlags); // @v10.1.1
			getCtrlString(sel = CTL_EXTDEV_DRVVER, temp_buf); // @v10.0.03
			THROW(Data.DrvVerFromStr(temp_buf)); // @v10.0.03
			ASSIGN_PTR(pData, Data);
			CATCHZOKPPERRBYDLG
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
			else if(event.isCbSelected(CTLSEL_EXTDEV_CASHNODE) || event.isCbSelected(CTLSEL_EXTDEV_ALTREG)) {
				PPID   ext_node_id = 0;
				PPID   alt_reg_id = 0;
				getCtrlData(CTLSEL_EXTDEV_CASHNODE,  &ext_node_id);
				getCtrlData(CTLSEL_EXTDEV_ALTREG,    &alt_reg_id);
				DisableClusterItem(CTL_EXTDEV_EXTNODEASALT, 0, (!ext_node_id || alt_reg_id));
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
					CATCHZOKPPERRBYDLG
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
	if(CheckDialogPtrErr(&dlg)) {
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
//@erik v10.6.13 {
		dlg->setCtrlData(CTL_IGNPANNYFROMBCARD, &param.IgnPennyFromBCardFlag);
// } @erik
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
			param.IgnPennyFromBCardFlag = dlg->getCtrlUInt16(CTL_IGNPANNYFROMBCARD); //@erik v10.6.13
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
	if(CheckDialogPtrErr(&dlg)) {
		SString ctbl_list_buf;
		long   dlvr_items_show_tag = 0; //DlvrItemsShowTag
		if(Data.Scf.DlvrItemsShowTag > 0)
			dlvr_items_show_tag = 1;
		else if(Data.Scf.DlvrItemsShowTag < 0)
			dlvr_items_show_tag = -1;
		dlg->setCtrlLong(CTL_CASHNSEXT_SCF_DAYS, (long)Data.Scf.DaysPeriod);
		dlg->AddClusterAssocDef(CTL_CASHNSEXT_SCF_DLVRT, 0, 0);
		dlg->AddClusterAssoc(CTL_CASHNSEXT_SCF_DLVRT, 1, +1);
		dlg->AddClusterAssoc(CTL_CASHNSEXT_SCF_DLVRT, 2, -1);
		dlg->SetClusterData(CTL_CASHNSEXT_SCF_DLVRT, dlvr_items_show_tag);
		dlg->AddClusterAssoc(CTL_CASHNSEXT_NOTSF, 0, Data.Scf.fNotSpFinished); // @v9.7.5
		dlg->SetClusterData(CTL_CASHNSEXT_NOTSF, Data.Scf.Flags); // @v9.7.5
		dlg->setCtrlReal(CTL_CASHNSEXT_BONUSMAX, (double)Data.BonusMaxPart / 10.0);
		dlg->setCtrlString(CTL_CASHNSEXT_CTBLLIST, Data.CTblListToString(ctbl_list_buf));
		dlg->AddClusterAssoc(CTL_CASHNSEXT_FLAGS, 0, CASHFX_INPGUESTCFTBL);
		dlg->SetClusterData(CTL_CASHNSEXT_FLAGS, Data.ExtFlags);
		while(ok < 0 && ExecView(dlg) == cmOK) {
			long   days_period = dlg->getCtrlLong(CTL_CASHNSEXT_SCF_DAYS);
			if(days_period < 0 || days_period > 1000) {
				PPErrorByDialog(dlg, CTL_CASHNSEXT_SCF_DAYS, PPERR_USERINPUT);
			}
			else {
				dlg->getCtrlString(CTL_CASHNSEXT_CTBLLIST, ctbl_list_buf);
				if(!Data.CTblListFromString(ctbl_list_buf)) {
					PPErrorByDialog(dlg, CTL_CASHNSEXT_CTBLLIST);
				}
				else {
					Data.BonusMaxPart = static_cast<uint16>(dlg->getCtrlReal(CTL_CASHNSEXT_BONUSMAX) * 10.0);
					if(Data.BonusMaxPart < 0 || Data.BonusMaxPart > 1000)
						Data.BonusMaxPart = 0;
					Data.Scf.DaysPeriod = (uint16)days_period;
					Data.Scf.DlvrItemsShowTag = (int16)dlg->GetClusterData(CTL_CASHNSEXT_SCF_DLVRT);
					dlg->GetClusterData(CTL_CASHNSEXT_FLAGS, &Data.ExtFlags);
					dlg->GetClusterData(CTL_CASHNSEXT_NOTSF, &Data.Scf.Flags);
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
		AddClusterAssoc(CTL_CASHN_EXTFLAGS, 11, 0x0800); // @v10.8.1 CASHFX_NOTIFYEQPTIMEMISM

		long   ef = 0;
		SETFLAG(ef, 0x0001, Data.Flags & CASHF_UNIFYGDSATCHECK);
		SETFLAG(ef, 0x0002, Data.Flags & CASHF_UNIFYGDSTOPRINT);
		SETFLAG(ef, 0x0004, Data.Flags & CASHF_NOTUSECHECKCUTTER);
		SETFLAG(ef, 0x0008, Data.ExtFlags & CASHFX_SELSERIALBYGOODS);
		SETFLAG(ef, 0x0010, Data.ExtFlags & CASHFX_FORCEDIVISION);
		SETFLAG(ef, 0x0020, Data.Flags & CASHF_ABOVEZEROSALE);
		SETFLAG(ef, 0x0040, Data.ExtFlags & CASHFX_EXTSCARDSEL);
		SETFLAG(ef, 0x0080, Data.ExtFlags & CASHFX_KEEPORGCCUSER);
		SETFLAG(ef, 0x0100, Data.ExtFlags & CASHFX_DISABLEZEROSCARD);
		SETFLAG(ef, 0x0200, Data.ExtFlags & CASHFX_UHTTORDIMPORT);
		SETFLAG(ef, 0x0400, Data.ExtFlags & CASHFX_ABSTRGOODSALLOWED);
		SETFLAG(ef, 0x0800, Data.ExtFlags & CASHFX_NOTIFYEQPTIMEMISM); // @v10.8.1
		SetClusterData(CTL_CASHN_EXTFLAGS, ef);
		{
			PPAlbatrossConfig acfg;
			PPAlbatrosCfgMngr::Get(&acfg);
			SString temp_buf;
			acfg.GetExtStrData(ALBATROSEXSTR_UHTTACC, temp_buf);
			DisableClusterItem(CTL_CASHN_EXTFLAGS, 9, /*acfg.UhttAccount.Empty()*/temp_buf.IsEmpty());
		}
	}
	AddClusterAssoc(CTL_CASHN_PASSIVE, 0, CASHFX_PASSIVE);
	SetClusterData(CTL_CASHN_PASSIVE, Data.ExtFlags);
	//disableCtrl(CTL_CASHN_OVRFLW, 1);
	setCtrlData(CTL_CASHN_SLEEPTIMEOUT, &Data.SleepTimeout);
	if(!Data.TableSelWhatman.NotEmptyS()) {
		FileBrowseCtrlGroup * p_fbg = static_cast<FileBrowseCtrlGroup *>(getGroup(GRP_TBLDGMPATH));
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
	Data.Speciality = static_cast<uint16>(getCtrlLong(CTLSEL_CASHN_SPECIALITY));
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
		SETFLAG(Data.ExtFlags, CASHFX_DISABLEZEROSCARD,  ef & 0x0100);
		SETFLAG(Data.ExtFlags, CASHFX_UHTTORDIMPORT,     ef & 0x0200);
		SETFLAG(Data.ExtFlags, CASHFX_ABSTRGOODSALLOWED, ef & 0x0400);
		SETFLAG(Data.ExtFlags, CASHFX_NOTIFYEQPTIMEMISM, ef & 0x0800); // @v10.8.1
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
	CATCHZOKPPERRBYDLG
	return ok;
}

int SyncCashNodeCfgDialog::EditCashParam()
{
	int    ok = -1;
	if(Data.ID) {
		PPCashMachine * p_cm = PPCashMachine::CreateInstance(Data.ID);
		PPSyncCashSession * p_si = p_cm ? p_cm->SyncInterface() : 0;
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
	else if(event.isCmd(cmTags)) { // @v9.6.5
		Data.TagL.ObjType = PPOBJ_CASHNODE;
		EditObjTagValList(&Data.TagL, 0);
	}
	else if(event.isClusterClk(CTL_CASHN_FLAGS))
		SetupCtrls();
	else if(event.isCbSelected(CTLSEL_CASHN_NOA))
		DisableClusterItem(CTL_CASHN_NOA_PRINTONLY, 0, !getCtrlLong(CTLSEL_CASHN_NOA));
	else if(event.isCmd(cmEditCashParam))
		EditCashParam();
	else if(event.isCmd(cmDiagnostics)) { // @v10.5.12
		PPObjCashNode cn_obj;
		if(getDTS(0)) {
			StringSet ss;
			if(cn_obj.DiagnoseNode(Data, ss) > 0) {
				if(ss.getCount()) {
					TDialog * dlg = new TDialog(DLG_DIAGPOSNODE);
					if(CheckDialogPtrErr(&dlg)) {
						SString temp_buf;
						SString info_buf;
						for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
							info_buf.Cat(temp_buf).CRB();
						} 
						dlg->setStaticText(CTL_DIAGPOSNODE_RESULT, info_buf);
						ExecViewAndDestroy(dlg);
					}
				}
			}
		}
	}
	else
		return;
	clearEvent(event);
}

int PPObjCashNode::EditSync(PPSyncCashNode * pSCN) { DIALOG_PROC_BODY(SyncCashNodeCfgDialog, pSCN); }

int PPObjCashNode::DiagnoseNode(const PPGenCashNode & rNode, StringSet & rSsResult)
{
	int    ok = -1;
	rSsResult.clear();
	if(PPCashMachine::IsSyncCMT(rNode.CashType)) {
		if(rNode.ID) {
			PPCashMachine * p_cm = PPCashMachine::CreateInstance(rNode.ID);
			if(p_cm && p_cm->SyncDiagnose(&rSsResult) > 0 && rSsResult.getCount()) {
				ok = 1;
			}
			delete p_cm;
		}
	}
	else if(PPCashMachine::IsAsyncCMT(rNode.CashType)) {
	}
	return ok;
}

int PPObjCashNode::EditGroup(PPGenCashNode * pGroup)
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
			CATCHZOKPPERRBYDLG
			return ok;
		}
	private:
		PPGenCashNode Data;
	};
	DIALOG_PROC_BODY(GroupCashNodeCfgDialog, pGroup);
}

int PPObjCashNode::EditDistrib(PPGenCashNode * pData)
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
			CATCHZOKPPERRBYDLG
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
class ApnCorrListDialog : public PPListDialog {
	DECL_DIALOG_DATA(PPAsyncCashNode);
public:
	ApnCorrListDialog() : PPListDialog(DLG_APNCORLIST, CTL_APNCORLIST_LIST)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		int    ok = 1;
		RVALUEPTR(Data, pData);
		updateList(-1);
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		ASSIGN_PTR(pData, Data);
		return ok;
	}
private:
	virtual int setupList()
	{
		int     ok = -1;
		SString buf;
		StringSet ss(SLBColumnDelim);
		if(Data.ApnCorrList.getCount()) {
			for(uint i = 0; i < Data.ApnCorrList.getCount(); i++) {
				PPGenCashNode::PosIdentEntry * p_entry = Data.ApnCorrList.at(i);
				if(p_entry) {
					ss.clear();
					ss.add(buf.Z().Cat(p_entry->N_));
					ss.add(p_entry->Name);
					p_entry->Uuid.ToStr(S_GUID::fmtIDL, buf);
					ss.add(buf);
					THROW(addStringToList(i+1, ss.getBuf()));
					ok = 1;
				}
			}
		}
		CATCHZOK
		return ok;
	}
	virtual int addItem(long * pPos, long * pID)
	{
		int    ok = -1;
		PPGenCashNode::PosIdentEntry entry;
		while(ok < 0 && Helper_EditItem(entry) > 0) {
			if(VerifyEntry(UINT_MAX, entry)) {
				uint new_pos = 0;
				PPGenCashNode::PosIdentEntry * p_new_entry = Data.ApnCorrList.CreateNewItem(&new_pos);
				if(p_new_entry) {
					*p_new_entry = entry;
					ASSIGN_PTR(pPos, new_pos);
					ASSIGN_PTR(pID, new_pos+1);
					ok = 1;
				}
				else {
					PPSetErrorSLib();
					ok = PPErrorZ();
				}
			}
			else
				ok = 0;
		}
		return ok;
	}
	virtual int editItem(long pos, long id)
	{
		int    ok = -1;
		if(id > 0 && id <= Data.ApnCorrList.getCountI()) {
			PPGenCashNode::PosIdentEntry * p_entry = Data.ApnCorrList.at(id-1);
			if(p_entry) {
				PPGenCashNode::PosIdentEntry temp_entry = *p_entry;
				int    err = 1;
				while(err && Helper_EditItem(temp_entry) > 0) {
					if(VerifyEntry(id-1, temp_entry)) {
						*p_entry = temp_entry;
						err = 0;
					}
				}
			}
		}
		return ok;
	}
	virtual int delItem(long pos, long id)
	{
		int    ok = -1;
		if(id > 0 && id <= Data.ApnCorrList.getCountI()) {
			Data.ApnCorrList.atFree((uint)(id-1));
			ok = 1;
		}
		return ok;
	}
	int    VerifyEntry(uint pos, const PPGenCashNode::PosIdentEntry & rEntry)
	{
		int    ok = 1;
		for(uint i = 0; i < Data.ApnCorrList.getCount(); i++) {
			const PPGenCashNode::PosIdentEntry * p_test_entry = Data.ApnCorrList.at(i);
			if(i != pos) {
				THROW_PP(!p_test_entry || p_test_entry->N_ != rEntry.N_, PPERR_APNCORR_N_DUP);
				THROW_PP(rEntry.Uuid.IsZero() || rEntry.Uuid != p_test_entry->Uuid, PPERR_APNCORR_UUID_DUP);
			}
		}
		CATCH
			ok = PPErrorZ();
		ENDCATCH
		return ok;
	}
	int    Helper_EditItem(PPGenCashNode::PosIdentEntry & rEntry)
	{
		int    ok = -1;
		SString temp_buf;
        TDialog * dlg = new TDialog(DLG_APNCORITEM);
        if(CheckDialogPtrErr(&dlg)) {
            dlg->setCtrlLong(CTL_APNCORITEM_N, rEntry.N_);
            dlg->setCtrlString(CTL_APNCORITEM_NAME, rEntry.Name);
			rEntry.Uuid.ToStr(S_GUID::fmtIDL, temp_buf);
            dlg->setCtrlString(CTL_APNCORITEM_UUID, temp_buf);
            while(ok < 0 && ExecView(dlg) == cmOK) {
                rEntry.N_ = dlg->getCtrlLong(CTL_APNCORITEM_N);
				if(rEntry.N_ > 0) {
					dlg->getCtrlString(CTL_APNCORITEM_NAME, rEntry.Name);
					rEntry.Name.Strip();
					dlg->getCtrlString(CTL_APNCORITEM_UUID, temp_buf);
					if(rEntry.Uuid.FromStr(temp_buf)) {
						ok = 1;
					}
					else
						PPError(PPERR_SLIB, temp_buf);
				}
				else
					PPError(PPERR_APNCORR_N_INV, 0);
            }
        }
        delete dlg;
        return ok;
	}
};

class AsyncCashNodeDialog : public TDialog {
	enum {
		dummyFirst = 1,
		brushValidPath,
		brushInvalidPath
	};
public:
	explicit AsyncCashNodeDialog(PPAsyncCashNode * pData) : TDialog(DLG_CASHNA), P_Data(pData)
	{
		Ptb.SetBrush(brushValidPath,   SPaintObj::bsSolid, GetColorRef(SClrAqua),  0); // @v10.9.11
		Ptb.SetBrush(brushInvalidPath, SPaintObj::bsSolid, GetColorRef(SClrCoral), 0); // @v10.9.11
		PPIniFile  ini_file;
		ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_ACSCLOSE_USEALTIMPORT, &UseAltImport);
		FileBrowseCtrlGroup::Setup(this, CTLBRW_CASHN_IMPFILES, CTL_CASHN_IMPFILES, 1, 0, 0, FileBrowseCtrlGroup::fbcgfPath);
		FileBrowseCtrlGroup::Setup(this, CTLBRW_CASHN_EXPPATHS, CTL_CASHN_EXPPATHS, 2, 0, 0, FileBrowseCtrlGroup::fbcgfPath);
	}
private:
	DECL_HANDLE_EVENT;
	int    EditDivGrpAssoc(SArray *);
	int    EditApnCorrList();
	PPAsyncCashNode * P_Data; // class AsyncCahsNodeDialog not owns P_Data
	int    UseAltImport;      //
	SPaintToolBox Ptb;
};

int AsyncCashNodeDialog::EditDivGrpAssoc(SArray * pList) { DIALOG_PROC_BODY(DivGrpAsscListDialog, pList); }
int AsyncCashNodeDialog::EditApnCorrList() { DIALOG_PROC_BODY(ApnCorrListDialog, P_Data); }

IMPL_HANDLE_EVENT(AsyncCashNodeDialog)
{
	TDialog::handleEvent(event);
	if(event.isCmd(cmDivGrpAssc)) {
		SArray div_grp_list(sizeof(PPGenCashNode::DivGrpAssc));
		if(P_Data->P_DivGrpList)
			div_grp_list.copy(*P_Data->P_DivGrpList);
		if(EditDivGrpAssoc(&div_grp_list) > 0)
			if(P_Data->P_DivGrpList)
				P_Data->P_DivGrpList->copy(div_grp_list);
			else
				P_Data->P_DivGrpList = new SArray(div_grp_list);
	}
	else if(event.isCmd(cmApnCorrList)) {
		EditApnCorrList();
	}
	else if(event.isCmd(cmInputUpdated) && event.isCtlEvent(CTL_CASHN_DEVICE)) {
		const PPID cash_type = getCtrlLong(CTLSEL_CASHN_DEVICE);
		showCtrl(CTL_CASHN_IMPPARAM, BIN(cash_type == PPCMT_CRCSHSRV && UseAltImport));
	}
	else if(event.isCmd(cmCtlColor)) { // @v10.9.11
		TDrawCtrlData * p_dc = static_cast<TDrawCtrlData *>(TVINFOPTR);
		if(p_dc) {
			static const uint16 ctl_list[] = { CTL_CASHN_IMPFILES, CTL_CASHN_EXPPATHS };
			SString input_buf;
			SString temp_buf;
			for(uint i = 0; i < SIZEOFARRAY(ctl_list); i++) {
				const uint16 ctl_id = ctl_list[i];
				if(p_dc->H_Ctl == getCtrlHandle(ctl_id)) {
					getCtrlString(ctl_id, input_buf);
					int    local_result = -1; // -1 - empty, 0 - path unavailable, 1 - path available
					if(input_buf.NotEmpty()) {
						StringSet ss(';', input_buf);
						for(uint ssp = 0; local_result != 0 && ss.get(&ssp, temp_buf);) {
							temp_buf.Strip().RmvLastSlash();
							local_result = (IsDirectory(temp_buf) || fileExists(temp_buf)) ? 1 : 0;
						}
					}
					const int tool_id = (local_result > 0) ? brushValidPath : ((local_result == 0) ? brushInvalidPath : 0);
					if(tool_id) {
						::SetBkMode(p_dc->H_DC, TRANSPARENT);
						p_dc->H_Br = static_cast<HBRUSH>(Ptb.Get(tool_id));
					}
				}
			}
			clearEvent(event);
		}		
	}
	else if(event.isCmd(cmImpParam)) {
		// @v9.1.3 EditCSessImpExpParams(1);
	}
	else if(event.isCmd(cmTags)) {
		P_Data->TagL.ObjType = PPOBJ_CASHNODE;
		EditObjTagValList(&P_Data->TagL, 0);
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
						PPID   bill_id = static_cast<const BillFilt *>(bill_view.GetBaseFilt())->Sel;
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

int PPObjCashNode::Validate(PPGenCashNode * pRec, long)
{
	int    ok = 1;
	THROW_PP(*strip(pRec->Name) != 0, PPERR_NAMENEEDED);
	THROW(CheckDupName(pRec->ID, pRec->Name));
	THROW(P_Ref->CheckUniqueSymb(Obj, pRec->ID, pRec->Symb, offsetof(PPCashNode, Symb)));
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

int PPObjCashNode::EditAsync(PPAsyncCashNode * pACN)
{
	int    ok = -1;
	int    r = cmCancel;
	int    valid_data = 0;
	PPIDArray test_log_num_list;
	SString temp_buf;
	PPObjLocation loc_obj;
	AsyncCashNodeDialog * dlg = 0;
	THROW(CheckDialogPtr(&(dlg = new AsyncCashNodeDialog(pACN))));
	dlg->setCtrlData(CTL_CASHN_NAME, pACN->Name);
	dlg->setCtrlData(CTL_CASHN_SYMB, pACN->Symb);
	dlg->setCtrlLong(CTL_CASHN_ID, pACN->ID);
	// @vmiller SetupStringCombo(dlg, CTLSEL_CASHN_DEVICE, PPTXT_CMT, pACN->CashType, 0);
	SetupStringComboDevice(dlg, CTLSEL_CASHN_DEVICE, DVCCLS_SYNCPOS, pACN->CashType, 0); // @vmiller
	// @v10.0.03 dlg->setCtrlData(CTL_CASHN_DRVVERMAJOR, &pACN->DrvVerMajor);
	// @v10.0.03 dlg->setCtrlData(CTL_CASHN_DRVVERMINOR, &pACN->DrvVerMinor);
	pACN->DrvVerToStr(temp_buf); // @v10.0.03
	dlg->setCtrlString(CTL_CASHN_DRVVER, temp_buf); // @v10.0.03
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
		CASHFX_CREATEOBJSONIMP|CASHFX_SEPARATERCPPRN|CASHFX_IGNLOOKBACKPRICES|CASHFX_IGNCONDQUOTS));
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
	dlg->AddClusterAssoc(CTL_CASHN_FLAGS, 11, CASHFX_IGNLOOKBACKPRICES);
	dlg->AddClusterAssoc(CTL_CASHN_FLAGS, 12, CASHFX_IGNCONDQUOTS); // @v10.0.03
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
		temp_buf.Z();
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
		// @v10.0.03 dlg->getCtrlData(CTL_CASHN_DRVVERMAJOR, &pACN->DrvVerMajor);
		// @v10.0.03 dlg->getCtrlData(CTL_CASHN_DRVVERMINOR, &pACN->DrvVerMinor);
		dlg->getCtrlString(CTL_CASHN_DRVVER, temp_buf);
		if(!pACN->DrvVerFromStr(temp_buf)) {
			PPErrorByDialog(dlg, CTL_CASHN_DRVVER);
		}
		else {
			dlg->getCtrlData(CTLSEL_CASHN_EXTQUOT, &pACN->ExtQuotID);
			dlg->getCtrlData(CTLSEL_CASHN_LOC, &pACN->LocID);
			dlg->getCtrlData(CTLSEL_CASHN_NOA, &pACN->GoodsLocAssocID);
			dlg->getCtrlString(CTL_CASHN_IMPFILES, pACN->ImpFiles);
			dlg->getCtrlString(CTL_CASHN_EXPPATHS, pACN->ExpPaths);
			dlg->getCtrlString(CTL_CASHN_LOGNUMS, pACN->LogNumList);
			if(!pACN->GetLogNumList(test_log_num_list)) {
				PPErrorByDialog(dlg, CTL_CASHN_LOGNUMS);
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
								r2 = PPErrorByDialog(dlg, CTLSEL_CASHN_NOA);
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
								pACN->Flags = (temp_flags & ~(CASHFX_EXPLOCPRNASSOC|CASHFX_APPLYUNITRND|CASHFX_RESTRUSERGGRP|
									CASHFX_RMVPASSIVEGOODS|CASHFX_CREATEOBJSONIMP|CASHFX_SEPARATERCPPRN|
									CASHFX_IGNLOOKBACKPRICES|CASHFX_IGNCONDQUOTS));
								pACN->Flags |= CASHF_ASYNC;
								SETFLAGBYSAMPLE(pACN->ExtFlags, CASHFX_EXPLOCPRNASSOC,  temp_flags);
								SETFLAGBYSAMPLE(pACN->ExtFlags, CASHFX_APPLYUNITRND,    temp_flags);
								SETFLAGBYSAMPLE(pACN->ExtFlags, CASHFX_RESTRUSERGGRP,   temp_flags);
								SETFLAGBYSAMPLE(pACN->ExtFlags, CASHFX_RMVPASSIVEGOODS, temp_flags);
								SETFLAGBYSAMPLE(pACN->ExtFlags, CASHFX_CREATEOBJSONIMP, temp_flags);
								SETFLAGBYSAMPLE(pACN->ExtFlags, CASHFX_SEPARATERCPPRN,  temp_flags);
								SETFLAGBYSAMPLE(pACN->ExtFlags, CASHFX_IGNLOOKBACKPRICES, temp_flags);
								SETFLAGBYSAMPLE(pACN->ExtFlags, CASHFX_IGNCONDQUOTS, temp_flags); // @v10.0.03
								dlg->GetClusterData(CTL_CASHN_PASSIVE, &pACN->ExtFlags);
								ok = valid_data = 1;
							}
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

int PPObjCashNode::Edit(PPID * pID, void * extraPtr)
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

int ViewCashNodes()
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
	CashNodeCache() : ObjCache(PPOBJ_CASHNODE, sizeof(Data))
	{
	}
private:
	virtual int  FetchEntry(PPID, ObjCacheEntry * pEntry, long);
	virtual void EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
public:
	struct Data : public ObjCacheEntry {
		PPID   CashType;
		PPID   ExtQuotID;
		PPID   LocID;
		PPID   ParentID;
		long   Flags;
		long   ExtFlags;
		uint16 Speciality; // @v10.4.5
		uint16 Reserve;    // @v10.4.5 @alignment
	};
};

int CashNodeCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, long)
{
	int    ok = 1;
	Data * p_cache_rec = static_cast<Data *>(pEntry);
	PPObjCashNode cn_obj;
	PPCashNode rec;
	if(cn_obj.Search(id, &rec) > 0) {
#define CPY_FLD(Fld) p_cache_rec->Fld=rec.Fld
		CPY_FLD(CashType);
		CPY_FLD(ExtQuotID);
		CPY_FLD(LocID);
		CPY_FLD(ParentID);
		CPY_FLD(Flags);
		CPY_FLD(ExtFlags);
		CPY_FLD(Speciality); // @v10.4.5
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

void CashNodeCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	PPCashNode * p_data_rec = static_cast<PPCashNode *>(pDataRec);
	const Data * p_cache_rec = static_cast<const Data *>(pEntry);
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
	CPY_FLD(Speciality); // @v10.4.5
#undef CPY_FLD
	MultTextBlock b(this, pEntry);
	b.Get(p_data_rec->Name, sizeof(p_data_rec->Name));
	b.Get(p_data_rec->Symb, sizeof(p_data_rec->Symb));
}

IMPL_OBJ_FETCH(PPObjCashNode, PPCashNode, CashNodeCache);
//
//
//
PPEquipConfig::PPEquipConfig()
{
	THISZERO();
}

PPID PPEquipConfig::GetCashierTabNumberRegTypeID() const
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

int ReadEquipConfig(PPEquipConfig * pCfg)
{
	int    use_scale_input = 0;
	{
		WinRegKey reg_key(HKEY_CURRENT_USER, PPRegKeys::SysSettings, 1);
		uint32 val = 0;
		if(reg_key.GetDWord(RpCheckScaleInput, &val))
			use_scale_input = BIN(val);
	}
	int    r = PPRef->GetPropMainConfig(PPPRP_EQUIPCFG, pCfg, sizeof(*pCfg));
	if(pCfg)
		if(r > 0) {
			SETFLAG(pCfg->Flags, PPEquipConfig::fCheckScaleInput, use_scale_input);
		}
		else
			memzero(pCfg, sizeof(*pCfg));
	return r;
}

class EquipConfigDialog : public TDialog {
	DECL_DIALOG_DATA(PPEquipConfig);
public:
	EquipConfigDialog() : TDialog(DLG_EQUIPCFG)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		PPIDArray op_type_list;
		if(!RVALUEPTR(Data, pData))
			MEMSZERO(Data);
		SetupPPObjCombo(this, CTLSEL_EQCFG_PSNKNDCSHRS, PPOBJ_PRSNKIND, Data.CshrsPsnKindID, 0, 0);
		SetupPPObjCombo(this, CTLSEL_EQCFG_DEFCASHNODE, PPOBJ_CASHNODE, Data.DefCashNodeID, 0, 0);
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
		SetupPPObjCombo(this, CTLSEL_EQCFG_PHNSVC, PPOBJ_PHONESERVICE, Data.PhnSvcID, 0);
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
		AddClusterAssoc(CTL_EQCFG_FLAGS, 10, PPEquipConfig::fUnifiedPaymentCfmBank);
		AddClusterAssoc(CTL_EQCFG_FLAGS, 11, PPEquipConfig::fIgnoreNoDisGoodsTag);
		AddClusterAssoc(CTL_EQCFG_FLAGS, 12, PPEquipConfig::fRestrictQttyByUnitRnd);
		AddClusterAssoc(CTL_EQCFG_FLAGS, 13, PPEquipConfig::fDisableManualSCardInput);
		AddClusterAssoc(CTL_EQCFG_FLAGS, 14, PPEquipConfig::fDisableAdjWrOffAmount);
		SetClusterData(CTL_EQCFG_FLAGS, Data.Flags);

		AddClusterAssoc(CTL_EQCFG_FLAGS2, 0, PPEquipConfig::fUseQuotAsPrice);
		AddClusterAssoc(CTL_EQCFG_FLAGS2, 1, PPEquipConfig::fUncondAsyncBasePrice);
		AddClusterAssoc(CTL_EQCFG_FLAGS2, 2, PPEquipConfig::fAutosaveSyncChecks);
		AddClusterAssoc(CTL_EQCFG_FLAGS2, 3, PPEquipConfig::fWrOffPartStrucs);
		AddClusterAssoc(CTL_EQCFG_FLAGS2, 4, PPEquipConfig::fSkipPrintingZeroPrice); // @v10.0.12
		AddClusterAssoc(CTL_EQCFG_FLAGS2, 5, PPEquipConfig::fAttachBillChecksToCSess); // @v10.9.9
		SetClusterData(CTL_EQCFG_FLAGS2, Data.Flags);
		SetupCtrls();
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		uint   sel = 0;
		getCtrlData(CTLSEL_EQCFG_WROFFACCOP,  &Data.WrOffAccOpID);
		getCtrlData(CTLSEL_EQCFG_PSNKNDCSHRS, &Data.CshrsPsnKindID);
		getCtrlData(CTLSEL_EQCFG_DEFCASHNODE, &Data.DefCashNodeID);
		getCtrlData(CTLSEL_EQCFG_OPDTHISLOC,  &Data.OpOnDfctThisLoc);
		getCtrlData(CTLSEL_EQCFG_OPDOTHRLOC,  &Data.OpOnDfctOthrLoc);
		getCtrlData(CTLSEL_EQCFG_TEMPSESSOP,  &Data.OpOnTempSess);
		getCtrlData(CTLSEL_EQCFG_QUOT,        &Data.QuotKindID);
		getCtrlData(CTLSEL_EQCFG_PHNSVC,      &Data.PhnSvcID);
		GetClusterData(CTL_EQCFG_FLAGS,  &Data.Flags);
		GetClusterData(CTL_EQCFG_FLAGS2, &Data.Flags);
		ASSIGN_PTR(pData, Data);
		//CATCHZOKPPERRBYDLG
		return ok;
	}
private:
	DECL_HANDLE_EVENT;
	void   SetupCtrls();
	int    EditExtParams();
};

int EquipConfigDialog::EditExtParams()
{
	class ExtEquipConfigDialog : public TDialog {
		DECL_DIALOG_DATA(PPEquipConfig);
	public:
		ExtEquipConfigDialog() : TDialog(DLG_EQCFGEXT)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			int    ok = 1;
			RVALUEPTR(Data, pData);
			SetupPPObjCombo(this, CTLSEL_EQCFG_FTPACCT, PPOBJ_INTERNETACCOUNT, Data.FtpAcctID, 0, 
				reinterpret_cast<void *>(PPObjInternetAccount::filtfFtp)/*INETACCT_ONLYFTP*/);
			SetupPPObjCombo(this, CTLSEL_EQCFG_SALESGRP, PPOBJ_GOODSGROUP, Data.SalesGoodsGrp, OLW_CANSELUPLEVEL, reinterpret_cast<void *>(GGRTYP_SEL_ALT));
			setCtrlData(CTL_EQCFG_AGENTCODELEN, &Data.AgentCodeLen);
			setCtrlData(CTL_EQCFG_AGENTPREFIX,  &Data.AgentPrefix);
			setCtrlData(CTL_EQCFG_SUSPCPFX, Data.SuspCcPrefix);
			{
				RealRange subst_range;
				SetRealRangeInput(this, CTL_EQCFG_DFCTCOSTRNG, &(subst_range = Data.DeficitSubstPriceDevRange).Scale(0.1), 1);
			}
			{
				double rng_lim = fdiv100i(Data.BHTRngLimWgtGoods);
				setCtrlData(CTL_EQCFG_RNGLIMGOODSBHT, &rng_lim);
				rng_lim = fdiv100i(Data.BHTRngLimPrice);
				setCtrlData(CTL_EQCFG_RNGLIMPRICEBHT, &rng_lim);
			}
			setCtrlLong(CTL_EQCFG_LOOKBKPRCPRD, Data.LookBackPricePeriod);
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			uint   sel = 0;
			getCtrlData(CTLSEL_EQCFG_FTPACCT,     &Data.FtpAcctID);
			getCtrlData(sel = CTLSEL_EQCFG_SALESGRP,    &Data.SalesGoodsGrp);
			if(Data.SalesGoodsGrp) {
				PPObjGoodsGroup ggobj;
				Goods2Tbl::Rec ggrec;
				// @v10.6.8 @ctr MEMSZERO(ggrec);
				ggobj.Search(Data.SalesGoodsGrp, &ggrec);
				//sel = CTL_EQCFG_SALESGRP;
				THROW_PP(ggrec.Flags & GF_EXCLALTFOLD, PPERR_INVSALESGRP);
			}
			getCtrlData(sel = CTL_EQCFG_AGENTCODELEN, &Data.AgentCodeLen);
			getCtrlData(sel = CTL_EQCFG_AGENTPREFIX,  &Data.AgentPrefix);
			getCtrlData(sel = CTL_EQCFG_SUSPCPFX, Data.SuspCcPrefix);
			{
				RealRange subst_range;
				GetRealRangeInput(this, CTL_EQCFG_DFCTCOSTRNG, &subst_range);
				if(!subst_range.IsZero()) {
					const double _srl = subst_range.low;
					if(_srl == subst_range.upp)
						subst_range.Set(-fabs(_srl), +fabs(_srl));
					subst_range.Scale(10.0);
					Data.DeficitSubstPriceDevRange.Set(static_cast<int>(subst_range.low), static_cast<int>(subst_range.upp));
				}
				else
					Data.DeficitSubstPriceDevRange.Set(0);
			}
			{
				double rng_lim = 0.0;
				getCtrlData(sel = CTL_EQCFG_RNGLIMGOODSBHT, &rng_lim);
				THROW_PP(rng_lim >= 0.0 && rng_lim <= 100.0, PPERR_PERCENTINPUT);
				Data.BHTRngLimWgtGoods = (long)(rng_lim * 100.0);
				getCtrlData(sel = CTL_EQCFG_RNGLIMPRICEBHT, &(rng_lim = 0));
				THROW_PP(rng_lim >= 0 && rng_lim <= 100, PPERR_PERCENTINPUT);
				Data.BHTRngLimPrice = (long)(rng_lim * 100.0);
			}
			{
				Data.LookBackPricePeriod = getCtrlLong(sel = CTL_EQCFG_LOOKBKPRCPRD);
				THROW_PP(checkirange(Data.LookBackPricePeriod, 0, (365*2)), PPERR_USERINPUT); // @v10.8.1 365-->(365*2)
			}
			const int prefix_len = sstrlen(Data.AgentPrefix);
			THROW_PP(Data.AgentCodeLen >= 0 && (!prefix_len || prefix_len < Data.AgentCodeLen), PPERR_USERINPUT);
			ASSIGN_PTR(pData, Data);
			CATCHZOKPPERRBYDLG
			return ok;
		}
	};
	DIALOG_PROC_BODY(ExtEquipConfigDialog, &Data);
}

void EquipConfigDialog::SetupCtrls()
{
	const PPID quotk = getCtrlLong(CTLSEL_EQCFG_QUOT);
	DisableClusterItem(CTL_EQCFG_FLAGS, 6, quotk);
	if(quotk) {
		const long flags = GetClusterData(CTL_EQCFG_FLAGS);
		SetClusterData(CTL_EQCFG_FLAGS, (flags & ~PPEquipConfig::fIntrPriceByRetailRules));
	}
}

IMPL_HANDLE_EVENT(EquipConfigDialog)
{
	TDialog::handleEvent(event);
	if(event.isCbSelected(CTLSEL_EQCFG_QUOT))
		SetupCtrls();
	else if(event.isCmd(cmExtParams))
		EditExtParams();
	else
		return;
	clearEvent(event);
}

int EditEquipConfig()
{
	int    ok = -1;
	int    is_new = 0;
	EquipConfigDialog * dlg = 0;
	PPEquipConfig  eq_cfg;
	THROW(CheckCfgRights(PPCFGOBJ_EQUIP, PPR_READ, 0));
	THROW(is_new = ReadEquipConfig(&eq_cfg));
	THROW(CheckDialogPtr(&(dlg = new EquipConfigDialog())));
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
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

PPID GetCashiersPsnKindID()
{
	PPEquipConfig  eq_cfg;
	return (ReadEquipConfig(&eq_cfg) > 0) ? eq_cfg.CshrsPsnKindID : 0L;
}
//
// @ModuleDef(PPObjTouchScreen)
//
PPTouchScreen2::PPTouchScreen2()
{
	THISZERO();
}

PPTouchScreenPacket::PPTouchScreenPacket()
{
}

PPTouchScreenPacket & FASTCALL PPTouchScreenPacket::operator = (const PPTouchScreenPacket & s)
{
	Rec = s.Rec;
	GrpIDList.copy(s.GrpIDList);
	return *this;
}

PPObjTouchScreen::PPObjTouchScreen(void * extraPtr) : PPObjReference(PPOBJ_TOUCHSCREEN, extraPtr)
{
}

int PPObjTouchScreen::GetPacket(PPID id, PPTouchScreenPacket * pPack)
{
	int    ok = 1;
	PPTouchScreenPacket  ts_pack;
	THROW(P_Ref->GetItem(Obj, id, &ts_pack.Rec) > 0);
	THROW(P_Ref->GetPropArray(Obj, id, DBDPRP_LOCLIST, &ts_pack.GrpIDList));
	CATCH
		MEMSZERO(ts_pack.Rec);
		ts_pack.GrpIDList.freeAll();
		ok = 0;
	ENDCATCH
	ASSIGN_PTR(pPack, ts_pack);
	return ok;
}

int PPObjTouchScreen::PutPacket(PPID * pID, PPTouchScreenPacket * pPack, int use_ta)
{
	int    ok = 1;
	PPIDArray * p_grp_list = (pPack && pPack->GrpIDList.getCount()) ? &pPack->GrpIDList : 0;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(pPack) {
			THROW(CheckDupName(*pID, pPack->Rec.Name));
			if(*pID) {
				THROW(P_Ref->UpdateItem(Obj, *pID, &pPack->Rec, 1, 0));
			}
			else {
				THROW(P_Ref->AddItem(Obj, pID, &pPack->Rec, 0));
			}
		}
		else if(*pID) {
			THROW(P_Ref->RemoveItem(Obj, *pID, 0));
		}
		THROW(P_Ref->PutPropArray(Obj, *pID, TSCPRP_GRPLIST, p_grp_list, 0));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}
//
// TouchScreenDlg
//
class TouchScreenDlg : public TDialog {
	DECL_DIALOG_DATA(PPTouchScreenPacket);
public:
	TouchScreenDlg() : TDialog(DLG_TCHSCR)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		setCtrlData(CTL_TCHSCR_NAME, Data.Rec.Name);
		setCtrlData(CTL_TCHSCR_ID,  &Data.Rec.ID);
		SetupStringCombo(this, CTLSEL_TCHSCR_TYPE, PPTXT_TOUCHSCREEN, Data.Rec.TouchScreenType);
		SetupPPObjCombo(this, CTLSEL_TCHSCR_GDSGRP, PPOBJ_GOODSGROUP, Data.Rec.AltGdsGrpID, OLW_CANINSERT, reinterpret_cast<void *>(GGRTYP_SEL_ALT));
		setCtrlData(CTL_TCHSCR_GOODSLISTGAP, &Data.Rec.GdsListEntryGap);
		AddClusterAssoc(CTL_TCHSCR_FLAGS, 0, TSF_PRINTSLIPDOC);
		AddClusterAssoc(CTL_TCHSCR_FLAGS, 1, TSF_TXTSTYLEBTN);
		SetClusterData(CTL_TCHSCR_FLAGS, Data.Rec.Flags);
		return 1;
	}
	DECL_DIALOG_GETDTS()
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
private:
	DECL_HANDLE_EVENT
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
	int    SelGdsFont();
	int    SelGrpList();
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
	STRNSCPY(log_font.lfFaceName, SUcSwitch(Data.Rec.GdsListFontName)); // @unicodeproblem
	log_font.lfHeight = Data.Rec.GdsListFontHight;
	font.lpLogFont   = &log_font;
	font.lStructSize = sizeof(font);
	if(::ChooseFont(&font)) { // @unicodeproblem
		STRNSCPY(Data.Rec.GdsListFontName, SUcSwitch(font.lpLogFont->lfFaceName)); // @unicodeproblem
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

int PPObjTouchScreen::Edit(PPID * pID, void * extraPtr)
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
PPLocPrinter2::PPLocPrinter2()
{
	THISZERO();
}

PPObjLocPrinter::PPObjLocPrinter(void * extraPtr) : PPObjReference(PPOBJ_LOCPRINTER, extraPtr)
{
}

int PPObjLocPrinter::GetPacket(PPID id, PPLocPrinter * pPack)
{
	PPLocPrinter  loc_prn;
	int   ok = Search(id, &loc_prn);
	ASSIGN_PTR(pPack, loc_prn);
	return ok;
}

int PPObjLocPrinter::PutPacket(PPID * pID, const PPLocPrinter * pPack, int use_ta)
{
	int    ok = 1;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID) {
			if(pPack) {
				THROW(CheckDupName(*pID, pPack->Name));
				THROW(P_Ref->UpdateItem(Obj, *pID, pPack, 1, 0));
			}
			else {
				THROW(P_Ref->RemoveItem(Obj, *pID, 0));
			}
		}
		else {
			*pID = pPack->ID;
			THROW(CheckDupName(*pID, pPack->Name));
			THROW(P_Ref->AddItem(Obj, pID, pPack, 0));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

/*virtual*/int PPObjLocPrinter::Edit(PPID * pID, void * extraPtr)
{
	class LocPrinterDialog : public TDialog {
		DECL_DIALOG_DATA(PPLocPrinter);
	public:
		LocPrinterDialog() : TDialog(DLG_LOCPRN)
		{
			PPSetupCtrlMenu(this, CTL_LOCPRN_PORT, CTLMNU_LOCPRN_PORT, CTRLMENU_SELPRINTER);
		}
		DECL_DIALOG_SETDTS()
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
		DECL_DIALOG_GETDTS()
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

PPALDD_DESTRUCTOR(LocPrnTest) { Destroy(); }

int PPALDD_LocPrnTest::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = 1;
	PPLocPrinter * p_locprn = 0;
	if(rsrv)
		Extra[1].Ptr = p_locprn = static_cast<PPLocPrinter *>(rFilt.Ptr);
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

int PPALDD_LocPrnTest::NextIteration(PPIterID iterId)
{
	IterProlog(iterId, 0);
	if(I.IterNo < TEST_ITERCOUNT) {
		SString buf;
		I.IterNo++;
		PPLoadString("test", buf);
		buf.Cat(I.IterNo);
		buf.CopyTo(I.TestIterText, sizeof(I.TestIterText));
	}
	else
		return -1;
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_LocPrnTest::Destroy()
{
	Extra[1].Ptr = 0;
}

int PPObjLocPrinter::Browse(void * extraPtr)
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
		if(CheckDialogPtrErr(&dlg))
			ExecViewAndDestroy(dlg);
		else
			ok = 0;
	}
	else
		ok = PPErrorZ();
	return ok;
}

int PPObjLocPrinter::GetPrinterByLocation(PPID locID, SString & rPrnPort, PPLocPrinter * pRec)
{
	int    ok = -1;
	PPLocPrinter lp_rec;
	rPrnPort.Z();
	for(SEnum en = P_Ref->Enum(Obj, 0); ok < 0 && en.Next(&lp_rec) > 0;) {
		if(lp_rec.LocID == locID) {
			rPrnPort = lp_rec.Port;
			ASSIGN_PTR(pRec, lp_rec);
			ok = 1;
		}
	}
	return ok;
}

int PPObjLocPrinter::GetLocPrnAssoc(LAssocArray & rList)
{
	int    ok = -1;
	rList.clear();
	PPLocPrinter lp_rec;
	for(SEnum en = P_Ref->Enum(Obj, 0); en.Next(&lp_rec) > 0;) {
		rList.AddUnique(lp_rec.LocID, lp_rec.ID, 0);
		ok = 1;
	}
	return ok;
}

int PPObjLocPrinter::IsPrinter()
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

PPALDD_DESTRUCTOR(CashNode) { Destroy(); }

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
