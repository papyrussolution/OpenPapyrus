// PPNOTES.CPP
// Copyright (c) A.Sobolev 2026
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop

PPNotesPacket::PPNotesPacket() : PPExtStrContainer()
{
}
	
PPNotesPacket & PPNotesPacket::Z()
{
	PPExtStrContainer::Z();
	Rec.Clear();
	TagL.Z();
	return *this;
}

TLP_IMPL(PPObjNotes, NotesTbl, P_Tbl);
	
PPObjNotes::PPObjNotes(void * extraPtr/*=0*/) : PPObject(PPOBJ_NOTES)
{
	TLP_OPEN(P_Tbl);
	ImplementFlags |= (implStrAssocMakeList|implTreeSelector);
}

PPObjNotes::~PPObjNotes()
{
	TLP_CLOSE(P_Tbl);
}

int PPObjNotes::SerializePacket(int dir, PPNotesPacket * pPack, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW(pPack->PPExtStrContainer::SerializeB(dir, rBuf, pSCtx));
	THROW_SL(P_Tbl->SerializeRecord(dir, &pPack->Rec, rBuf, pSCtx));
	THROW(pPack->TagL.Serialize(dir, rBuf, pSCtx));
	CATCHZOK
	return ok;
}

int PPObjNotes::IsPacketEq(const PPNotesPacket & rS1, const PPNotesPacket & rS2, long flags)
{
#define CMP_MEMB(m)  if(rS1.Rec.m != rS2.Rec.m) return 0;
	CMP_MEMB(ID);
	CMP_MEMB(UedCrTm);
	CMP_MEMB(ParentID);
	CMP_MEMB(LinkObjType);
	CMP_MEMB(LinkObjID);
	CMP_MEMB(Flags);
	CMP_MEMB(Status);
	CMP_MEMB(CategoryID);
#undef CMP_MEMB
	if(!rS1.TagL.IsEq(rS2.TagL))
		return 0;
	else {
		SString t1, t2;
        rS1.GetExtStrData(PPNotesPacket::extssTitle, t1);
        rS2.GetExtStrData(PPNotesPacket::extssTitle, t2);
        if(t1 != t2)
			return 0;
		else {
			rS1.GetExtStrData(PPNotesPacket::extssText, t1);
			rS2.GetExtStrData(PPNotesPacket::extssText, t2);
			if(t1 != t2)
				return 0;
		}
	}
	return 1;
}

int PPObjNotes::GetName(PPID id, SString & rName) // @construction
{
	rName.Z();
	PPRef->UtrC.SearchUtf8(TextRefIdent(Obj, id, PPTRPROP_NAME), rName);
	rName.Transf(CTRANSF_UTF8_TO_INNER);
	return rName.NotEmpty();
}

int PPObjNotes::GetText(PPID id, SString & rText) // @construction
{
	rText.Z();
	PPRef->UtrC.SearchUtf8(TextRefIdent(Obj, id, PPTRPROP_MEMO), rText);
	return rText.NotEmpty();
}

int PPObjNotes::GetPacket(PPID id, PPNotesPacket * pPack)
{
	pPack->Z();
	int    ok = 1;
	Reference * p_ref(PPRef);
	if(PPCheckGetObjPacketID(Obj, id)) {
		if(Search(id, &pPack->Rec) > 0) {
			SString temp_buf;
			THROW(p_ref->Ot.GetList(Obj, id, &pPack->TagL));
			//
			p_ref->UtrC.SearchUtf8(TextRefIdent(Obj, id, PPTRPROP_NAME), temp_buf);
			pPack->PutExtStrData(PPNotesPacket::extssTitle, temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
			p_ref->UtrC.SearchUtf8(TextRefIdent(Obj, id, PPTRPROP_MEMO), temp_buf);
			pPack->PutExtStrData(PPNotesPacket::extssText, temp_buf); // основной текст хранится в utf8
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int PPObjNotes::PutExtText(PPID id, const PPNotesPacket * pPack, int use_ta)
{
	int    ok = 1;
	SString ext_buffer;
	Reference * p_ref(PPRef);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(pPack) {
			(ext_buffer = pPack->GetBuffer()).Strip();
			pPack->GetExtStrData(PPNotesPacket::extssTitle, ext_buffer);
			THROW(p_ref->UtrC.SetTextUtf8(TextRefIdent(Obj, id, PPTRPROP_NAME), ext_buffer.Transf(CTRANSF_INNER_TO_UTF8), 0));
			pPack->GetExtStrData(PPNotesPacket::extssText, ext_buffer);
			THROW(p_ref->UtrC.SetTextUtf8(TextRefIdent(Obj, id, PPTRPROP_MEMO), ext_buffer/*.Transf(CTRANSF_INNER_TO_UTF8)*/, 0)); // основной текст хранится в utf8
		}
		else {
			THROW(p_ref->UtrC.SetTextUtf8(TextRefIdent(Obj, id, PPTRPROP_NAME), ext_buffer.Z(), 0));
			THROW(p_ref->UtrC.SetTextUtf8(TextRefIdent(Obj, id, PPTRPROP_MEMO), ext_buffer.Z(), 0));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjNotes::PutPacket(PPID * pID, PPNotesPacket * pPack, int use_ta)
{
	PPTRPROP_USER; // @debug
	int    ok = 1;
	int    ta = 0;
	PPID   log_action_id = 0;
	SString ext_buffer;
	Reference * p_ref(PPRef);
	PPNotesPacket org_pack;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID) {
			THROW(GetPacket(*pID, &org_pack) > 0);
			if(pPack == 0) {
				//
				// Удаление пакета
				//
				THROW(p_ref->RemoveProperty(Obj, *pID, 0, 0));
				THROW(p_ref->Ot.PutList(Obj, *pID, 0, 0));
				THROW(RemoveSync(*pID));
				THROW(RemoveByID(P_Tbl, *pID, 0));
				THROW(PutExtText(*pID, 0, 0));
				//log_action_id = PPACN_OBJRMV;
				Dirty(*pID);
			}
			else {
				//
				// Изменение пакета
				//
				if(IsPacketEq(org_pack, *pPack, 0)) {
					ok = -1;
				}
				else {
					THROW(UpdateByID(P_Tbl, Obj, *pID, &pPack->Rec, 0));
					THROW(p_ref->Ot.PutList(Obj, *pID, &pPack->TagL, 0));
					THROW(PutExtText(*pID, pPack, 0));
					log_action_id = PPACN_OBJUPD;
					Dirty(*pID);
				}
			}
		}
		else if(pPack) {
			//
			// Создание пакета
			//
			THROW(AddObjRecByID(P_Tbl, Obj, pID, &pPack->Rec, 0));
			pPack->Rec.ID = *pID;
			THROW(p_ref->Ot.PutList(Obj, *pID, &pPack->TagL, 0));
			THROW(PutExtText(*pID, pPack, 0));
			log_action_id = PPACN_OBJADD;
		}
		DS.LogAction(log_action_id, Obj, *pID, 0, 0);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}
	
/*virtual*/int PPObjNotes::Search(PPID id, void * pRec/*=0*/)
{
	return SearchByID(P_Tbl, Obj, id, pRec);
}

/*virtual*/int PPObjNotes::Browse(void * extraPtr)
{
	int    ok = -1;
	return ok;
}
	
/*virtual*/int PPObjNotes::Edit(PPID * pID, void * extraPtr)
{
	int    ok = 0;
	return ok;
}

/*virtual*/int PPObjNotes::DeleteObj(PPID id)
{
	int    ok = 0;
	return ok;
}

int PPObjNotes::AddItemToSelectorList(const NotesTbl::Rec & rRec, AislBlock & rBlk) // @recursion
{
	int    ok = -1;
	if(!rBlk.El.Has(rRec.ID)) {
		PPID   parent_id = rRec.ParentID;
		SString title_buf;
		if(parent_id) {
			NotesTbl::Rec parent_rec;
			if(Search(parent_id, &parent_rec) > 0) {
				if(rBlk.Stack.lsearch(parent_id)) {
					SString added_buf;
					//(added_buf = rRec.Name).CatChar('-').CatChar('>');
					//added_buf.Cat(parent_rec.Name);
					// @todo title_buf
					PPSetError(PPERR_CYCLEWORLDOBJ, added_buf); // @todo PPERR_CYCLEWORLDOBJ-->PPERR_CYCLENOTESOBJ
					PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR_TIME_USER);
					parent_id = 0;
				}
				else {
					rBlk.Stack.add(parent_id);
					THROW(AddItemToSelectorList(parent_rec, rBlk)); // @recursion
				}
			}
			else
				parent_id = 0;
		}
		{
			if(GetName(rRec.ID, title_buf) <= 0) {
				ideqvalstr(rRec.ID, title_buf);
			}
			THROW_SL(rBlk.P_List->AddFast(rRec.ID, parent_id, title_buf));
			rBlk.El.Add(rRec.ID);
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}
	
/*virtual*/StrAssocArray * PPObjNotes::MakeStrAssocList(void * extraPtr)
{
	//SelFilt sf;
	//ConvertExtraParam(extraPtr, &sf);
	StrAssocArray * p_list = new StrAssocArray;
	NotesTbl::Key1 k1;
	BExtQuery q(P_Tbl, 1);
	DBQ * dbq = 0;
	THROW_MEM(p_list);
	MEMSZERO(k1);
	q.select(P_Tbl->ID, P_Tbl->ParentID, 0L);
	//dbq = ppcheckfiltid(dbq, P_Tbl->ParentID, sf.ParentID);
	q.where(*dbq);
	MEMSZERO(k1);
	{
		NotesTbl::Rec rec;
		AislBlock blk;
		blk.P_List = p_list;
		blk.UseHierarchy = 1;//BIN(sf.ParentID == 0 && sf.CountryID == 0);
		for(q.initIteration(false, &k1, spGt); q.nextIteration() > 0;) {
			blk.Stack.clear();
			P_Tbl->CopyBufTo(&rec);
			THROW(AddItemToSelectorList(rec, blk));
		}
	}
	p_list->SortByText();
	CATCH
		ZDELETE(p_list);
	ENDCATCH
	return p_list;
}


/*virtual*/const char * PPObjNotes::GetNamePtr()
{
	const char * p_result = "";
	return p_result;
}
	
/*virtual*/int  PPObjNotes::Read(PPObjPack *, PPID, void * stream, ObjTransmContext *)
{
	int    ok = 0;
	return ok;
}
	
/*virtual*/int  PPObjNotes::Write(PPObjPack *, PPID *, void * stream, ObjTransmContext *)
{
	int    ok = 0;
	return ok;
}

/*virtual*/int  PPObjNotes::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	int    ok = -1;
	return ok;
}

/*virtual*/int  PPObjNotes::HandleMsg(int, PPID, PPID, void * extraPtr)
{
	int    ok = -1;
	return ok;
}
