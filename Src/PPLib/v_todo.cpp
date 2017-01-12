// V_TODO.CPP
// Copyright (c) A.Sobolev 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2015, 2016, 2017
//
#include <pp.h>
#pragma hdrstop
//
// VCalendar
//
VCalendar::Todo::Todo()
{
	Init();
}

void VCalendar::Todo::Init()
{
	CreatedDtm.SetZero();
	CompletedDtm.SetZero();
	StartDtm.SetZero();
	EndDtm.SetZero();
	DueDtm.SetZero();
	Sequence = 0;
	Priority = 0;
	Status = stNeedsAction;
	Classification = clPublic;
	Category = 0;
	Owner = 0;
	Summary = 0;
	Location = 0;
	Contact = 0;
	Attendee = 0;
	Descr = 0;
}

SLAPI VCalendar::VCalendar(const char * pFileName /*=0*/, int forExport /*=1*/)
{
	P_Stream = 0;
	PPLoadString(PPSTR_TEXT, PPTXT_VCAL_PROPERTIES,     Properties);
	PPLoadString(PPSTR_TEXT, PPTXT_VCAL_STATUSLIST,     Status);
	PPLoadString(PPSTR_TEXT, PPTXT_VCAL_CLASSIFICATION, Classification);
	PPLoadString(PPSTR_TEXT, PPTXT_VCAL_PROPATTRIBUTE,  PropAttrib);
	Open(pFileName, forExport);
}

SLAPI VCalendar::~VCalendar()
{
	Close();
}

int SLAPI VCalendar::Open(const char * pFileName, int forExport)
{
	int ok = -1;
	Close();
	if(pFileName && strlen(pFileName)) {
		Export = forExport;
		THROW_MEM(P_Stream = new SFile(pFileName, (Export) ? SFile::mWrite : SFile::mRead));
		if(P_Stream->IsValid()) {
			if(forExport) {
				SString temp_buf;
				temp_buf.GetSubFrom(Properties, ';', prpBeginCal);
				temp_buf.CR().Cat("VERSION:2.0").CR();
				P_Stream->WriteLine(temp_buf);
			}
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI VCalendar::Close()
{
  	if(P_Stream) {
		SString temp_buf;
		temp_buf.GetSubFrom(Properties, ';', prpEndCal);
		P_Stream->WriteLine(temp_buf.CR());
		ZDELETE(P_Stream);
	}
	return 1;
}

int SLAPI VCalendar::PutEvent(VCalendar::Event * pData)
{
	return -1;
}

int SLAPI VCalendar::PutTodo(const VCalendar::Todo * pData)
{
	int    ok = -1;
	if(pData && P_Stream && P_Stream->IsValid() && Export) {
		SString temp_buf;
		temp_buf.GetSubFrom(Properties, ';', prpBeginTodo);
		P_Stream->WriteLine(temp_buf.CR());
		THROW(PutTodoProperty(prpCreatedDtm,     &pData->CreatedDtm,     0));
		THROW(PutTodoProperty(prpCompletedDtm,   &pData->CompletedDtm,   0));
		THROW(PutTodoProperty(prpStartDtm,       &pData->StartDtm,       0));
		THROW(PutTodoProperty(prpEndDtm,         &pData->EndDtm,         0));
		THROW(PutTodoProperty(prpEndDtm,         &pData->DueDtm,         0));
		THROW(PutTodoProperty(prpSequence,       &pData->Sequence,       0));
		THROW(PutTodoProperty(prpStatus,         &pData->Status,         0));
		THROW(PutTodoProperty(prpCategory,       &pData->Category,       0));
		THROW(PutTodoProperty(prpClassification, &pData->Classification, 0));
		THROW(PutTodoProperty(prpPriority,       &pData->Priority,       0));
		THROW(PutTodoProperty(prpOwner,          &pData->Owner,          0));
		THROW(PutTodoProperty(prpLocation,       &pData->Location,       0));
		THROW(PutTodoProperty(prpSummary,        &pData->Summary,        0));
		THROW(PutTodoProperty(prpDescr,          &pData->Descr,          0));
		THROW(PutTodoProperty(prpAttendee,       &pData->Attendee,       0));
		THROW(PutTodoProperty(prpContact,        &pData->Contact,        0));
		temp_buf.GetSubFrom(Properties, ';', prpEndTodo);
		P_Stream->WriteLine(temp_buf.CR());
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SLAPI VCalendar::PutTodoProperty(TodoProperty prop, const void * pVal, long addedParam)
{
	int    ok = -1;
	if(P_Stream && P_Stream->IsValid() && prop && pVal && Export) {
		SString temp_buf;
		switch(prop) {
			case prpCreatedDtm:
			case prpCompletedDtm:
			case prpStartDtm:
			case prpEndDtm:
			case prpDueDtm:
				{
					LDATETIME dtm;
					dtm = *(LDATETIME*)pVal;
					if(checkdate(dtm.d, 0)) {
						if(dtm.t == ZEROTIME)
							dtm.t = encodetime(23, 59, 59, 0);
						temp_buf.Cat(dtm.d, DATF_ISO8601|DATF_NODIV|DATF_CENTURY);
						temp_buf.CatChar('T').Cat(dtm.t, TIMF_HMS|TIMF_NODIV);
						//temp_buf.Cat(dtm.d, DATF_CENTURY|DATF_YMD).ReplaceStr(".", "", 0);
						//temp_buf.ReplaceStr("\\", "", 0);
						//temp_buf.ReplaceStr("/", "", 0);
						//temp_buf.CatChar('T').Cat(dtm.t, TIMF_HMS).ReplaceStr(":", "", 0);
					}
				}
				break;
			case prpSequence:
				temp_buf.Cat(*(int16*)pVal);
				break;
			case prpStatus:
				temp_buf.GetSubFrom(Status, ';', *(int16*)pVal);
				break;
			case prpClassification:
				temp_buf.GetSubFrom(Classification, ';', *(int16*)pVal);
				break;
			case prpPriority:
				temp_buf.Cat(*(int16*)pVal);
				break;
			case prpCategory:
			case prpOwner:
			case prpSummary:
			case prpLocation:
			case prpDescr:
			case prpContact:
			case prpAttendee:
				temp_buf = *(const SString *)pVal;
				break;
		}
		if(temp_buf.Len()) {
			SString buf;
			temp_buf.ReplaceStr("\xD\xA", " ", 0);
			temp_buf.ReplaceStr("\\", "\\\\", 0);
			temp_buf.ReplaceStr(",", "\\,", 0);
			temp_buf.ReplaceStr(";", "\\;", 0);
			buf.GetSubFrom(Properties, ';', (int)prop);
			if(prop == prpOwner) {
				SString role_owner;
				role_owner.GetSubFrom(PropAttrib, ';', prpatrRoleOwner);
				buf.Semicol().Cat(role_owner);
			}
			buf.CatChar(':').Cat(temp_buf).Transf(CTRANSF_INNER_TO_UTF8).CR();
			P_Stream->WriteLine(buf);
		}
	}
	return ok;
}

int SLAPI VCalendar::ReadProp(TodoProperty * pProp, SString & rVal, SString & rAttrib)
{
	int    ok = -1;
	SString temp_buf, val, attrs;
	TodoProperty prop;
	if(P_Stream && P_Stream->IsValid() && Export == 0 && P_Stream->ReadLine(temp_buf) > 0) {
		SString attrs, str_mime64_enc, temp_buf2, str_prop;
		ok = 1;
		temp_buf.TrimRightChr('\n');
		prop = (TodoProperty)Properties.GetIdxBySub(temp_buf, ';');
		if(prop == -1) {
			int mime64_enc = 0;
			uint pos = 0;
			if(temp_buf.StrChr(':', &pos) > 0 && pos > 0 && temp_buf.C(pos - 1) != '\\') {
				temp_buf.Divide(':', temp_buf2, val);
				PrevTempBuf = temp_buf;
			}
			else {
				PrevTempBuf.Divide(':', temp_buf2, val);
				val = temp_buf;
			}
			temp_buf2.Divide(';', str_prop, attrs);
			prop = (TodoProperty)Properties.GetIdxBySub(str_prop, ';');
			str_mime64_enc.GetSubFrom(PropAttrib, ';', prpatrEncMime64);
			if((mime64_enc = attrs.GetIdxBySub(str_mime64_enc, ';')) < 0) {
				str_mime64_enc.GetSubFrom(PropAttrib, ';', prpatrShortEncMime64);
				mime64_enc = attrs.GetIdxBySub(str_mime64_enc, ';');
			}
			if(mime64_enc >= 0) {
				(temp_buf2 = 0).DecodeMime64((void*)(const char*)val, val.Len(), 0);
				val = temp_buf2;
			}
			else
				val.Transf(CTRANSF_UTF8_TO_OUTER);
			val.Transf(CTRANSF_OUTER_TO_INNER);
			val.ReplaceStr("\n", "\xD\xA", 0);
			val.ReplaceStr("\\,", ",", 0);
			val.ReplaceStr("\\;", ";", 0);
			val.ReplaceStr("\\""", """", 0);
			temp_buf.ReplaceStr("\\\\", "\\", 0);
		}
	}
	ASSIGN_PTR(pProp, prop);
	rVal    = val;
	rAttrib = attrs;
	return ok;
}

int SLAPI VCalendar::GetTodo(VCalendar::Todo * pData)
{
	int ok = -1;
	if(pData && P_Stream && P_Stream->IsValid() && Export == 0) {
		int read_todo = 0, end_todo = 0;
		SString val, attrs;
		TodoProperty prop;
		VCalendar::Todo todo_rec;

		pData->Init();
		PrevTempBuf = 0;
		while(!end_todo && ReadProp(&prop, val, attrs) > 0) {
			switch(prop) {
				case prpBeginTodo:
					read_todo = 1;
					break;
				case prpEndTodo:
					end_todo = 1;
					if(read_todo == 1)
						ok = 1;
					break;
				case prpCreatedDtm:
					GetDtm(val, &todo_rec.CreatedDtm);
					break;
				case prpCompletedDtm:
					GetDtm(val, &todo_rec.CompletedDtm);
					break;
				case prpStartDtm:
					GetDtm(val, &todo_rec.StartDtm);
					break;
				case prpEndDtm:
					GetDtm(val, &todo_rec.EndDtm);
					break;
				case prpDueDtm:
					GetDtm(val, &todo_rec.DueDtm);
					break;
				case prpSequence:
					todo_rec.Sequence = (int16)val.ToLong();
					break;
				case prpStatus:
					{
						int status = Status.GetIdxBySub(val, ';');
						todo_rec.Status = (status >= 0) ? (TodoStatus)status : stNeedsAction;
					}
					break;
				case prpClassification:
					{
						int classification = Classification.GetIdxBySub(val, ';');
						todo_rec.Classification = (classification >= 0) ? (TodoClass)classification : clPublic;
					}
					break;
				case prpPriority:
					todo_rec.Priority = (int16)val.ToLong();
					break;
				case prpCategory:
					todo_rec.Category.Cat(val);
					break;
				case prpOwner:
					{
						SString role_owner;
						role_owner.GetSubFrom(PropAttrib, ';', prpatrRoleOwner);
						if(attrs.GetIdxBySub(role_owner, ';') >= 0)
							todo_rec.Owner.Cat(val);
					}
					break;
				case prpSummary:
					todo_rec.Summary.Cat(val);
					break;
				case prpLocation:
					pData->Location.Cat(val);
					break;
				case prpDescr:
					todo_rec.Descr.Cat(val);
					break;
			}
		}
		if(ok > 0)
			ASSIGN_PTR(pData, todo_rec);
	}
	return ok;
}

int SLAPI VCalendar::GetDtm(SString & rBuf, LDATETIME * pDtm)
{
	int    ok = -1;
	LDATETIME dtm;
	SString str_dt, str_tm;
	rBuf.Divide('T', str_dt, str_tm);
	if(str_dt.Len() == 8) {
		SString buf;
		encodedate(str_dt.Sub(6, 2, buf).ToLong(), str_dt.Sub(4, 2, buf).ToLong(), str_dt.Sub(0, 4, buf).ToLong(), &dtm.d);
		ok = 1;
		if(str_tm.Len() == 6)
			dtm.t = encodetime(str_tm.Sub(0, 2, buf).ToLong(), str_tm.Sub(2, 2, buf).ToLong(), str_tm.Sub(4, 2, buf).ToLong(), 0);
	}
	ASSIGN_PTR(pDtm, dtm);
	return ok;
}
//
// @ModuleDef(PPViewPrjTask)
//
IMPLEMENT_PPFILT_FACTORY(PrjTask); SLAPI PrjTaskFilt::PrjTaskFilt() : PPBaseFilt(PPFILT_PRJTASK, 0, 0)
{
	SetFlatChunk(offsetof(PrjTaskFilt, ReserveStart),
		offsetof(PrjTaskFilt, Reserve)-offsetof(PrjTaskFilt, ReserveStart)+sizeof(Reserve));
	Init(1, 0);
}

int SLAPI PrjTaskFilt::Init(int fullyDestroy, long extraData)
{
	PPBaseFilt::Init(fullyDestroy, extraData);
	ExcludeStatus(TODOSTTS_REJECTED);
	ExcludeStatus(TODOSTTS_COMPLETED);
	return 1;
}

int SLAPI PrjTaskFilt::InclInList(int16 * pList, size_t listSize, int16 val)
{
	size_t first_zero_idx = MAXLONG; // @v8.9.8 MAXINT-->MAXLONG
	for(size_t i = 0; i < listSize; i++) {
		if(pList[i] == 0 && first_zero_idx == MAXLONG) // @v8.9.8 MAXINT-->MAXLONG
			first_zero_idx = i;
		if(pList[i] == val)
			return -1;
	}
	if(first_zero_idx != MAXLONG) { // @v8.9.8 MAXINT-->MAXLONG
		pList[first_zero_idx] = val;
		return 1;
	}
	else
		return 0;
}

int SLAPI PrjTaskFilt::ExclFromList(int16 * pList, size_t listSize, int16 minVal, int16 maxVal, int16 val)
{
	int    is_empty = 1;
	size_t i;
	for(i = 0; is_empty && i < listSize; i++)
		if(pList[i] != 0)
			is_empty = 0;
	if(is_empty) {
		int16 v = minVal;
		for(i = 0; v <= maxVal && i < listSize; i++)
			pList[i] = v++;
	}
	for(i = 0; i < listSize; i++)
		if(pList[i] == val) {
			pList[i] = 0;
			return 1;
		}
	return -1;
}

int SLAPI PrjTaskFilt::GetList(const int16 * pList, size_t listSize, PPIDArray * pDestList) const
{
	pDestList->freeAll();
	int    is_empty = 1;
	size_t i;
	for(i = 0; is_empty && i < listSize; i++)
		if(pList[i] != 0)
			is_empty = 0;
	if(is_empty)
		return -1;
	else {
		for(i = 0; i < listSize; i++)
			if(pList[i])
				pDestList->addUnique(pList[i]);
		return 1;
	}
}

int SLAPI PrjTaskFilt::IncludeStatus(long status)
{
	if(!PrjTaskCore::IsValidStatus(status))
		return 0;
	return InclInList(StatusList, sizeof(StatusList) / sizeof(StatusList[0]), (int16)status);
}

int SLAPI PrjTaskFilt::ExcludeStatus(long status)
{
	if(!PrjTaskCore::IsValidStatus(status))
		return 0;
	return ExclFromList(StatusList, sizeof(StatusList) / sizeof(StatusList[0]), 1, 5, (int16)status);
}

int SLAPI PrjTaskFilt::IncludePrior(long prior)
{
	if(!PrjTaskCore::IsValidPrior(prior))
		return 0;
	return InclInList(PriorList, sizeof(PriorList) / sizeof(PriorList[0]), (int16)prior);
}

int SLAPI PrjTaskFilt::ExcludePrior(long prior)
{
	if(!PrjTaskCore::IsValidPrior(prior))
		return 0;
	return ExclFromList(PriorList, sizeof(PriorList) / sizeof(PriorList[0]), 1, 5, (int16)prior);
}

int SLAPI PrjTaskFilt::GetStatusList(PPIDArray * pList) const
{
	return GetList(StatusList, sizeof(StatusList) / sizeof(StatusList[0]), pList);
}

SString & SLAPI PrjTaskFilt::GetStatusListText(SString & rDest) const
{
	PPIDArray id_list;
	GetStatusList(&id_list);
	rDest = 0;
	SString temp_buf;
	for(uint i = 0; i < id_list.getCount(); i++)
		rDest.CatDiv(';', 2, 1).Cat(PPObjPrjTask::GetStatusText(id_list.get(i), temp_buf));
	return rDest;
}

SString & SLAPI PrjTaskFilt::GetPriorListText(SString & rDest) const
{
	PPIDArray id_list;
	GetPriorList(&id_list);
	rDest = 0;
	SString temp_buf;
	for(uint i = 0; i < id_list.getCount(); i++)
		rDest.CatDiv(';', 2, 1).Cat(PPObjPrjTask::GetPriorText(id_list.get(i), temp_buf));
	return rDest;
}

int SLAPI PrjTaskFilt::GetPriorList(PPIDArray * pList) const
{
	return GetList(PriorList, sizeof(PriorList) / sizeof(PriorList[0]), pList);
}
//
//
//
SLAPI PPViewPrjTask::PPViewPrjTask() : PPView(&TodoObj, &Filt, PPVIEW_PRJTASK)
{
	ImplementFlags |= implChangeFilt;
	P_TempOrd = 0;
	P_TempTbl = 0;
	UpdateTaskList.freeAll();
	Grid.P_View = this;
}

SLAPI PPViewPrjTask::~PPViewPrjTask()
{
	UpdateTimeBrowser(1);
	delete P_TempOrd;
	delete P_TempTbl;
}

int SLAPI PPViewPrjTask::UpdateTempTable(const PPIDArray * pIdList, int use_ta)
{
	int    ok = -1;
	if(P_TempOrd && pIdList) {
		PPTransaction tra(ppDbDependTransaction, use_ta);
		THROW(tra);
		for(uint i = 0; i < pIdList->getCount(); i++) {
			PPID   id = pIdList->get(i);
			PrjTaskTbl::Rec rec;
			TempOrderTbl::Rec ord_rec;
			if(TodoObj.Search(id, &rec) > 0 && CheckRecForFilt(&rec)) {
				ok = 1;
				MakeTempEntry(&rec, &ord_rec);
				if(SearchByID(P_TempOrd, 0, id, &ord_rec) > 0) {
					UpdateByID(P_TempOrd, 0, id, &ord_rec, 0);
				}
				else
					AddByID(P_TempOrd, &id, &ord_rec, 0);
				AddItemToTimeGrid(&rec, 0);
			}
			else if(SearchByID(P_TempOrd, 0, id, &ord_rec) > 0) {
				deleteFrom(P_TempOrd, 0, P_TempOrd->ID == id);
				AddItemToTimeGrid(&rec, 1);
				ok = 1;
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPViewPrjTask::CheckRecForFilt(const PrjTaskTbl::Rec * pRec)
{
	if(pRec == 0)
		return 0;
	if(Filt.Kind && pRec->Kind != Filt.Kind)
		return 0;
	if(Filt.ProjectID && pRec->ProjectID != Filt.ProjectID)
		return 0;
	if(ClientList.IsExists() && ClientList.CheckID(pRec->ClientID) == 0)
		return 0;
	if(CreatorList.IsExists() && CreatorList.CheckID(pRec->CreatorID) == 0)
		return 0;
	if(EmployerList.IsExists() && EmployerList.CheckID(pRec->EmployerID) == 0)
		return 0;
	if(!Filt.Period.CheckDate(pRec->Dt))
		return 0;
	if(!Filt.StartPeriod.CheckDate(pRec->StartDt))
		return 0;
	if(!Filt.EstFinishPeriod.CheckDate(pRec->EstFinishDt))
		return 0;
	if(!Filt.FinishPeriod.CheckDate(pRec->FinishDt))
		return 0;
	if(Filt.Flags & PrjTaskFilt::fUnbindedOnly && pRec->ProjectID)
		return 0;

	PPIDArray f_list;
	if(Filt.GetStatusList(&f_list) > 0 && !f_list.lsearch(pRec->Status))
		return 0;
	f_list.freeAll();
	if(Filt.GetPriorList(&f_list) > 0 && !f_list.lsearch(pRec->Priority))
		return 0;
	if(Filt.CliCityID)
		if(pRec->ClientID) {
			PPID addr_id = 0, city_id = 0;
			PPObjPerson psn_obj;
			if(psn_obj.GetAddrID(pRec->ClientID, pRec->DlvrAddrID, PSNGETADDRO_DLVRADDR, &addr_id) > 0) {
				psn_obj.GetCityByAddr(addr_id, &city_id, 0);
				if(city_id != Filt.CliCityID)
					return 0;
			}
			else
				return 0;
		}
		else
			return 0;
	return 1;
}

int SLAPI PPViewPrjTask::MakeTempEntry(const PrjTaskTbl::Rec * pRec, TempOrderTbl::Rec * pTempRec)
{
	SString ord_buf;
	if(Filt.Order == PrjTaskFilt::ordByDt)
		ord_buf.Cat(pRec->Dt, DATF_YMD|DATF_CENTURY);
	else if(Filt.Order == PrjTaskFilt::ordByStartDt)
		ord_buf.Cat(pRec->StartDt, DATF_YMD|DATF_CENTURY);
	else if(Filt.Order == PrjTaskFilt::ordByEstFinishDt)
		ord_buf.Cat(pRec->EstFinishDt, DATF_YMD|DATF_CENTURY);
	else if(Filt.Order == PrjTaskFilt::ordByFinishDt)
		ord_buf.Cat(pRec->FinishDt, DATF_YMD|DATF_CENTURY);
	else if(Filt.Order == PrjTaskFilt::ordByCreator)
		GetPersonName(pRec->CreatorID, ord_buf);
	else if(Filt.Order == PrjTaskFilt::ordByEmployer)
		GetPersonName(pRec->EmployerID, ord_buf);
	else if(Filt.Order == PrjTaskFilt::ordByClient)
		GetPersonName(pRec->ClientID, ord_buf);
	else if(Filt.Order == PrjTaskFilt::ordByCode)
		ord_buf.Cat(pRec->Code);
	else // @default
		ord_buf.Cat(pRec->Dt, DATF_YMD|DATF_CENTURY);
	memzero(pTempRec, sizeof(*pTempRec));
	pTempRec->ID = pRec->ID;
	ord_buf.CopyTo(pTempRec->Name, sizeof(pTempRec->Name));
	return 1;
}

PP_CREATE_TEMP_FILE_PROC(CreateTempFile, TempPrjTask);

class CrosstabProcessor {
public:
	SLAPI  CrosstabProcessor(TempPrjTaskTbl * pTbl, PrjTaskFilt * pFilt);
	int    SLAPI ProcessRec(PrjTaskTbl::Rec * pRec);
	int    SLAPI Start();
	int    SLAPI Finish();
private:
	int    SLAPI SearchRec(PPID tabID, void * pAddInfo, TempPrjTaskTbl::Rec * pRec);
	int    SLAPI AddRec(PPID tabID, double tabParam, double addParam, PrjTaskTbl::Rec * pRec);

	PPProjectConfig PrjCfg;
	PrjTaskFilt Filt;
	TempPrjTaskTbl * P_TempTbl;
};

SLAPI CrosstabProcessor::CrosstabProcessor(TempPrjTaskTbl * pTbl, PrjTaskFilt * pFilt)
{
	P_TempTbl = pTbl;
	if(!RVALUEPTR(Filt, pFilt))
		Filt.Init(1, 0);
}

int SLAPI CrosstabProcessor::Start()
{
	int    ok = 1;
	MEMSZERO(PrjCfg);
	if(Filt.TabType == PrjTaskFilt::crstDateHour || Filt.TabType == PrjTaskFilt::crstEmployerHour) {
		PPObjProject::ReadConfig(&PrjCfg);
		if(Filt.TabType == PrjTaskFilt::crstDateHour) {
			LDATE  dt = Filt.StartPeriod.low;
			DateIter dt_iter;
			for(dt_iter.Init(&Filt.StartPeriod); !dt_iter.IsEnd(); plusdate(&dt, 1, 0), dt_iter.Advance(dt, 0)) {
				PrjTaskTbl::Rec rec;
				MEMSZERO(rec);
				rec.StartDt = dt;
				for(int h = PrjCfg.WorkHoursBeg; h <= PrjCfg.WorkHoursEnd; h++)
					THROW(AddRec(h, 0, 0, &rec));
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI CrosstabProcessor::Finish()
{
	int    ok = 1;
	TempPrjTaskTbl * p_tbl = 0;
	if(Filt.TabParam == PrjTaskFilt::ctpComplTaskRatio) {
		long   tab_type = Filt.TabType;
		IterCounter counter;
		TempPrjTaskTbl::Key0 k, k_;
		THROW(CheckTblPtr(p_tbl = new TempPrjTaskTbl(P_TempTbl->fileName)));
		BExtQuery q(p_tbl, 0);
		q.select(p_tbl->StartDt, p_tbl->ClientID, p_tbl->EmployerID, p_tbl->TabParam, 0L);
		MEMSZERO(k);
		k_ = k;
		counter.Init(q.countIterations(0, &k_, spGt));
		for(q.initIteration(0, &k, spGt); q.nextIteration() > 0;) {
			union {
				LDATE  dt;
				PPID   id;
			} add_i;
			double task_count = 1.0;
			if(tab_type == PrjTaskFilt::crstDateHour)
				add_i.dt = p_tbl->data.StartDt;
			else if(tab_type == PrjTaskFilt::crstClientDate || tab_type == PrjTaskFilt::crstClientEmployer)
				add_i.id = p_tbl->data.ClientID;
			else if(tab_type == PrjTaskFilt::crstEmployerDate || tab_type == PrjTaskFilt::crstEmployerHour)
				add_i.id = p_tbl->data.EmployerID;
			counter.Increment();
			SearchRec(p_tbl->data.TabID, (void*)&add_i, 0);
			task_count = P_TempTbl->data.TaskCount;
			P_TempTbl->data.TabParam /= NZOR(task_count, 1);
			P_TempTbl->updateRec();
			PPWaitPercent(counter);
		}
	}
	CATCHZOK
	ZDELETE(p_tbl);
	return ok;
}

int SLAPI CrosstabProcessor::ProcessRec(PrjTaskTbl::Rec * pRec)
{
	int    ok = 1;
	if(pRec) {
		long   tab_param = Filt.TabParam;
		PPID   tab_id = 0;
		double tab_param_val = 0.0;
		int    start_hr = PrjCfg.WorkHoursBeg;
		int    end_hr   = PrjCfg.WorkHoursEnd ? PrjCfg.WorkHoursEnd : 23;
		if(Filt.Sgd != sgdNone && oneof2(Filt.TabType, PrjTaskFilt::crstClientDate, PrjTaskFilt::crstEmployerDate))
			ShrinkSubstDate(Filt.Sgd, pRec->StartDt, &pRec->StartDt);
		switch(Filt.TabType) {
			case PrjTaskFilt::crstDateHour:
			case PrjTaskFilt::crstEmployerHour:
				for(int h = start_hr; h <= end_hr; h++) {
					tab_id = h;
					if(oneof2(tab_param, PrjTaskFilt::ctpUnComplTask, PrjTaskFilt::ctpComplTaskRatio)) {
						/*
						if(pRec->FinishDt != ZERODATE && pRec->FinishDt < pRec->StartDt)
							tab_param_val = 0;
						else if(pRec->FinishDt == ZERODATE)
							tab_param_val = 1;
						else
						*/
							tab_param_val = (/*pRec->FinishDt != pRec->StartDt || !pRec->FinishTm.hour() ||
								pRec->StartTm.hour() > h*/pRec->StartTm.hour() == h) ? 1 : 0;
						if(tab_param == PrjTaskFilt::ctpComplTaskRatio)
							tab_param_val = tab_param_val == 1 ? 0 : 1;
					}
					else if(tab_param == PrjTaskFilt::ctpWrofBillPrct) {
					}
					else if(tab_param == PrjTaskFilt::ctpTaskCount)
						tab_param_val = pRec->StartTm.hour() <= h ? 1 : 0;
					THROW(AddRec(tab_id, tab_param_val, 1, pRec));
				}
				break;
			case PrjTaskFilt::crstClientDate:
			case PrjTaskFilt::crstEmployerDate:
				tab_id = pRec->StartDt.v;
				if(tab_param == PrjTaskFilt::ctpUnComplTask || tab_param == PrjTaskFilt::ctpComplTaskRatio) {
					tab_param_val = ((pRec->FinishDt == ZERODATE || pRec->FinishDt > pRec->StartDt)) ? 1 : 0;
					if(tab_param == PrjTaskFilt::ctpComplTaskRatio)
						tab_param_val = (tab_param_val == 1) ? 0 : 1;
				}
				else if(tab_param == PrjTaskFilt::ctpWrofBillPrct) {
				}
				else if(tab_param == PrjTaskFilt::ctpTaskCount)
					tab_param_val = 1;
				THROW(AddRec(tab_id, tab_param_val, 1, pRec));
				break;
			case PrjTaskFilt::crstClientEmployer:
				tab_id = pRec->EmployerID;
				if(oneof2(tab_param, PrjTaskFilt::ctpUnComplTask, PrjTaskFilt::ctpComplTaskRatio)) {
					tab_param_val = BIN(Filt.StartPeriod.CheckDate(pRec->StartDt) &&
						(!pRec->FinishDt || !Filt.StartPeriod.CheckDate(pRec->FinishDt)));
					if(tab_param == PrjTaskFilt::ctpComplTaskRatio)
						tab_param_val = (tab_param_val == 1) ? 0 : 1;
				}
				else if(tab_param == PrjTaskFilt::ctpWrofBillPrct) {
				}
				else if(tab_param == PrjTaskFilt::ctpTaskCount)
					tab_param_val = 1;
				THROW(AddRec(tab_id, tab_param_val, 1, pRec));
				break;
			default:
				ok = (PPErrCode = PPERR_INVPARAM, 0);
				break;
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int SLAPI CrosstabProcessor::AddRec(PPID tabID, double tabParam, double addParam, PrjTaskTbl::Rec * pRec)
{
	int    ok = -1;
	if(pRec) {
		long   tab_type = Filt.TabType;
		SString buf;
		TempPrjTaskTbl::Rec temp_rec;
		MEMSZERO(temp_rec);
		temp_rec.TabID       = tabID;
		temp_rec.EmployerID  = pRec->EmployerID;
		temp_rec.ClientID    = pRec->ClientID;
		temp_rec.StartDt     = pRec->StartDt;
		if(tab_type != PrjTaskFilt::crstDateHour) {
            GetPersonName(pRec->EmployerID, buf);
			buf.CopyTo(temp_rec.EmployerName, sizeof(temp_rec.EmployerName));
			GetPersonName(pRec->ClientID, buf);
			buf.CopyTo(temp_rec.ClientName, sizeof(temp_rec.ClientName));
		}
		else
			buf.Cat(pRec->StartDt).CopyTo(temp_rec.ClientName, sizeof(temp_rec.ClientName));
		{
			int found = 0;
			union {
				LDATE dt;
				PPID id;
			} add_i;
			if(tab_type == PrjTaskFilt::crstDateHour)
				add_i.dt = temp_rec.StartDt;
			else if(tab_type == PrjTaskFilt::crstClientDate || tab_type == PrjTaskFilt::crstClientEmployer)
				add_i.id = temp_rec.ClientID;
			else if(tab_type == PrjTaskFilt::crstEmployerDate || tab_type == PrjTaskFilt::crstEmployerHour)
				add_i.id = temp_rec.EmployerID;
			if((found = SearchRec(tabID, (void*)&add_i, 0)) > 0) {
				switch(Filt.TabParam) {
					case PrjTaskFilt::ctpComplTaskRatio:
					case PrjTaskFilt::ctpUnComplTask:
					case PrjTaskFilt::ctpTaskCount:    P_TempTbl->data.TabParam += tabParam; break;
					case PrjTaskFilt::ctpWrofBillPrct: break;
				}
				P_TempTbl->data.TaskCount += addParam;
				THROW_DB(P_TempTbl->updateRec());
			}
			else {
				temp_rec.TabParam = tabParam;
				temp_rec.TaskCount = addParam;
				THROW_DB(P_TempTbl->insertRecBuf(&temp_rec));
			}
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SLAPI CrosstabProcessor::SearchRec(PPID tabID, void * pAddInfo, TempPrjTaskTbl::Rec * pRec)
{
	int    ok = -1;
	int    idx = 0;
	long   tab_type = Filt.TabType;
	union {
		TempPrjTaskTbl::Key0 k0;
		TempPrjTaskTbl::Key1 k1;
		TempPrjTaskTbl::Key2 k2;
	} k;
	if(tab_type == PrjTaskFilt::crstDateHour) {
		k.k0.TabID     = tabID;
		k.k0.StartDt   = *(LDATE*)pAddInfo;
		idx = 0;
	}
	else if(oneof2(tab_type, PrjTaskFilt::crstClientDate, PrjTaskFilt::crstClientEmployer)) {
		k.k1.TabID     = tabID;
		k.k1.ClientID  = *(PPID *)pAddInfo;
		idx = 1;
	}
	else if(oneof2(tab_type, PrjTaskFilt::crstEmployerDate, PrjTaskFilt::crstEmployerHour)) {
		k.k2.TabID      = tabID;
		k.k2.EmployerID = *(PPID *)pAddInfo;
		idx = 2;
	}
	return SearchByKey(P_TempTbl, idx, &k, pRec);
}

int SLAPI PPViewPrjTask::GetItem(PPID id, PrjTaskViewItem * pItem)
{
	int    ok = -1;
	PrjTaskTbl::Rec rec;
	if(id && TodoObj.Search(id, &rec) > 0) {
		ASSIGN_PTR(pItem, rec);
		ok = 1;
	}
	return ok;
}

int SLAPI PPViewPrjTask::AddItemToTimeGrid(const PrjTaskViewItem * pItem, int rmv)
{
	long   row_id = (Filt.Order == PrjTaskFilt::ordByClient) ? pItem->ClientID : pItem->EmployerID;
	if(rmv)
		Grid.RemoveChunk(row_id, pItem->ID);
	else {
		STimeChunk chunk;
		chunk.Start.Set(NZOR(pItem->StartDt, pItem->Dt), NZOR(pItem->StartTm, pItem->Tm));
		chunk.Finish.Set(NZOR(pItem->FinishDt, pItem->EstFinishDt), NZOR(pItem->FinishTm, pItem->EstFinishTm));
		if(!chunk.Finish.d)
			chunk.Finish.SetFar();
		// @v7.6.10 {
		else if(cmp(chunk.Finish, chunk.Start) < 0) {
			(chunk.Finish = chunk.Start).addsec(60 * 60);
		}
		// } @v7.6.10
		if(Grid.SetChunk(row_id, pItem->ID, MakeLong(pItem->Priority, pItem->Status), &chunk) == 2) {
			SString name_buf;
			if(row_id <= 0)
				name_buf = "ALL";
			else
				GetPersonName(row_id, name_buf);
			Grid.SetRowText(row_id, name_buf, 1);
		}
	}
	return 1;
}

int SLAPI PPViewPrjTask::GetTabTitle(PPID tabID, SString & rBuf)
{
	int    ok = -1;
	if(P_TempTbl) {
		TempPrjTaskTbl::Key0 k0;
		MEMSZERO(k0);
		k0.TabID = tabID;
		if(P_TempTbl->search(0, &k0, spGe) > 0 && k0.TabID == tabID) {
			rBuf = 0;
			if(oneof2(Filt.TabType, PrjTaskFilt::crstDateHour, PrjTaskFilt::crstEmployerHour))
				rBuf.Printf("%02ld:00", P_TempTbl->data.TabID);
			else if(oneof2(Filt.TabType, PrjTaskFilt::crstClientDate, PrjTaskFilt::crstEmployerDate)) {
				LDATE dt = ZERODATE;
				dt.v = P_TempTbl->data.TabID;
				if(Filt.Sgd == sgdNone)
					rBuf.Cat(dt);
				else
					FormatSubstDate(Filt.Sgd, dt, rBuf);
			}
			else if(Filt.TabType == PrjTaskFilt::crstClientEmployer)
				rBuf.CopyFrom(P_TempTbl->data.EmployerName);
			ok = 1;
		}
		else
			rBuf = 0;
	}
	return ok;
}

int SLAPI PPViewPrjTask::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1, use_ta = 1;
	TempOrderTbl * p_ord = 0;
	BExtInsert * p_bei = 0;
	CrosstabProcessor * p_ct_prcssr = 0;
	Grid.freeAll();
	THROW(Helper_InitBaseFilt(pFilt));
	TodoObj.LinkTaskID = Filt.LinkTaskID;
	if(!(Filt.Flags & PrjTaskFilt::fNotShowPPWaitOnInit))
		PPWait(1);
	ZDELETE(P_IterQuery);
	UndefPriorList  = (Filt.GetPriorList(&PriorList) > 0) ? 0 : 1;
	UndefStatusList = (Filt.GetStatusList(&StatusList) > 0) ? 0 : 1;

	CreatorList.Set(0);
	EmployerList.Set(0);
	ClientList.Set(0);
	if(Filt.ClientID || Filt.CreatorID || Filt.EmployerID) {
		PPObjPersonRelType prt_obj;
		PPIDArray grp_prt_list;
		if(prt_obj.GetGroupingList(&grp_prt_list) > 0) {
			PPIDArray empl_list, creator_list, client_list;
			for(uint i = 0; i < grp_prt_list.getCount(); i++) {
				if(Filt.CreatorID)
					PsnObj.GetRelPersonList(Filt.CreatorID,  grp_prt_list.get(i), 1, &creator_list);
				if(Filt.EmployerID)
					PsnObj.GetRelPersonList(Filt.EmployerID, grp_prt_list.get(i), 1, &empl_list);
				if(Filt.ClientID)
					PsnObj.GetRelPersonList(Filt.ClientID,   grp_prt_list.get(i), 1, &client_list);
			}
			creator_list.addnz(Filt.CreatorID);
			empl_list.addnz(Filt.EmployerID);
			client_list.addnz(Filt.ClientID);
			creator_list.sort();
			if(creator_list.getCount())
				CreatorList.Set(&creator_list);
			empl_list.sort();
			if(empl_list.getCount())
				EmployerList.Set(&empl_list);
			client_list.sort();
			if(client_list.getCount())
				ClientList.Set(&client_list);
		}
		if(!CreatorList.IsExists())
			CreatorList.Add(Filt.CreatorID, 1);
		if(!EmployerList.IsExists())
			EmployerList.Add(Filt.EmployerID, 1);
		if(!ClientList.IsExists())
			ClientList.Add(Filt.ClientID, 1);
	}
	{
		PrjTaskFilt filt = Filt;
		PrjTaskTbl::Rec rec;
		ZDELETE(P_TempOrd);
		ZDELETE(P_TempTbl);
		Filt.TabType = PrjTaskFilt::crstNone; // @todo Кросстабуляцию доделать!
		if(Filt.TabType != PrjTaskFilt::crstNone) {
			THROW(P_TempTbl = CreateTempFile());
			THROW_MEM(p_ct_prcssr = new CrosstabProcessor(P_TempTbl, &filt));
		}
		else if(!(Filt.Flags & PrjTaskFilt::fNoTempTable)) {
			THROW(p_ord = CreateTempOrderFile());
			THROW_MEM(p_bei = new BExtInsert(p_ord));
		}
		if(p_ct_prcssr || p_ord) {
			PPTransaction tra(ppDbDependTransaction, use_ta);
			THROW(tra);
			if(p_ct_prcssr)
				THROW(p_ct_prcssr->Start());
			for(InitIteration(); NextIteration(&rec) > 0; PPWaitPercent(GetCounter())) {
				/* @v8.5.11
				if((!CreatorList.IsExists() || CreatorList.CheckID(rec.CreatorID) > 0) && (!EmployerList.IsExists() || EmployerList.CheckID(rec.EmployerID) > 0) &&
					(!ClientList.IsExists() || ClientList.CheckID(rec.ClientID) > 0)) {
					if((filt.StartTmPeriodBeg || filt.StartTmPeriodEnd) && filt.StartTmPeriodEnd >= filt.StartTmPeriodBeg)
						if(rec.StartTm < filt.StartTmPeriodBeg || rec.StartTm > filt.StartTmPeriodEnd)
							continue;
					*/
					if(p_ct_prcssr) {
						THROW(p_ct_prcssr->ProcessRec(&rec));
					}
					else {
						TempOrderTbl::Rec ord_rec;
						MakeTempEntry(&rec, &ord_rec);
						THROW_DB(p_bei->insert(&ord_rec));
					}
					THROW(AddItemToTimeGrid(&rec, 0));
				// @v8.5.11 }
			}
			if(p_ct_prcssr) {
				THROW(p_ct_prcssr->Finish());
			}
			else {
				THROW_DB(p_bei->flash());
			}
			THROW(tra.Commit());
		}
		delete p_bei;
		delete p_ct_prcssr;
		Filt.TabType = filt.TabType;
		P_TempOrd = p_ord;
	}
	{
		class PrjTaskCrosstab : public Crosstab {
		public:
			SLAPI  PrjTaskCrosstab(PPViewPrjTask * pV) : Crosstab()
			{
				P_V = pV;
			}
			virtual BrowserWindow * SLAPI CreateBrowser(uint brwId, int dataOwner)
			{
				PPViewBrowser * p_brw = new PPViewBrowser(brwId, CreateBrowserQuery(), P_V, dataOwner);
				SetupBrowserCtColumns(p_brw);
				return p_brw;
			}
		protected:
			virtual int SLAPI GetTabTitle(const void * pVal, TYPEID typ, SString & rBuf) const
			{
				return (pVal && /*typ == MKSTYPE(S_INT, 4) &&*/ P_V) ?
					P_V->GetTabTitle(*(const long *)pVal, rBuf) : 0;
			}
			PPViewPrjTask * P_V;
		};
		ZDELETE(P_Ct);
		if(Filt.TabType && P_TempTbl) {
			SString temp_buf;
			DBFieldList total_list;
			THROW_MEM(P_Ct = new PrjTaskCrosstab(this));
			P_Ct->SetTable(P_TempTbl, P_TempTbl->TabID);
			if(Filt.TabType == PrjTaskFilt::crstDateHour) {
				P_Ct->AddIdxField(P_TempTbl->StartDt);
				P_Ct->AddInheritedFixField(P_TempTbl->ClientName);
			}
			else if(oneof2(Filt.TabType, PrjTaskFilt::crstClientDate, PrjTaskFilt::crstClientEmployer)) {
				P_Ct->AddIdxField(P_TempTbl->ClientID);
				P_Ct->AddInheritedFixField(P_TempTbl->ClientName);
			}
			else if(oneof2(Filt.TabType, PrjTaskFilt::crstEmployerDate, PrjTaskFilt::crstEmployerHour)) {
				P_Ct->AddIdxField(P_TempTbl->EmployerID);
				P_Ct->AddInheritedFixField(P_TempTbl->EmployerName);
			}
			P_Ct->AddAggrField(P_TempTbl->TabParam);
			total_list.Add(P_TempTbl->TabParam);
			P_Ct->AddTotalRow(total_list, 0, PPGetWord(PPWORD_TOTAL, 0, temp_buf));
			P_Ct->AddTotalColumn(P_TempTbl->TabParam, 0, PPGetWord(PPWORD_TOTAL, 0, temp_buf));
			THROW(P_Ct->Create(use_ta));
		}
	}
	CATCH
		ZDELETE(P_Ct);
		ZDELETE(p_bei);
		ZDELETE(p_ord);
		ZDELETE(P_TempTbl);
		ok = 0;
	ENDCATCH
	PPWait(0);
	return ok;
}

class PrjTaskFiltDialog : public TDialog {
public:
	PrjTaskFiltDialog() : TDialog(DLG_TODOFILT)
	{
		SetupCalPeriod(CTLCAL_TODOFILT_PERIOD, CTL_TODOFILT_PERIOD);
		SetupCalPeriod(CTLCAL_TODOFILT_START, CTL_TODOFILT_START);
		SetupCalPeriod(CTLCAL_TODOFILT_ESTFINISH, CTL_TODOFILT_ESTFINISH);
		SetupCalPeriod(CTLCAL_TODOFILT_FINISH, CTL_TODOFILT_FINISH);
	}
	int    setDTS(const PrjTaskFilt *);
	int    getDTS(PrjTaskFilt *);
private:
	DECL_HANDLE_EVENT;
	int    SetupCtrls();
	PrjTaskFilt Data;
};

IMPL_HANDLE_EVENT(PrjTaskFiltDialog)
{
	TDialog::handleEvent(event);
	if(event.isCbSelected(CTLSEL_TODOFILT_CROSSTAB)) {
		getCtrlData(CTLSEL_TODOFILT_CROSSTAB, &Data.TabType);
		SetupCtrls();
	}
	else if(event.isClusterClk(CTL_TODOFILT_KIND)) {
		GetClusterData(CTL_TODOFILT_KIND, &Data.Kind);
		SetupCtrls();
	}
	else
		return;
	clearEvent(event);
}

int PrjTaskFiltDialog::SetupCtrls()
{
	Data.Order = (Data.TabType != PrjTaskFilt::crstNone) ? PrjTaskFilt::ordByDefault : Data.Order;
	Data.Kind = (Data.TabType != PrjTaskFilt::crstNone) ? TODOKIND_TASK : Data.Kind;
	setCtrlData(CTLSEL_TODOFILT_ORDER, &Data.Order);
	disableCtrls(Data.TabType != PrjTaskFilt::crstNone, CTL_TODOFILT_KIND, CTLSEL_TODOFILT_ORDER, 0L);
	SetClusterData(CTL_TODOFILT_KIND, Data.Kind);
	if(Data.TabType == PrjTaskFilt::crstNone)
		Data.TabParam = PrjTaskFilt::ctpNone;
	else
		Data.TabParam = (Data.TabParam == PrjTaskFilt::ctpNone) ? PrjTaskFilt::ctpUnComplTask : Data.TabParam;
	setCtrlData(CTLSEL_TODOFILT_CTPAR, &Data.TabParam);
	disableCtrl(CTLSEL_TODOFILT_CTPAR, Data.TabType == PrjTaskFilt::crstNone);
	if(Data.Kind == TODOKIND_TEMPLATE)
		SetupPPObjCombo(this, CTLSEL_TODOFILT_TEMPLATE, PPOBJ_PRJTASK, 0, 0, (void *)TODOKIND_TEMPLATE);
	disableCtrls(Data.Kind == TODOKIND_TEMPLATE, CTL_TODOFILT_TEMPLATE, CTLSEL_TODOFILT_TEMPLATE, 0L);
	disableCtrl(CTLSEL_TODOFILT_SUBST, !oneof2(Data.TabType, PrjTaskFilt::crstClientDate, PrjTaskFilt::crstEmployerDate));
	if(Data.TabType != PrjTaskFilt::crstClientDate && Data.TabType != PrjTaskFilt::crstEmployerDate)
		setCtrlLong(CTLSEL_TODOFILT_SUBST, 0);
	return 1;
}

int PrjTaskFiltDialog::setDTS(const PrjTaskFilt * pData)
{
	Data = *pData;

	AddClusterAssoc(CTL_TODOFILT_KIND, 0, TODOKIND_TASK);
	AddClusterAssoc(CTL_TODOFILT_KIND, 1, TODOKIND_TEMPLATE);

	AddClusterAssoc(CTL_TODOFILT_FLAGS, 0, PrjTaskFilt::fUnbindedOnly);
	AddClusterAssoc(CTL_TODOFILT_FLAGS, 1, PrjTaskFilt::fUnviewedOnly);
	AddClusterAssoc(CTL_TODOFILT_FLAGS, 2, PrjTaskFilt::fUnviewedEmployerOnly);
	SetClusterData(CTL_TODOFILT_FLAGS, Data.Flags);

	SetPeriodInput(this, CTL_TODOFILT_PERIOD, &Data.Period);
	SetPeriodInput(this, CTL_TODOFILT_START,  &Data.StartPeriod);
	SetPeriodInput(this, CTL_TODOFILT_ESTFINISH, &Data.EstFinishPeriod);
	SetPeriodInput(this, CTL_TODOFILT_FINISH,  &Data.FinishPeriod);

	SetupPPObjCombo(this,  CTLSEL_TODOFILT_TEMPLATE, PPOBJ_PRJTASK, Data.TemplateID, 0, (void *)TODOKIND_TEMPLATE);
	SetupPersonCombo(this, CTLSEL_TODOFILT_CREATOR,  Data.CreatorID, 0, (PPID)PPPRK_EMPL, 0);
	SetupPersonCombo(this, CTLSEL_TODOFILT_EMPLOYER, Data.EmployerID, 0, (PPID)PPPRK_EMPL, 0);
	SetupPersonCombo(this, CTLSEL_TODOFILT_CLIENT,   Data.ClientID, 0, (PPID)PPPRK_CLIENT, 0);
	SetupPPObjCombo(this,  CTLSEL_TODOFILT_CITY,     PPOBJ_WORLD,     Data.CliCityID, 0, PPObjWorld::MakeExtraParam(WORLDOBJ_CITY, 0, 0));
	SetupStringCombo(this, CTLSEL_TODOFILT_ORDER,    PPTXT_TODOORDER,     Data.Order);
	SetupStringCombo(this, CTLSEL_TODOFILT_CROSSTAB, PPTXT_TODOCROSSTAB,  Data.TabType);
	SetupStringCombo(this, CTLSEL_TODOFILT_CTPAR,    PPTXT_TODOCTPARAMS,  Data.TabParam);
	SetupSubstDateCombo(this, CTLSEL_TODOFILT_SUBST, Data.Sgd);

	int    i;
	ushort v = 0;
	PPIDArray list;
	if(Data.GetPriorList(&list) < 0)
		v = 0x001f;
	else
		for(i = 1; i <= 5; i++)
			if(list.lsearch(i))
				v |= (1 << (i-1));
	setCtrlData(CTL_TODOFILT_PRIOR, &v);
	v = 0;
	if(Data.GetStatusList(&list) < 0)
		v = 0x001f;
	else
		for(i = 1; i <= 5; i++)
			if(list.lsearch(i))
				v |= (1 << (i-1));
	setCtrlData(CTL_TODOFILT_STATUS, &v);
	SetupCtrls();
	return 1;
}

int PrjTaskFiltDialog::getDTS(PrjTaskFilt * pData)
{
	int    ok = 1;
	uint   sel = 0;

	GetClusterData(CTL_TODOFILT_KIND, &Data.Kind);
	GetClusterData(CTL_TODOFILT_FLAGS, &Data.Flags);
	THROW(GetPeriodInput(this, sel = CTL_TODOFILT_PERIOD, &Data.Period));
	THROW(GetPeriodInput(this, sel = CTL_TODOFILT_START,  &Data.StartPeriod));
	THROW(GetPeriodInput(this, sel = CTL_TODOFILT_ESTFINISH, &Data.EstFinishPeriod));
	THROW(GetPeriodInput(this, sel = CTL_TODOFILT_FINISH,    &Data.FinishPeriod));
	getCtrlData(CTLSEL_TODOFILT_TEMPLATE, &Data.TemplateID);
	getCtrlData(CTLSEL_TODOFILT_CREATOR,  &Data.CreatorID);
	getCtrlData(CTLSEL_TODOFILT_EMPLOYER, &Data.EmployerID);
	getCtrlData(CTLSEL_TODOFILT_CLIENT,   &Data.ClientID);
	getCtrlData(CTLSEL_TODOFILT_CITY,     &Data.CliCityID);
	getCtrlData(CTLSEL_TODOFILT_ORDER,    &Data.Order);
	getCtrlData(CTLSEL_TODOFILT_CROSSTAB, &Data.TabType);
	if(Data.StartPeriod.IsZero() && Data.TabType != PrjTaskFilt::crstNone) {
		const LDATE oper_date = LConfig.OperDate;
		if(oneof2(Data.TabType, PrjTaskFilt::crstDateHour, PrjTaskFilt::crstEmployerHour))
			Data.StartPeriod.low = Data.StartPeriod.upp = oper_date;
		else {
			LDATE odt = oper_date;
			encodedate(1, odt.month(), odt.year(), &Data.StartPeriod.low);
			encodedate(dayspermonth(odt.month(), odt.year()), odt.month(), odt.year(), &Data.StartPeriod.upp);
		}
	}
	getCtrlData(CTLSEL_TODOFILT_SUBST,          &Data.Sgd);
	getCtrlData(sel = CTLSEL_TODOFILT_CTPAR,    &Data.TabParam);
	int    i;
	ushort v = getCtrlUInt16(CTL_TODOFILT_PRIOR);
	for(i = 1; i <= 5; i++)
		if(v & (1 << (i-1)))
			Data.IncludePrior(i);
		else
			Data.ExcludePrior(i);
	getCtrlData(CTL_TODOFILT_STATUS, &(v = 0));
	for(i = 1; i <= 5; i++)
		if(v & (1 << (i-1)))
			Data.IncludeStatus(i);
		else
			Data.ExcludeStatus(i);
	ASSIGN_PTR(pData, Data);
	CATCH
		ok = PPErrorByDialog(this, sel, -1);
	ENDCATCH
	return ok;
}

int SLAPI PPViewPrjTask::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	if(!Filt.IsA(pBaseFilt))
		return 0;
	DIALOG_PROC_BODY(PrjTaskFiltDialog, (PrjTaskFilt *)pBaseFilt);
}

int SLAPI PPViewPrjTask::InitIteration()
{
	int    ok = 1;
	int    idx = 0;
	ZDELETE(P_IterQuery);
	if(Filt.TabType == PrjTaskFilt::crstNone) {
		if(P_TempOrd) {
			TempOrderTbl::Key1 k, k_;
			MEMSZERO(k);
			P_IterQuery = new BExtQuery(P_TempOrd, 1, 128);
			P_IterQuery->select(P_TempOrd->ID, P_TempOrd->Name, 0L);
			k_ = k;
			Counter.Init(P_IterQuery->countIterations(0, &k_, spGe));
			P_IterQuery->initIteration(0, &k, spGe);
		}
		else {
			DBQ  * dbq = 0;
			PrjTaskTbl * t = TodoObj.P_Tbl;
			union {
				PrjTaskTbl::Key1 k1;
				PrjTaskTbl::Key2 k2;
				PrjTaskTbl::Key3 k3;
				PrjTaskTbl::Key4 k4;
				PrjTaskTbl::Key5 k5;
				PrjTaskTbl::Key6 k6;
			} k, k_;
			MEMSZERO(k);
			if(Filt.LinkTaskID) {
				idx = 6;
				k.k6.LinkTaskID = Filt.LinkTaskID;
			}
			else if(Filt.ProjectID) {
				idx = 2;
				k.k2.ProjectID = Filt.ProjectID;
				k.k2.Dt = Filt.Period.low;
			}
			else if(ClientList.GetSingle()) {
				idx = 5;
				k.k5.ClientID = ClientList.GetSingle();
				k.k5.Dt = Filt.Period.low;
			}
			else if(EmployerList.GetSingle()) {
				idx = 4;
				k.k4.EmployerID = EmployerList.GetSingle();
				k.k4.Dt = Filt.Period.low;
			}
			else if(Filt.TemplateID) {
				idx = 3;
				k.k3.TemplateID = Filt.TemplateID;
				k.k3.Dt = Filt.Period.low;
			}
			else {
				idx = 1;
				k.k1.Dt = Filt.Period.low;
			}
			P_IterQuery = new BExtQuery(t, idx, 128);
			dbq = ppcheckfiltid(dbq, t->ProjectID,  Filt.ProjectID);
			dbq = ppcheckfiltid(dbq, t->ClientID,   ClientList.GetSingle());
			dbq = ppcheckfiltid(dbq, t->EmployerID, EmployerList.GetSingle());
			dbq = ppcheckfiltid(dbq, t->TemplateID, Filt.TemplateID);
			dbq = ppcheckfiltid(dbq, t->CreatorID,  CreatorList.GetSingle());
			dbq = ppcheckfiltid(dbq, t->LinkTaskID, Filt.LinkTaskID);
			dbq = ppcheckfiltid(dbq, t->Kind, Filt.Kind);
			dbq = &(*dbq && daterange(t->Dt, &Filt.Period));
			dbq = &(*dbq && daterange(t->StartDt, &Filt.StartPeriod));
			dbq = &(*dbq && daterange(t->EstFinishDt, &Filt.EstFinishPeriod));
			dbq = &(*dbq && daterange(t->FinishDt, &Filt.FinishPeriod));
			if(Filt.Flags & PrjTaskFilt::fUnbindedOnly)
				dbq = &(*dbq && t->ProjectID == 0L);
			//
			// Поля Priority, Status, ClientID, DlvrAddrID участвуют в выборке по тому, что
			// функция NextIteration использует их для дополнительной фильтрации
			//
			P_IterQuery->select(t->ID, t->Priority, t->Status, t->ClientID,
				t->DlvrAddrID, t->Flags, t->OpenCount, 0L).where(*dbq);
			k_ = k;
			Counter.Init(P_IterQuery->countIterations(0, &k_, spGe));
			P_IterQuery->initIteration(0, &k, spGe);
		}
	}
	else {
		union {
			TempPrjTaskTbl::Key0 k0;
			TempPrjTaskTbl::Key1 k1;
			TempPrjTaskTbl::Key2 k2;
		} k, k_;
		MEMSZERO(k);
		if(Filt.TabType == PrjTaskFilt::crstDateHour)
			idx = 0;
		else if(Filt.TabType == PrjTaskFilt::crstClientDate || Filt.TabType == PrjTaskFilt::crstClientEmployer)
			idx = 1;
		else if(Filt.TabType == PrjTaskFilt::crstEmployerDate)
			idx = 2;
		P_IterQuery = new BExtQuery(P_TempTbl, idx);
		P_IterQuery->selectAll();
		k_ = k;
		Counter.Init(P_IterQuery->countIterations(0, &k_, spGe));
		P_IterQuery->initIteration(0, &k, spGe);
	}
	return ok;
}

int SLAPI PPViewPrjTask::NextInnerIteration(PrjTaskViewItem * pItem)
{
	int    ok = 1;
	if(Item.ID) {
		if(Filt.TabType == PrjTaskFilt::crstNone) {
			if(!UndefPriorList && !PriorList.lsearch(Item.Priority))
				ok = -1;
			if(ok > 0 && (!UndefStatusList) && (!StatusList.lsearch(Item.Status)))
				ok = -1;
			if(ok > 0 && (Filt.Flags & PrjTaskFilt::fUnviewedOnly) && Item.OpenCount > 0)
				ok = -1;
			if(ok > 0 && (Filt.Flags & PrjTaskFilt::fUnviewedEmployerOnly) && (Item.Flags & TODOF_OPENEDBYEMPL))
				ok = -1;
			if(ok > 0 && Filt.CliCityID) {
				PPID   addr_id = 0, city_id = 0;
				if(Item.ClientID && PsnObj.GetAddrID(Item.ClientID, Item.DlvrAddrID, PSNGETADDRO_DLVRADDR, &addr_id) > 0) {
					PsnObj.GetCityByAddr(addr_id, &city_id, 0);
					if(city_id != Filt.CliCityID)
						ok = -1;
				}
				else
					ok = -1;
			}
		}
	}
	else
		ok = -1;
	if(ok > 0)
		ASSIGN_PTR(pItem, Item);
	return ok;
}

int SLAPI PPViewPrjTask::NextOuterIteration()
{
	int    ok = -1;
	PrjTaskTbl::Rec rec;
	MEMSZERO(rec);
	if(P_TempOrd) {
		if(P_IterQuery->nextIteration() > 0) {
			Counter.Increment();
			ok = TodoObj.Search(P_TempOrd->data.ID, &rec);
		}
	}
	else {
		if((ok = P_IterQuery->nextIteration()) > 0) {
			Counter.Increment();
			if(Filt.TabType == PrjTaskFilt::crstNone)
				ok = TodoObj.Search(TodoObj.P_Tbl->data.ID, &rec);
			else {
				rec.ID         = P_TempTbl->data.TabID;
				rec.Dt.v       = (ulong)P_TempTbl->data.TabParam;
				rec.EmployerID = P_TempTbl->data.EmployerID;
				rec.ClientID   = P_TempTbl->data.ClientID;
				rec.StartDt    = P_TempTbl->data.StartDt;
			}
		}
	}
	if(ok > 0)
		Item = rec;
	return ok;
}

int SLAPI PPViewPrjTask::NextIteration(PrjTaskViewItem * pItem)
{
	int    ok = -1;
	const  int tm_period_valid = BIN((Filt.StartTmPeriodBeg || Filt.StartTmPeriodEnd) && Filt.StartTmPeriodEnd >= Filt.StartTmPeriodBeg);
	MEMSZERO(Item);
	do {
		if(NextInnerIteration(pItem) > 0) {
			// @v8.5.11 {
			if(CreatorList.IsExists() && !CreatorList.CheckID(Item.CreatorID))
				continue;
			else if(EmployerList.IsExists() && !EmployerList.CheckID(Item.EmployerID))
				continue;
			else if(ClientList.IsExists() && !ClientList.CheckID(Item.ClientID))
				continue;
			else if(tm_period_valid && (Item.StartTm < Filt.StartTmPeriodBeg || Item.StartTm > Filt.StartTmPeriodEnd))
				continue;
			else
			// } @v8.5.11
				ok = 1;
		}
	} while(ok < 0 && NextOuterIteration() > 0);
	return ok;
}

int SLAPI PPViewPrjTask::Transmit(PPID /*id*/, int kind)
{
	int    ok = -1;
	if(kind == 0) {
		ObjTransmitParam param;
		if(ObjTransmDialog(DLG_OBJTRANSM, &param) > 0) {
			PrjTaskViewItem item;
			const  PPIDArray & rary = param.DestDBDivList.Get();
			PPObjIDArray objid_ary;
			PPWait(1);
			for(InitIteration(); NextIteration(&item) > 0; PPWaitPercent(GetCounter()))
				objid_ary.Add(PPOBJ_PRJTASK, item.ID);
			THROW(PPObjectTransmit::Transmit(&rary, &objid_ary, &param));
			ok = 1;
		}
	}
	else if(kind == 1) {  // @reserve for transmit charry
	}
	else if(kind == 2) {
		SString path, ical_path;
		VCalendar vcal;
		PrjTaskViewItem item;
		VCalendar::Todo vrec;

		PPGetFilePath(PPPATH_OUT, PPFILNAM_VCALTODO, path);
		PPGetFilePath(PPPATH_OUT, PPFILNAM_ICALTODO, ical_path);
		THROW(vcal.Open(path, 1));
		PPWait(1);
		for(InitIteration(); NextIteration(&item) > 0; PPWaitPercent(GetCounter())) {
			vrec.Init();
			vrec.CreatedDtm.Set(item.Dt, item.Tm);
			vrec.StartDtm.Set(item.StartDt, item.StartTm);
			vrec.CompletedDtm.Set(item.FinishDt, item.FinishTm);
			vrec.DueDtm.Set(item.EstFinishDt, item.EstFinishTm);
			vrec.Sequence = (int16)item.OpenCount;
			vrec.Priority = item.Priority;
			if(item.Status == TODOSTTS_NEW)
				vrec.Status = VCalendar::stAccepted;
			else if(item.Status == TODOSTTS_REJECTED)
				vrec.Status = VCalendar::stDeclined;
			else if(item.Status == TODOSTTS_INPROGRESS)
				vrec.Status = VCalendar::stConfirmed;
			else if(item.Status == TODOSTTS_ONHOLD)
				vrec.Status = VCalendar::stNeedsAction;
			else if(item.Status == TODOSTTS_COMPLETED)
				vrec.Status = VCalendar::stCompleted;
			PPGetWord(PPWORD_MISCELLANEOUS, 0, vrec.Category);
			vrec.Classification = VCalendar::clPublic;
			GetObjectName(PPOBJ_PERSON, item.EmployerID, vrec.Owner);
			// @v9.4.3 {
			if(item.ClientID) {
				GetObjectName(PPOBJ_PERSON, item.ClientID, vrec.Contact);
			}
			// } @v9.4.3
			PPGetWord(PPWORD_WORK, 0, vrec.Location);
			vrec.Summary = item.Descr;
			vrec.Descr = item.Descr;
			THROW(vcal.PutTodo(&vrec));
			PPWaitPercent(GetCounter());
		}
		vcal.Close();
		SCopyFile(path, ical_path, 0, 0, 0);
	}
	CATCHZOKPPERR
	PPWait(0);
	return ok;
}

int SLAPI PPViewPrjTask::ViewTotal()
{
	TDialog * dlg = new TDialog(DLG_TODOTOTAL);
	if(CheckDialogPtr(&dlg, 1)) {
		long   count = 0;
		PrjTaskViewItem item;
		PPWait(1);
		for(InitIteration(); NextIteration(&item) > 0; PPWaitPercent(GetCounter()))
			count++;
		PPWait(0);
		dlg->setCtrlLong(CTL_TODOTOTAL_COUNT, count);
		ExecViewAndDestroy(dlg);
		return -1;
	}
	else
		return 0;
}

int SLAPI PPViewPrjTask::Detail(const void * pHdr, PPViewBrowser * pBrw)
{
	PrjTaskTbl::Rec rec;
	PPID   templ_id = pHdr ? *(PPID *)pHdr : 0;
	if(TodoObj.Search(templ_id, &rec) > 0 && rec.Kind == TODOKIND_TEMPLATE) {
		PrjTaskFilt filt;
		filt.TemplateID = templ_id;
		ViewPrjTask(&filt);
	}
	return -1;
}

int SLAPI PPViewPrjTask::ViewCrosstabDetail(PPID tabID, const DBFieldList * pFldList)
{
	int    ok = -1;
	if(P_TempTbl && pFldList) {
		PPID tab_id = tabID;
		union {
			LDATE dt;
			PPID  id;
		} k;
		MEMSZERO(k);
		if(Filt.TabType == PrjTaskFilt::crstDateHour)
			 pFldList->GetValue(0, &k.dt, 0);
		else
			pFldList->GetValue(0, &k.id, 0);
		PrjTaskFilt filt = Filt;
		if(Filt.TabType == PrjTaskFilt::crstDateHour) {
			filt.StartPeriod.low = filt.StartPeriod.upp = k.dt;
			if(tab_id) {
				filt.StartTmPeriodBeg = encodetime(tab_id, 0, 0, 0);
				filt.StartTmPeriodEnd = encodetime((tab_id == 24) ? 0 : tab_id, 59, 59, 999);
			}
		}
		else if(Filt.TabType == PrjTaskFilt::crstClientDate || Filt.TabType == PrjTaskFilt::crstEmployerDate) {
			if(Filt.TabType == PrjTaskFilt::crstClientDate)
				filt.ClientID = k.id;
			else
				filt.EmployerID = k.id;
			if(tab_id) {
				LDATE dt = ZERODATE;
				dt.v = tab_id;
				if(Filt.Sgd != sgdNone) {
					ExpandSubstDate(Filt.Sgd, dt, &filt.StartPeriod);
					filt.Sgd = sgdNone;
				}
				else
					filt.StartPeriod.low = filt.StartPeriod.upp = dt;
			}
		}
		else if(Filt.TabType == PrjTaskFilt::crstClientEmployer) {
			filt.ClientID   = k.id;
			if(tab_id)
				filt.EmployerID = tab_id;
		}
		else if(Filt.TabType == PrjTaskFilt::crstEmployerHour) {
			filt.EmployerID = k.id;
			if(tab_id) {
				filt.StartTmPeriodBeg = encodetime(tab_id, 0, 0, 0);
				filt.StartTmPeriodEnd = encodetime((tab_id == 24) ? 0 : tab_id, 59, 59, 999);
			}
		}
		filt.TabType = PrjTaskFilt::crstNone;
		THROW(ok = ViewPrjTask(&filt));
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewPrjTask::CreateByTemplate()
{
	int    ok = -1, r = 0;
	PPIDArray id_list;
	DateRange period;
	period.SetZero();
	if(Filt.Kind == TODOKIND_TEMPLATE) {
		PrjTaskViewItem item;
		if(DateRangeDialog(0, 0, &period) > 0) {
			PPTransaction tra(1);
			THROW(tra);
			for(InitIteration(); NextIteration(&item) > 0;) {
				THROW(r = TodoObj.CreateByTemplate(item.ID, &period, &id_list, 0));
				if(r > 0) {
					UpdateTempTable(&id_list, 0);
					ok = 1;
				}
			}
			THROW(tra.Commit());
		}
	}
	else if(Filt.TemplateID) {
		if(DateRangeDialog(0, 0, &period) > 0) {
			PPTransaction tra(1);
			THROW(tra);
			THROW(r = TodoObj.CreateByTemplate(Filt.TemplateID, &period, &id_list, 0));
			if(r > 0) {
				UpdateTempTable(&id_list, 0);
				ok = 1;
			}
			THROW(tra.Commit());
		}
	}
	if(ok > 0)
		UpdateTimeBrowser(0);
	CATCHZOKPPERR
	return ok;
}

void * SLAPI PPViewPrjTask::GetEditExtraParam()
{
	PPID   extra_param = 0;
	if(Filt.ProjectID)
		extra_param = Filt.ProjectID;
	else if(EmployerList.GetSingle())
		extra_param = EmployerList.GetSingle() + PRJTASKBIAS_EMPLOYER;
	else if(ClientList.GetSingle())
		extra_param = ClientList.GetSingle() + PRJTASKBIAS_CLIENT;
	if(Filt.Kind == TODOKIND_TEMPLATE)
		extra_param += PRJTASKBIAS_TEMPLATE;
	return (void *)extra_param;
}

int SLAPI PPViewPrjTask::Print(const void *)
{
	return Helper_Print(Filt.TabType == PrjTaskFilt::crstNone ?
		REPORT_PRJTASKVIEW : REPORT_PRJTASKVIEWCT, Filt.Order);
}

int SLAPI PPViewPrjTask::Export()
{
	return -1;
}

DBQuery * SLAPI PPViewPrjTask::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	static DbqStringSubst prior_subst(5);  // @global @threadsafe
	static DbqStringSubst status_subst(5); // @global @threadsafe
	DBQuery * q  = 0;
	PrjTaskTbl * t = 0;
	TempOrderTbl * p_ord = 0;
	uint   brw_id = 0;
	if(P_Ct == 0) {
		PPIDArray prior_list, status_list;
		DBQ  * dbq = 0;
		DBE  * dbe_prior = 0;
		DBE  * dbe_status = 0;
		DBE    dbe_psn_cli, dbe_psn_emp;

		brw_id = (Filt.Kind == TODOKIND_TEMPLATE) ? BROWSER_PRJTASKTEMPL : BROWSER_PRJTASK;
		THROW(CheckTblPtr(p_ord = new TempOrderTbl(P_TempOrd->fileName)));
		THROW(CheckTblPtr(t = new PrjTaskTbl));
		PPDbqFuncPool::InitObjNameFunc(dbe_psn_cli, PPDbqFuncPool::IdObjNamePerson, t->ClientID);
		PPDbqFuncPool::InitObjNameFunc(dbe_psn_emp, PPDbqFuncPool::IdObjNamePerson, t->EmployerID);

		dbe_prior  = & enumtoa(t->Priority, 5, prior_subst.Get(PPTXT_TODO_PRIOR));
		dbe_status = & enumtoa(t->Status,   5, status_subst.Get(PPTXT_TODO_STATUS));
		dbq = &(*dbq && t->ID == p_ord->ID);
		q = & select(
			t->ID,          // #0
			t->Code,        // #1
			t->Dt,          // #2
			t->StartDt,     // #3
			t->EstFinishDt, // #4
			t->FinishDt,    // #5
			*dbe_prior,     // #6
			*dbe_status,    // #7
			dbe_psn_cli,    // #8
			dbe_psn_emp,    // #9
			t->Descr,       // #10
			0L).from(p_ord, t, 0L).where(*dbq);
		delete dbe_prior;
		delete dbe_status;
		q->orderBy(p_ord->Name, 0L);
		THROW(CheckQueryPtr(q));
	}
	else {
		if(Filt.TabType == PrjTaskFilt::crstDateHour)
			brw_id = BROWSER_PRJTASK_DATEHOURCT;
		else if(Filt.TabType == PrjTaskFilt::crstClientDate)
			brw_id = BROWSER_PRJTASK_CLIENTDATECT;
		else if(Filt.TabType == PrjTaskFilt::crstEmployerDate)
			brw_id = BROWSER_PRJTASK_EMPLOYERDATECT;
		else
			brw_id = BROWSER_PRJTASK_CLIENTEMPLOYERCT;
		q = PPView::CrosstabDbQueryStub;
	}
	if(pSubTitle) {
		PPObjProject prj_obj;
		SString name_buf;
		*pSubTitle = 0;
		if(Filt.ProjectID)
			prj_obj.GetFullName(Filt.ProjectID, *pSubTitle);
		if(Filt.TemplateID) {
			pSubTitle->CatDiv('-', 1, 1);
			GetObjectName(PPOBJ_PRJTASK, Filt.TemplateID, *pSubTitle, 1);
		}
		if(Filt.EmployerID) {
			GetPersonName(Filt.EmployerID, name_buf);
			pSubTitle->CatDiv('-', 1, 1).Cat(name_buf);
		}
		if(Filt.ClientID) {
			GetPersonName(Filt.ClientID, name_buf);
			pSubTitle->CatDiv('-', 1, 1).Cat(name_buf);
		}
		if(Filt.LinkTaskID) {
			SString  link_task_name;
			GetObjectName(PPOBJ_PRJTASK, Filt.LinkTaskID, link_task_name, 0);
			if(!link_task_name.NotEmptyS())
				link_task_name.CatChar('#').Cat(Filt.LinkTaskID);
			PPLoadString("connectedto", *pSubTitle);
			pSubTitle->Space().Cat(link_task_name);
		}
	}
	CATCH
		if(q)
			ZDELETE(q);
		else {
			delete p_ord;
			delete t;
		}
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

#define CHNGTASKS_DELALL  1
#define CHNGTASKS_STATUS  2

int SLAPI PPViewPrjTask::ChangeTasks(PPIDArray * pAry)
{
	int    ok = -1;
	long   flags = CHNGTASKS_DELALL;
	PPIDArray list;
	TDialog * p_dlg = 0;
	THROW(TodoObj.CheckRights(PRJTASKRT_MULTCHANGE));
	THROW(CheckDialogPtr(&(p_dlg = new TDialog(DLG_CHNGTASKS)), 0));
	p_dlg->AddClusterAssoc(CTL_CHNGTASKS_FLAGS, -1, CHNGTASKS_DELALL);
	p_dlg->AddClusterAssoc(CTL_CHNGTASKS_FLAGS,  0, CHNGTASKS_DELALL);
	p_dlg->AddClusterAssoc(CTL_CHNGTASKS_FLAGS,  1, CHNGTASKS_STATUS);
	p_dlg->SetClusterData(CTL_CHNGTASKS_FLAGS, flags);
	if(ExecView(p_dlg) == cmOK) {
		p_dlg->GetClusterData(CTL_CHNGTASKS_FLAGS, &flags);
		ok = 1;
	}
	if(ok > 0) {
		PrjTaskViewItem item;
		PrjTaskTbl::Rec rec;
		PPTransaction tra(1);
		THROW(tra);
		PPWait(1);
		for(InitIteration(); NextIteration(&item) > 0;) {
			list.add(item.ID);
			if(flags == CHNGTASKS_STATUS) {
				MEMSZERO(rec);
				THROW(TodoObj.Search(item.ID, &rec) > 0);
				rec.Status = TODOSTTS_COMPLETED;
				THROW(TodoObj.PutPacket(&item.ID, &rec, 0));
			}
			PPWaitPercent(Counter);
		}
		if(list.getCount() && flags == CHNGTASKS_DELALL) {
			DBQ * dbq = 0;
			for(uint i = 0; i < list.getCount(); i++) {
				THROW(deleteFrom(TodoObj.P_Tbl, 0, (TodoObj.P_Tbl->ID == list.at(i))));
			}
		}
		THROW(tra.Commit());
		ASSIGN_PTR(pAry, list);
	}
	CATCHZOKPPERR
	delete p_dlg;
	PPWait(0);
	return ok;
}

int SLAPI PPViewPrjTask::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	PPIDArray id_list;
	PPID   id = (pHdr) ? *(PPID *)pHdr : 0;
	PPID   temp_id;
	int    ok = 0;
	/*
	if(ppvCmd == PPVCMD_PRINT || Filt.TabType == PrjTaskFilt::crstNone) {
		if(ppvCmd != PPVCMD_ADDITEM)
			ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
		else
			ok = -2;
	}
	else if(ppvCmd == PPVCMD_EDITITEM)
		ok = -2;
	*/
	if(ppvCmd == PPVCMD_EDITITEM && P_Ct)
		ok = -2;
	else
		ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		switch(ppvCmd) {
			/*
			case PPVCMD_ADDITEM:
				ok = (TodoObj.Edit(&(temp_id = 0), GetEditExtraParam()) == cmOK) ? 1 : -1;
				if(ok > 0)
					id_list.add(temp_id);
				break;
			*/
			case PPVCMD_ADDBYSAMPLE:
				ok = TodoObj.AddBySample(&(temp_id = 0), id);
				if(ok > 0)
					id_list.add(temp_id);
				break;
			case PPVCMD_BUILD:
				ok = CreateByTemplate();
				break;
			case PPVCMD_CHANGEFILT:
				{
					STimeChunkBrowser * p_brw = PPFindLastTimeChunkBrowser();
					ok = ChangeFilt(0, pBrw);
					if(p_brw)
						if(ok == 2) {
							PPCloseBrowser(p_brw);
							TimeChunkBrowser();
							::SetFocus(pBrw->H());
						}
						else if(ok > 0)
							p_brw->UpdateData();
				}
				break;
			case PPVCMD_EDITITEM:
				if(P_Ct) {
					uint   tab_idx = pBrw ? pBrw->GetCurColumn() : 0;
					PPID   tab_id = 0;
					DBFieldList fld_list; // realy const, do not modify
					int    r = (tab_idx > 0) ? P_Ct->GetTab(tab_idx - 1, &tab_id) : 1;
					if(r > 0 && P_Ct->GetIdxFields(id, &fld_list) > 0)
						ViewCrosstabDetail(tab_id, &fld_list);
				}
				break;
			case PPVCMD_EDITBILLPOOL:
				ViewBillsByPool(PPASS_TODOBILLPOOL, id);
				break;
			case PPVCMD_TRANSMIT:
				ok = -1;
				Transmit(0, 0);
				break;
			case PPVCMD_EXPORTVCAL:
				ok = -1;
				Transmit(0, 2);
				break;
			case PPVCMD_LINKTASKS:
				if(id) {
					PPIDArray  task_list;
					PPViewPrjTask * p_v = new PPViewPrjTask;
					PrjTaskFilt  lt_flt;
					lt_flt.Kind = TODOKIND_TASK;
					lt_flt.LinkTaskID = id;
					PPWait(1);
					TodoObj.GetLinkTasks(id, &task_list);
					if(p_v->Init_(&lt_flt) && p_v->Browse(0)) {
						TodoObj.GetLinkTasks(id, &id_list);
						for(int i = (int)task_list.getCount() - 1; i >= 0; i--)
							if(id_list.freeByKey(task_list.at(i), 0) > 0)
								task_list.atFree(i);
						id_list.add(&task_list);
						ok = id_list.getCount() ? 1 : -1;
					}
					else
						PPError();
					delete p_v;
				}
				break;
			case PPVCMD_DELETEALL:
				ok = ChangeTasks(&id_list);
				break;
			case PPVCMD_EDITLINKTASK:
				{
					PrjTaskTbl::Rec  pt_rec;
					if(id && TodoObj.Search(id, &pt_rec) > 0 && pt_rec.LinkTaskID) {
						ok = (TodoObj.Edit(&pt_rec.LinkTaskID, 0) == cmOK) ? 1 : -1;
						if(ok > 0)
							id_list.add(pt_rec.LinkTaskID);
					}
				}
				break;
			case PPVCMD_TIMEGRAPH:
				ok = -1;
				TimeChunkBrowser();
				break;
			case PPVCMD_MOUSEHOVER:
				{
					if(Filt.TabType == PrjTaskFilt::crstNone) {
						BrowserDef * p_def = pBrw->getDef();
						int    col = p_def ? (p_def->getCount() - 1) : -1;
						long   h = 0;
						pBrw->ItemByMousePos(&h, 0);
						if(col > 0 && h == col) {
							PrjTaskTbl::Rec  pt_rec;
							ok = -1;
							if(id && TodoObj.Search(id, &pt_rec) > 0 && (strlen(pt_rec.Descr) > 0 || strlen(pt_rec.Memo) > 0)) {
								long flags = SMessageWindow::fShowOnCursor|SMessageWindow::fCloseOnMouseLeave|SMessageWindow::fTextAlignLeft
									|SMessageWindow::fOpaque|SMessageWindow::fSizeByText|SMessageWindow::fChildWindow;
								SString buf;
								(buf = pt_rec.Descr).ReplaceChar('\n', ' ').ReplaceChar('\r', ' ');
								if(strlen(pt_rec.Memo) > 0) {
									SString word, memo;
									PPLoadString("memo", word);
									word.CatChar(':').CatChar('\n');
									(memo = pt_rec.Memo).ReplaceChar('\n', ' ').ReplaceChar('\r', ' ');
									buf.CatChar('\n').CatChar('\n').Cat(word).Cat(memo);
								}
								PPTooltipMessage(buf, 0, pBrw->H(), 10000, 0, flags);
							}
						}
					}
				}
				break;
		}
	}
	if(ok > 0 && oneof7(ppvCmd, PPVCMD_ADDITEM, PPVCMD_EDITITEM, PPVCMD_DELETEITEM,
		PPVCMD_ADDBYSAMPLE, PPVCMD_LINKTASKS, PPVCMD_DELETEALL, PPVCMD_EDITLINKTASK)) {
		if(!oneof4(ppvCmd, PPVCMD_ADDBYSAMPLE, PPVCMD_ADDITEM, PPVCMD_LINKTASKS, PPVCMD_EDITLINKTASK))
			id_list.add(id);
		if(UpdateTempTable(&id_list, 1) > 0)
			UpdateTimeBrowser(0);
	}
	return ok;
}

int SLAPI PPViewPrjTask::HandleNotifyEvent(int kind, const PPNotifyEvent * pEv, PPViewBrowser * pBrw, void * extraProcPtr)
{
	int    ok = -1, update = 0;
	if(pEv) {
		PPIDArray id_list;
		if(pEv->IsFinish() && kind == PPAdviseBlock::evTodoChanged && UpdateTaskList.getCount())
			update = 1;
		else if(kind == PPAdviseBlock::evTodoChanged)
			UpdateTaskList.addUnique(pEv->ObjID);
		ok = 1;
	}
	if(ok > 0 && update && pBrw) {
		if(UpdateTempTable(&UpdateTaskList, 1) > 0) {
			pBrw->refresh();
			UpdateTimeBrowser(0);
		}
		UpdateTaskList.freeAll();
	}
	return ok;
}

int SLAPI PPViewPrjTask::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw) {
		pBrw->Advise(PPAdviseBlock::evTodoChanged, 0, PPOBJ_PRJTASK, 0);
		return 1;
	}
	else
		return -1;
}
//
//
//
int SLAPI ViewPrjTask(const PrjTaskFilt * pFilt)
{
	return PPView::Execute(PPVIEW_PRJTASK, pFilt, PPView::exefModeless, 0);
}

int SLAPI ViewPrjTask_ByStatus()
{
	PrjTaskFilt filt;
	GetCurUserPerson(&filt.EmployerID, 0);
	filt.Flags = PrjTaskFilt::fUnviewedEmployerOnly;
	filt.Kind  = TODOKIND_TASK;
	for(uint i = 0; i < 5; i++) {
		filt.PriorList[i]  = i + 1;
		if(i != 1 && i != 4)
			filt.StatusList[i] = i + 1;
	}
	((PPApp*)APPL)->LastCmd = cmPrjTask;
	return ViewPrjTask(&filt);
}

// @v6.4.15 AHTOXA {
int SLAPI ViewPrjTask_ByReminder()
{
	int ok = -1;
	PPProjectConfig cfg;
	if(PPObjProject::ReadConfig(&cfg) > 0 && cfg.Flags & PRJCFGF_INCOMPLETETASKREMIND) {
		LDATE       cur_dt = ZERODATE;
		PrjTaskFilt filt;

		getcurdate(&cur_dt);
		filt.StartPeriod.low = filt.StartPeriod.upp = cur_dt;
		plusdate(&filt.StartPeriod.upp, abs(cfg.RemindPrd.low), 0);
		if(cfg.RemindPrd.low != cfg.RemindPrd.upp)
			plusdate(&filt.StartPeriod.low, -abs(cfg.RemindPrd.upp), 0);

		GetCurUserPerson(&filt.EmployerID, 0);
		filt.Kind  = TODOKIND_TASK;
		for(uint i = 0; i < 5; i++) {
			filt.PriorList[i]  = i + 1;
			if(i != 1 && i != 4)
				filt.StatusList[i] = i + 1;
		}
		((PPApp*)APPL)->LastCmd = cmPrjTask;
		ok = ViewPrjTask(&filt);
	}
	return ok;
}
// }@v6.4.15 AHTOXA
//
//
//
SLAPI PPViewPrjTask::PrjTaskTimeChunkGrid::PrjTaskTimeChunkGrid() : STimeChunkGrid()
{
}

SLAPI PPViewPrjTask::PrjTaskTimeChunkGrid::~PrjTaskTimeChunkGrid()
{
}

int PPViewPrjTask::PrjTaskTimeChunkGrid::GetText(int item, long id, SString & rBuf)
{
	int    ok = -1;
	if(item == iTitle) {
		rBuf = "TEST";
		ok = 1;
	}
	else if(item == iRow) {
		if(id < 0)
			ok = 1;
		else
			ok = STimeChunkGrid::GetText(item, id, rBuf);
	}
	else if(item == iChunk)
		ok = P_View->GetTimeGridItemText(id, rBuf);
	return ok;
}

int SLAPI PPViewPrjTask::GetTimeGridItemText(PPID taskID, SString & rBuf)
{
	int    ok = -1;
	PrjTaskTbl::Rec rec;
	if(TodoObj.Search(taskID, &rec) > 0) {
		if(strip(rec.Descr)[0]) {
			rBuf = rec.Descr;
			ok = 1;
		}
		else {
			if(Filt.Order == PrjTaskFilt::ordByClient) {
				if(rec.EmployerID) {
					GetPersonName(rec.EmployerID, rBuf);
					ok = 1;
				}
			}
			else {
				if(rec.ClientID) {
					GetPersonName(rec.ClientID, rBuf);
					ok = 1;
				}
			}
			if(ok < 0) {
				if(strip(rec.Memo)[0]) {
					rBuf = rec.Memo;
					ok = 1;
				}
			}
		}
	}
	return ok;
}

int SLAPI PPViewPrjTask::EditTimeGridItem(PPID * pID, PPID rowID, const LDATETIME & rDtm)
{
	int    ok = -1;
	PPIDArray id_list;
	THROW(TodoObj.CheckRightsModByID(pID));
	if(*pID) {
		//
		// Редактировать элемент
		//
		if(TodoObj.Edit(pID, 0) == cmOK) {
			id_list.add(*pID);
			UpdateTempTable(&id_list, 1);
			ok = 1;
		}
	}
	else if(rDtm.d) {
		//
		// Создать элемент
		//
		PrjTaskTbl::Rec rec;
		PPID   cli_id = (Filt.Order == PrjTaskFilt::ordByClient) ? rowID : 0;
		PPID   emp_id = (Filt.Order == PrjTaskFilt::ordByClient) ? 0 : rowID;
		THROW(TodoObj.InitPacket(&rec, TODOKIND_TASK, Filt.ProjectID, cli_id, emp_id, 1));
		rec.StartDt = rDtm.d;
		rec.StartTm = rDtm.t;
		while(ok < 0 && TodoObj.EditDialog(&rec) > 0)
			if(TodoObj.PutPacket(pID, &rec, 1)) {
				id_list.add(*pID);
				UpdateTempTable(&id_list, 1);
				ok = 1;
			}
			else
				PPError();
	}
	CATCHZOKPPERR
	return ok;
}

int PPViewPrjTask::PrjTaskTimeChunkGrid::Edit(int item, long rowID, const LDATETIME & rTm, long * pID)
{
	int    ok = -1;
	if(item == iChunk)
		ok = P_View->EditTimeGridItem(pID, rowID, rTm);
	return -1;
}

int PPViewPrjTask::PrjTaskTimeChunkGrid::MoveChunk(int mode, long id, long rowId, const STimeChunk & rNewChunk)
{
	return -1;
}

int SLAPI PPViewPrjTask::UpdateTimeBrowser(int destroy)
{
	return PPView::UpdateTimeBrowser(&Grid, 0, destroy);
}

int SLAPI PPViewPrjTask::TimeChunkBrowser()
{
	UpdateTimeBrowser(1);
	STimeChunkBrowser * p_brw = new STimeChunkBrowser;
	STimeChunkBrowser::Param p;
	InitSTimeChunkBrowserParam("PPViewPrjTask", &p);
	p.Quant = 3600;
	p.PixQuant = 5;
	p.PixRow = 12;
	p.PixRowMargin = 4;
	p.HdrLevelHeight = 20;
	//p.DefBounds.Set(Filt.Period.Start.d, plusdate(Filt.Period.Finish.d, 30));
	if(Filt.EmployerID)
		p.ViewType = STimeChunkBrowser::Param::vHourDay;
	p_brw->SetBmpId(STimeChunkBrowser::bmpModeGantt, BM_TIMEGRAPH_GANTT);
	p_brw->SetBmpId(STimeChunkBrowser::bmpModeHourDay, BM_TIMEGRAPH_HOURDAY);
	p_brw->SetBmpId(STimeChunkBrowser::bmpBack, BM_BACK);
	p_brw->SetParam(&p);
	p_brw->SetData(&Grid, 0);
	p_brw->SetResID(((PPApp *)APPL)->LastCmd);
	InsertView(p_brw);
	return 1;
}
//
// Implementation of PPALDD_PrjTask
//
PPALDD_CONSTRUCTOR(PrjTask)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		Extra[0].Ptr = new PPObjPrjTask;
	}
}

PPALDD_DESTRUCTOR(PrjTask)
{
	Destroy();
	delete (PPObjPrjTask *)Extra[0].Ptr;
}

int PPALDD_PrjTask::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		PrjTaskTbl::Rec rec;
		if(rFilt.ID && ((PPObjPrjTask *)(Extra[0].Ptr))->Search(rFilt.ID, &rec) > 0) {
			H.ID          = rec.ID;
			H.Kind        = rec.Kind;
			H.ProjectID   = rec.ProjectID;
			H.CreatorID   = rec.CreatorID;
			H.GroupID     = rec.GroupID;
			H.EmployerID  = rec.EmployerID;
			H.ClientID    = rec.ClientID;
			H.DlvrAddrID  = rec.DlvrAddrID;
			H.BillArID    = rec.BillArID;
			H.TemplateID  = rec.TemplateID;
			H.LinkTaskID  = rec.LinkTaskID;
			H.Dt          = rec.Dt;
			H.Tm          = rec.Tm;
			H.StartDt     = rec.StartDt;
			H.StartTm     = rec.StartTm;
			H.EstFinishDt = rec.EstFinishDt;
			H.EstFinishTm = rec.EstFinishTm;
			H.FinishDt    = rec.FinishDt;
			H.FinishTm    = rec.FinishTm;
			H.Priority    = rec.Priority;
			H.Status      = rec.Status;
			H.Amount      = rec.Amount;
			H.DrPrd       = rec.DrPrd;
			H.DrKind      = rec.DrKind;
			H.DrDetail    = rec.DrDetail;
			H.Flags       = rec.Flags;

			SString temp_buf;
			STRNSCPY(H.PriorText, PPObjPrjTask::GetPriorText(rec.Priority, temp_buf));
			STRNSCPY(H.StatusText, PPObjPrjTask::GetStatusText(rec.Status, temp_buf));

			STRNSCPY(H.Code,  rec.Code);
			STRNSCPY(H.Descr, rec.Descr);
			STRNSCPY(H.Memo,  rec.Memo);
			ok = 1;
		}
		if(!DlRtm::InitData(rFilt, rsrv))
			ok = 0;
	}
	return ok;
}
//
// Implementation of PPALDD_PrjTaskView
//
PPALDD_CONSTRUCTOR(PrjTaskView)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignIterData(1, &I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(PrjTaskView)
{
	Destroy();
}

int PPALDD_PrjTaskView::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(PrjTask, rsrv);
	H.FltKind       = p_filt->Kind;
	H.FltProjectID  = p_filt->ProjectID;
	H.FltClientID   = p_filt->ClientID;
	H.FltEmployerID = p_filt->EmployerID;
	H.FltTemplateID = p_filt->TemplateID;
	H.FltCreatorID  = p_filt->CreatorID;

	H.FltPeriod_beg = p_filt->Period.low;
	H.FltPeriod_end = p_filt->Period.upp;
	H.FltStart_beg  = p_filt->StartPeriod.low;
	H.FltStart_end  = p_filt->StartPeriod.upp;
	H.FltEstFn_beg  = p_filt->EstFinishPeriod.low;
	H.FltEstFn_end  = p_filt->EstFinishPeriod.upp;
	H.FltFn_beg     = p_filt->FinishPeriod.low;
	H.FltFn_end     = p_filt->FinishPeriod.upp;
	H.FltFlags      = p_filt->Flags;

	SString temp_buf;
	STRNSCPY(H.FltPriorList, p_filt->GetPriorListText(temp_buf));
	STRNSCPY(H.FltStatusList, p_filt->GetStatusListText(temp_buf));

	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_PrjTaskView::InitIteration(PPIterID iterId, int sortId, long rsrv)
{
	INIT_PPVIEW_ALDD_ITER(PrjTask);
}

int PPALDD_PrjTaskView::NextIteration(PPIterID iterId, long rsrv)
{
	START_PPVIEW_ALDD_ITER(PrjTask);
	I.TodoID  = item.ID;
	I.TemplID = item.TemplateID;
	I.LinkID  = item.LinkTaskID;
	FINISH_PPVIEW_ALDD_ITER();
}

int PPALDD_PrjTaskView::Destroy()
{
	DESTROY_PPVIEW_ALDD(PrjTask);
}
//
// Implementation of PPALDD_PrjTaskViewCt
//
PPALDD_CONSTRUCTOR(PrjTaskViewCt)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignIterData(1, &I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(PrjTaskViewCt)
{
	Destroy();
}

int PPALDD_PrjTaskViewCt::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(PrjTask, rsrv);
	H.FltKind       = p_filt->Kind;
	H.FltProjectID  = p_filt->ProjectID;
	H.FltClientID   = p_filt->ClientID;
	H.FltEmployerID = p_filt->EmployerID;
	H.FltTemplateID = p_filt->TemplateID;
	H.FltCreatorID  = p_filt->CreatorID;

	H.FltPeriod_beg = p_filt->Period.low;
	H.FltPeriod_end = p_filt->Period.upp;
	H.FltStart_beg  = p_filt->StartPeriod.low;
	H.FltStart_end  = p_filt->StartPeriod.upp;
	H.FltEstFn_beg  = p_filt->EstFinishPeriod.low;
	H.FltEstFn_end  = p_filt->EstFinishPeriod.upp;
	H.FltFn_beg     = p_filt->FinishPeriod.low;
	H.FltFn_end     = p_filt->FinishPeriod.upp;
	H.FltFlags      = p_filt->Flags;

	SString temp_buf, idx_fld, tab_fld;
	STRNSCPY(H.FltPriorList, p_filt->GetPriorListText(temp_buf));
	STRNSCPY(H.FltStatusList, p_filt->GetStatusListText(temp_buf));
	if(p_filt->TabType == PrjTaskFilt::crstDateHour) {
		PPLoadString("date", idx_fld);
		idx_fld.Transf(CTRANSF_INNER_TO_OUTER);
		PPGetWord(PPWORD_HOUR, 1, tab_fld);
	}
	else if(oneof2(p_filt->TabType, PrjTaskFilt::crstClientDate, PrjTaskFilt::crstEmployerDate)) {
		// @v9.2.6 PPGetWord((p_filt->TabType == PrjTaskFilt::crstEmployerDate) ? PPWORD_EXECUTOR : PPWORD_CLIENT, 1, idx_fld);
		PPLoadString(((p_filt->TabType == PrjTaskFilt::crstEmployerDate) ? "executor" : "client"), idx_fld); // @v9.2.6
		idx_fld.Transf(CTRANSF_INNER_TO_OUTER); // @v9.2.6
		PPLoadString("date", tab_fld);
		tab_fld.Transf(CTRANSF_INNER_TO_OUTER);
	}
	else if(p_filt->TabType == PrjTaskFilt::crstClientEmployer) {
		// @v9.2.6 PPGetWord(PPWORD_CLIENT,   1, idx_fld);
		PPLoadString("client", idx_fld); // @v9.2.6
		idx_fld.Transf(CTRANSF_INNER_TO_OUTER); // @v9.2.6
		// @v9.2.6 PPGetWord(PPWORD_EXECUTOR, 1, tab_fld);
		PPLoadString("executor", tab_fld); // @v9.2.6
		tab_fld.Transf(CTRANSF_INNER_TO_OUTER);
	}
	idx_fld.Transf(CTRANSF_OUTER_TO_INNER).CopyTo(H.IdxFld, sizeof(H.IdxFld));
	tab_fld.Transf(CTRANSF_OUTER_TO_INNER).CopyTo(H.TabFld, sizeof(H.TabFld));
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_PrjTaskViewCt::InitIteration(PPIterID iterId, int sortId, long rsrv)
{
	INIT_PPVIEW_ALDD_ITER(PrjTask);
}

int PPALDD_PrjTaskViewCt::NextIteration(PPIterID iterId, long rsrv)
{
	START_PPVIEW_ALDD_ITER(PrjTask);
	const PrjTaskFilt * p_filt = (const PrjTaskFilt *)p_v->GetBaseFilt();
	SString idx_name, tab_name;
	if(p_filt->TabType == PrjTaskFilt::crstDateHour) {
		idx_name.Cat(item.StartDt);
		tab_name.Printf("%02ld:00", item.ID);
	}
	else if(oneof2(p_filt->TabType, PrjTaskFilt::crstClientDate, PrjTaskFilt::crstEmployerDate)) {
		LDATE dt = ZERODATE;
		GetPersonName((p_filt->TabType == PrjTaskFilt::crstEmployerDate) ? item.EmployerID : item.ClientID, idx_name);
		dt.v = item.ID;
		tab_name.Cat(dt);
	}
	else if(p_filt->TabType == PrjTaskFilt::crstClientEmployer) {
		GetPersonName(item.ClientID, idx_name);
		GetPersonName(item.ID, tab_name);
	}
	idx_name.CopyTo(I.IdxName, sizeof(I.IdxName));
	tab_name.CopyTo(I.TabName, sizeof(I.TabName));
	I.Value = item.Dt.v;
	FINISH_PPVIEW_ALDD_ITER();
}

int PPALDD_PrjTaskViewCt::Destroy()
{
	DESTROY_PPVIEW_ALDD(PrjTask);
}
