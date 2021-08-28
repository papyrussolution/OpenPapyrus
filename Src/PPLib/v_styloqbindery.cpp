// V_STYLOQBINDERY.CPP
// Copyright (c) A.Sobolev 2021
//
#include <pp.h>
#pragma hdrstop

IMPLEMENT_PPFILT_FACTORY(StyloQBindery); StyloQBinderyFilt::StyloQBinderyFilt() : PPBaseFilt(PPFILT_STYLOQBINDERY, 0, 0)
{
	SetFlatChunk(offsetof(StyloQBinderyFilt, ReserveStart),
		offsetof(StyloQBinderyFilt, Reserve)-offsetof(StyloQBinderyFilt, ReserveStart)+sizeof(Reserve));
	Init(1, 0);
}

PPViewStyloQBindery::PPViewStyloQBindery() : PPView(0, &Filt, PPVIEW_STYLOQBINDERY, (implBrowseArray|implDontEditNullFilter), 0), P_DsList(0)
{
}

PPViewStyloQBindery::~PPViewStyloQBindery()
{
	ZDELETE(P_DsList);
}

/*virtual*/int PPViewStyloQBindery::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	BExtQuery::ZDelete(&P_IterQuery);
	Counter.Init();
	CATCHZOK
	return ok;
}

/*virtual*/int PPViewStyloQBindery::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	return -1;
}

int PPViewStyloQBindery::MakeList(PPViewBrowser * pBrw)
{
	int    ok = 1;
	if(P_DsList)
		P_DsList->clear();
	else {
		THROW_SL(P_DsList = new SArray(sizeof(BrwItem)));
	}
	StrPool.ClearS();
	{
		SString temp_buf;
		StyloQCore::StoragePacket pack;
		SBinaryChunk face_chunk;
		StyloQFace face_pack;
		StyloQSecTbl::Key0 k0;
		MEMSZERO(k0);
		StyloQCore * p_t = Obj.P_Tbl;
		if(p_t->search(0, &k0, spFirst)) do {
			if(p_t->ReadCurrentPacket(&pack)) {
				BrwItem new_entry;
				MEMSZERO(new_entry);
				new_entry.ID = pack.Rec.ID;
				new_entry.Kind = pack.Rec.Kind;
				assert(sizeof(new_entry.BI) == sizeof(pack.Rec.BI));
				memcpy(new_entry.BI, pack.Rec.BI, sizeof(new_entry.BI));
				new_entry.CorrespondID = pack.Rec.CorrespondID;
				new_entry.Expiration = pack.Rec.Expiration;
				new_entry.LinkOid.Set(pack.Rec.LinkObjType, pack.Rec.LinkObjID);
				if(new_entry.LinkOid.Obj && new_entry.LinkOid.Id) {
					char   name_buf[256];
					PPObject * ppobj = ObjColl.GetObjectPtr(new_entry.LinkOid.Obj);
					if(ppobj && ppobj->GetName(new_entry.LinkOid.Id, name_buf, sizeof(name_buf)) > 0) {					
						temp_buf = name_buf;
						StrPool.AddS(temp_buf, &new_entry.ObjNameP);
					}
				}
				{
					uint32  face_tag_id = 0;
					if(oneof2(pack.Rec.Kind, StyloQCore::kClient, StyloQCore::kForeignService))
						face_tag_id = SSecretTagPool::tagFace;
					else 
						face_tag_id = SSecretTagPool::tagSelfyFace;
					face_chunk.Z();
					if(face_tag_id && pack.Pool.Get(face_tag_id, &face_chunk) > 0) {
						temp_buf.Z().CatN(static_cast<const char *>(face_chunk.PtrC()), face_chunk.Len());
						if(face_pack.FromJson(temp_buf) && face_pack.GetRepresentation(0, temp_buf)) {
							assert(temp_buf.NotEmpty()); // face_pack.GetRepresentation() != 0 garantees this assertion
							temp_buf.Transf(CTRANSF_UTF8_TO_INNER); // в базе данных лик хранится в utf-8
							StrPool.AddS(temp_buf, &new_entry.FaceP);
						}
					}
				}
				THROW_SL(P_DsList->insert(&new_entry));
			}
		} while(p_t->search(&k0, spNext));
	}
	CATCHZOK
	return ok;
}

/*virtual*/SArray * PPViewStyloQBindery::CreateBrowserArray(uint * pBrwId, SString * pSubTitle)
{
	uint   brw_id = BROWSER_STYLOQBINDERY;
	SArray * p_array = 0;
	THROW(MakeList(0));
	p_array = new SArray(*P_DsList);
	CATCH
		ZDELETE(P_DsList);
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return p_array;
}

int PPViewStyloQBindery::_GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	int    ok = 1;
	if(pBlk->P_SrcData && pBlk->P_DestData) {
		ok = 1;
		const BrwItem * p_item = static_cast<const BrwItem *>(pBlk->P_SrcData);
		int    r = 0;
		switch(pBlk->ColumnN) {
			case 0: pBlk->Set(p_item->ID); break; // @id
			case 1: // kind
				//pBlk->Set(p_item->Kind);
				{
					pBlk->TempBuf.Z();
					const char * p_sign = 0;
					switch(p_item->Kind) {
						case StyloQCore::kNativeService: p_sign = "styloq_binderykind_nativeservice"; break;
						case StyloQCore::kForeignService: p_sign = "styloq_binderykind_foreignservice"; break;
						case StyloQCore::kClient: p_sign = "styloq_binderykind_client"; break;
						case StyloQCore::kSession: p_sign = "styloq_binderykind_session"; break;
						case StyloQCore::kFace: p_sign = "styloq_binderykind_face"; break;
					}
					pBlk->TempBuf.CatChar('(').Cat(p_item->Kind).CatChar(')');
					if(p_sign) {
						SString & r_temp_buf = SLS.AcquireRvlStr();
						PPLoadString(p_sign, r_temp_buf);
						if(r_temp_buf.Len()) {
							pBlk->TempBuf.Space().Cat(r_temp_buf);
						}
					}
					pBlk->Set(pBlk->TempBuf);
				}
				break;
			case 2: // stylo-q ident
				pBlk->TempBuf.Z().EncodeMime64(p_item->BI, sizeof(p_item->BI));
				pBlk->Set(pBlk->TempBuf);
				break;
			case 3: // expiry time
				pBlk->Set(p_item->Expiration);
				break;
			case 4: // common name
				pBlk->TempBuf.Z();
				pBlk->Set(pBlk->TempBuf);
				break;
			case 5: // correspond item
				pBlk->Set(p_item->CorrespondID);
				break;
			case 6: // link obj type
				pBlk->TempBuf.Z();
				if(p_item->LinkOid.Obj) {
					GetObjectTitle(p_item->LinkOid.Obj, pBlk->TempBuf);
				}
				pBlk->Set(pBlk->TempBuf);
				break;
			case 7: // link obj name
				pBlk->TempBuf.Z();
				StrPool.GetS(p_item->ObjNameP, pBlk->TempBuf);
				pBlk->Set(pBlk->TempBuf);
				break;
			case 8: // face
				pBlk->TempBuf.Z();
				StrPool.GetS(p_item->FaceP, pBlk->TempBuf);
				pBlk->Set(pBlk->TempBuf);
				break;
		}
	}
	return ok;
}

//static
int FASTCALL PPViewStyloQBindery::GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	PPViewStyloQBindery * p_v = static_cast<PPViewStyloQBindery *>(pBlk->ExtraPtr);
	return p_v ? p_v->_GetDataForBrowser(pBlk) : 0;
}

static int CellStyleFunc(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pStyle, void * extraPtr)
{
	int    ok = -1;
	PPViewBrowser * p_brw = static_cast<PPViewBrowser *>(extraPtr);
	if(p_brw) {
		PPViewStyloQBindery * p_view = static_cast<PPViewStyloQBindery *>(p_brw->P_View);
		ok = p_view ? p_view->CellStyleFunc_(pData, col, paintAction, pStyle, p_brw) : -1;
	}
	return ok;
}

int PPViewStyloQBindery::CellStyleFunc_(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pCellStyle, PPViewBrowser * pBrw)
{
	int    ok = -1;
	if(pBrw && pData && pCellStyle && col >= 0) {
		const BrowserDef * p_def = pBrw->getDef();
		if(col < static_cast<long>(p_def->getCount())) {
			/*
			const BroColumn & r_col = p_def->at(col);
			if(col == 0) { // id
				const long cfg_flags = static_cast<const BrwItem *>(pData)->CfgFlags;
				if(cfg_flags & PPObjTimeSeries::Config::efDisableStake)
					ok = pCellStyle->SetLeftTopCornerColor(GetColorRef(SClrGrey));
				if(cfg_flags & PPObjTimeSeries::Config::efLong && cfg_flags & PPObjTimeSeries::Config::efShort)
					ok = pCellStyle->SetRightFigCircleColor(GetColorRef(SClrOrange));
				else if(cfg_flags & PPObjTimeSeries::Config::efLong)
					ok = pCellStyle->SetRightFigCircleColor(GetColorRef(SClrGreen));
				else if(cfg_flags & PPObjTimeSeries::Config::efShort)
					ok = pCellStyle->SetRightFigCircleColor(GetColorRef(SClrRed));
			}
			else if(col == 2) { // name
				const int16 type = static_cast<const BrwItem *>(pData)->Type;
				if(type == PPTimeSeries::tForex)
					ok = pCellStyle->SetRightFigCircleColor(GetColorRef(SClrPink));
				else if(type == PPTimeSeries::tStocks)
					ok = pCellStyle->SetRightFigCircleColor(GetColorRef(SClrLightblue));
				else if(type == PPTimeSeries::tCrypto) // @v10.8.7
					ok = pCellStyle->SetRightFigCircleColor(GetColorRef(SClrLightgreen));
			}
			*/
		}
	}
	return ok;
}
	
/*virtual*/void PPViewStyloQBindery::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw) {
		pBrw->SetDefUserProc(PPViewStyloQBindery::GetDataForBrowser, this);
		pBrw->SetCellStyleFunc(CellStyleFunc, pBrw);
		//pBrw->Helper_SetAllColumnsSortable();
	}
}
	
/*virtual*/int PPViewStyloQBindery::OnExecBrowser(PPViewBrowser *)
{
	return -1;
}
	
/*virtual*/int PPViewStyloQBindery::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = -2;
	PPID   id = pHdr ? *static_cast<const PPID *>(pHdr) : 0;
	const  PPID preserve_id = id;
	ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		switch(ppvCmd) {
			case PPVCMD_MATCH:
				ok = -1;
				if(Obj.AssignObjToClientEntry(id) > 0) {
					ok = 1;
				}
				break;
			case PPVCMD_FACE:
				ok = -1;
				if(Obj.EditFace(id) > 0) {
					ok = 1;
				}
				break;
			case PPVCMD_USERSORT: ok = 1; break; // The rest will be done below
			case PPVCMD_DELETEALL:
				// @todo ok = DeleteAll();
				break;
			case PPVCMD_REFRESH:
				ok = 1;
				break;
		}
	}
	if(ok > 0) {
		MakeList(pBrw);
		if(pBrw) {
			AryBrowserDef * p_def = static_cast<AryBrowserDef *>(pBrw->getDef());
			if(p_def) {
				SArray * p_array = new SArray(*P_DsList);
				p_def->setArray(p_array, 0, 1);
				pBrw->setRange(p_array->getCount());
				uint   temp_pos = 0;
				long   update_pos = -1;
				if(preserve_id > 0 && P_DsList->lsearch(&preserve_id, &temp_pos, CMPF_LONG))
					update_pos = temp_pos;
				if(update_pos >= 0)
					pBrw->go(update_pos);
				else if(update_pos == MAXLONG)
					pBrw->go(p_array->getCount()-1);
			}
			pBrw->Update();
		}
	}
	return ok;
}

/*virtual*/int PPViewStyloQBindery::Detail(const void *, PPViewBrowser * pBrw)
{
	return -1;
}
//
//
//
IMPLEMENT_PPFILT_FACTORY(StyloQCommand); StyloQCommandFilt::StyloQCommandFilt() : PPBaseFilt(PPFILT_STYLOQCOMMAND, 0, 0)
{
	SetFlatChunk(offsetof(StyloQCommandFilt, ReserveStart),
		offsetof(StyloQCommandFilt, Reserve)-offsetof(StyloQCommandFilt, ReserveStart)+sizeof(Reserve));
	Init(1, 0);
}

PPViewStyloQCommand::PPViewStyloQCommand() : PPView(0, &Filt, PPVIEW_STYLOQCOMMAND, (implBrowseArray|implDontEditNullFilter), 0), P_DsList(0)
{
}

PPViewStyloQCommand::~PPViewStyloQCommand()
{
}

static int GetStyloQCommandFileName(SString & rFileName)
{
	rFileName.Z();
	int    ok = 1;
	PPGetFilePath(PPPATH_WORKSPACE, "styloqcommands", rFileName);
	if(::IsDirectory(rFileName) || ::createDir(rFileName)) {
		rFileName.SetLastSlash().Cat("stqc").Dot().Cat("xml");
	}
	else
		ok = 0;
	return ok;
}

int PPViewStyloQCommand::MakeList(PPViewBrowser * pBrw)
{
	int    ok = 1;
	if(P_DsList)
		P_DsList->clear();
	else {
		THROW_SL(P_DsList = new SArray(sizeof(uint)));
	}
	for(uint i = 0; i < List.GetCount(); i++) {
		uint idx = (i+1);
		P_DsList->insert(&idx);
	}
	CATCHZOK
	return ok;
}

/*virtual*/int PPViewStyloQCommand::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	SString file_name;
	CALLPTRMEMB(P_DsList, freeAll());
	if(GetStyloQCommandFileName(file_name)) {
		List.Load(file_name);
	}
	THROW(MakeList(0));
	CATCHZOK
	return ok;
}

/*virtual*/int PPViewStyloQCommand::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	int    ok = -1;
	return ok;
}

/*virtual*/SArray * PPViewStyloQCommand::CreateBrowserArray(uint * pBrwId, SString * pSubTitle)
{
	uint   brw_id = BROWSER_STYLOQCOMMANDS;
	SArray * p_array = 0;
	THROW(MakeList(0));
	p_array = new SArray(*P_DsList);
	CATCH
		ZDELETE(P_DsList);
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return p_array;
}

/*virtual*/void PPViewStyloQCommand::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw) {
		pBrw->SetDefUserProc(PPViewStyloQCommand::GetDataForBrowser, this);
		pBrw->SetCellStyleFunc(CellStyleFunc, pBrw);
		//pBrw->Helper_SetAllColumnsSortable();
	}
}

int PPViewStyloQCommand::_GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	int    ok = 1;
	if(pBlk->P_SrcData && pBlk->P_DestData) {
		const  uint idx = *static_cast<const uint *>(pBlk->P_SrcData);
		int    r = 0;
		const StyloQCommandList::Item * p_item = idx ? List.GetC(idx-1) : 0;
		if(p_item) {
			switch(pBlk->ColumnN) {
				case 0: // uuid
					pBlk->Set(pBlk->TempBuf.Z().Cat(p_item->Uuid));
					break;
				case 1: // name
					pBlk->Set((pBlk->TempBuf = p_item->Name).Transf(CTRANSF_UTF8_TO_INNER));
					break;
				case 2: // base command
					pBlk->Set(StyloQCommandList::GetBaseCommandName(p_item->BaseCmdId, pBlk->TempBuf));
					break;
				case 3: // DbSymb
					pBlk->Set(p_item->DbSymb);
					break;
				case 4: // ObjTypeRestriction
					pBlk->TempBuf.Z();
					if(p_item->ObjTypeRestriction) {
						GetObjectTitle(p_item->ObjTypeRestriction, pBlk->TempBuf);
					}
					pBlk->Set(pBlk->TempBuf);
					break;
				case 5: // ObjGroupRestriction
					pBlk->TempBuf.Z();
					if(p_item->ObjGroupRestriction) {
						if(p_item->ObjTypeRestriction == PPOBJ_PERSON) {
							GetObjectName(PPOBJ_PERSONKIND, p_item->ObjGroupRestriction, pBlk->TempBuf);
						}
					}
					pBlk->Set(pBlk->TempBuf);
					break;
				case 6: // Description
					pBlk->Set((pBlk->TempBuf = p_item->Description).Transf(CTRANSF_UTF8_TO_INNER));
					break;
			}
		}
	}
	return ok;
}

//static
int FASTCALL PPViewStyloQCommand::GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	PPViewStyloQCommand * p_v = static_cast<PPViewStyloQCommand *>(pBlk->ExtraPtr);
	return p_v ? p_v->_GetDataForBrowser(pBlk) : 0;
}

/*virtual*/int PPViewStyloQCommand::OnExecBrowser(PPViewBrowser *)
{
	return -1;
}

int PPViewStyloQCommand::EditStyloQCommand(StyloQCommandList::Item * pData)
{
	class StyloQCommandDialog : public TDialog {
		DECL_DIALOG_DATA(StyloQCommandList::Item);
		PPNamedFiltMngr * P_NfMgr;
		StrAssocArray CmdSymbList; // Список ассоциаций для обьектов PPView {id, символ}
		StrAssocArray CmdTextList; // Список ассоциаций для обьектов PPView {id, описание} (упорядоченный по возрастанию)
		PPObjPersonEvent PsnEvObj;
	public:
		StyloQCommandDialog(PPNamedFiltMngr * pNfMgr) : TDialog(DLG_STQCMD), P_NfMgr(pNfMgr)
		{
			CALLPTRMEMB(P_NfMgr, GetResourceLists(&CmdSymbList, &CmdTextList));
			SetupStrAssocCombo(this, CTLSEL_STQCMD_VCMD, &CmdTextList, 0, 0);
		}
		DECL_DIALOG_SETDTS()
		{
			int    ok = 1;
			SString temp_buf;
			RVALUEPTR(Data, pData);
			setCtrlString(CTL_STQCMD_NAME, (temp_buf = Data.Name).Transf(CTRANSF_UTF8_TO_INNER));
			setCtrlString(CTL_STQCMD_DESCR, (temp_buf = Data.Description).Transf(CTRANSF_UTF8_TO_INNER));
			setCtrlString(CTL_STQCMD_UUID, temp_buf.Z().Cat(Data.Uuid));
			{
				StrAssocArray basecmd_list;
				basecmd_list.Add(StyloQCommandList::Item::sqbcPersonEvent, StyloQCommandList::GetBaseCommandName(StyloQCommandList::Item::sqbcPersonEvent, temp_buf));
				basecmd_list.Add(StyloQCommandList::Item::sqbcReport, StyloQCommandList::GetBaseCommandName(StyloQCommandList::Item::sqbcReport, temp_buf));
				SetupStrAssocCombo(this, CTLSEL_STQCMD_BASECMD, &basecmd_list, Data.BaseCmdId, 0, 0, 0);
			}
			{
				PPIDArray obj_type_list;
				SetupObjListCombo(this, CTLSEL_STQCMD_OTR, Data.ObjTypeRestriction, &StyloQCore::MakeLinkObjTypeList(obj_type_list));
				SetupObjGroupCombo(Data.ObjTypeRestriction);
			}
			SetupBaseCommand(Data.BaseCmdId);
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			SString temp_buf;
			getCtrlString(CTL_STQCMD_NAME, temp_buf.Z());
			Data.Name = temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
			getCtrlString(CTL_STQCMD_DESCR, temp_buf.Z());
			Data.Description = temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
			getCtrlData(CTLSEL_STQCMD_BASECMD, &Data.BaseCmdId);
			getCtrlData(CTLSEL_STQCMD_OTR, &Data.ObjTypeRestriction);
			getCtrlData(CTLSEL_STQCMD_OGR, &Data.ObjGroupRestriction);
			Data.ViewSymb.Z();
			if(Data.BaseCmdId == Data.sqbcReport) {
				long view_id = getCtrlLong(CTLSEL_STQCMD_VCMD);
				uint pos = 0;
				if(CmdSymbList.Search(view_id, &pos))
					Data.ViewSymb = CmdSymbList.at_WithoutParent(pos).Txt;
			}
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmCmdParam)) {
				if(Data.BaseCmdId == Data.sqbcPersonEvent) {
					ChangePersonEventTemplate();
				}
				else if(Data.BaseCmdId == Data.sqbcReport) {
					ChangeBaseFilter();
				}
				else
					return;
			}
			else if(event.isCmd(cmOutFields)) {
				PPNamedFilt::EditRestrictedViewDefinitionList(Data.Vd);
			}
			else if(event.isCbSelected(CTLSEL_STQCMD_BASECMD)) {
				int32 new_base_cmd = getCtrlLong(CTLSEL_STQCMD_BASECMD);
				if(new_base_cmd != Data.BaseCmdId) {
					Data.BaseCmdId = new_base_cmd;
					SetupBaseCommand(new_base_cmd);
				}
			}
			else if(event.isCbSelected(CTLSEL_STQCMD_OTR)) {
				PPID new_obj_type = getCtrlLong(CTLSEL_STQCMD_OTR);
				if(new_obj_type != Data.ObjTypeRestriction) {
					Data.ObjTypeRestriction = new_obj_type;
					SetupObjGroupCombo(new_obj_type);
				}
			}
			else if(event.isCbSelected(CTLSEL_STQCMD_VCMD)) {
				/*
				uint    pos = 0;
				temp_buf.Z();
				if(CmdSymbList.SearchByText(nfilt.ViewSymb, 1, &pos)) {
					StrAssocArray::Item symb_item = CmdSymbList.at_WithoutParent(pos);
					uint  tpos = 0;
					if(CmdTextList.Search(symb_item.Id, &tpos))
						temp_buf = CmdTextList.at_WithoutParent(tpos).Txt;
				}
				*/
			}
			else
				return;
			clearEvent(event);
		}
		void ChangePersonEventTemplate()
		{
			const size_t sav_offs = Data.Param.GetRdOffs();
			PPPsnEventPacket pack;
			SSerializeContext sctx;
			if(Data.Param.GetAvailableSize()) {
				if(PsnEvObj.SerializePacket(-1, &pack, Data.Param, &sctx)) {
					;
				}
				else {
					Data.Param.Z();
				}
			}
			if(PsnEvObj.EditPacket(&pack, true) > 0) {
				Data.Param.Z();
				if(PsnEvObj.SerializePacket(+1, &pack, Data.Param, &sctx)) {
					;
				}
			}
		}
		void ChangeBaseFilter()
		{
			size_t sav_offs = 0;
			PPBaseFilt * p_filt = 0;
			PPView * p_view = 0;
			PPID   view_id = getCtrlLong(CTLSEL_STQCMD_VCMD);
			uint   pos = 0;
			if(view_id && CmdSymbList.Search(view_id, &pos)) {
				SString view_symb(CmdSymbList.at_WithoutParent(pos).Txt);
				sav_offs = Data.Param.GetRdOffs();
				THROW(PPView::CreateInstance(view_id, &p_view) > 0);
				assert(p_view);
				{
					if(Data.Param.GetAvailableSize()) {
						THROW(PPView::ReadFiltPtr(Data.Param, &p_filt));
						if(p_view && p_view->GetBaseFilt()) {
							if(p_filt->GetSignature() != p_view->GetBaseFilt()->GetSignature()) {
								// Путаница в фильтрах - убиваем считанный фильтр чтобы создать новый.
								ZDELETE(p_filt);
							}
						}
					}
					SETIFZ(p_filt, p_view->CreateFilt(0));
					if(p_view->EditBaseFilt(p_filt) > 0) {
						Data.Param.Z();
						THROW(p_view->WriteFiltPtr(Data.Param, p_filt));
					}
					else
						Data.Param.SetRdOffs(sav_offs);
				}
			}
			CATCH
				Data.Param.SetRdOffs(sav_offs);
			ENDCATCH
			ZDELETE(p_filt);
			ZDELETE(p_view);
		}
		void   SetupBaseCommand(int32 baseCmd)
		{
			bool enable_cmd_param = false;
			bool enable_viewcmd = false;
			if(baseCmd == StyloQCommandList::Item::sqbcPersonEvent) {
				enable_cmd_param = true;
			}
			else if(baseCmd == StyloQCommandList::Item::sqbcReport) {
				enable_cmd_param = true;
				long   view_id = 0;
				uint   pos = 0;
				if(Data.ViewSymb.NotEmpty() && CmdSymbList.SearchByText(Data.ViewSymb, 1, &pos)) {
					view_id = CmdSymbList.at_WithoutParent(pos).Id;
				}
				setCtrlLong(CTLSEL_STQCMD_VCMD, view_id);
				enable_viewcmd = true;
			}
			enableCommand(cmCmdParam, enable_cmd_param);
			disableCtrl(CTLSEL_STQCMD_VCMD, !enable_viewcmd);
		}
		void   SetupObjGroupCombo(PPID objType)
		{
			if(objType == PPOBJ_PERSON) {
				disableCtrl(CTLSEL_STQCMD_OGR, 0);
				SetupPPObjCombo(this, CTLSEL_STQCMD_OGR, PPOBJ_PERSONKIND, 0, 0);
			}
			else {
				disableCtrl(CTLSEL_STQCMD_OGR, 1);
			}
		}
	};
	DIALOG_PROC_BODY_P1(StyloQCommandDialog, &NfMgr, pData);
}

int PPViewStyloQCommand::AddItem(uint * pIdx)
{
	int    ok = -1;
	uint   new_item_idx = 0;
	StyloQCommandList::Item * p_new_item = List.CreateNewItem(&new_item_idx);
	if(p_new_item) {
		if(EditStyloQCommand(p_new_item) > 0) {
			ok = 1;
		}
		else
			List.Set(new_item_idx, 0);
	}
	return ok;
}

int PPViewStyloQCommand::EditItem(uint idx)
{
	int    ok = -1;
	StyloQCommandList::Item * p_item = idx ? List.Get(idx-1) : 0;
	if(p_item) {
		if(EditStyloQCommand(p_item) > 0) {
			ok = 1;
		}
	}
	return ok;
}

int PPViewStyloQCommand::DeleteItem(uint idx)
{
	int    ok = -1;
	StyloQCommandList::Item * p_item = idx ? List.Get(idx-1) : 0;
	if(p_item && CONFIRM(PPCFM_DELETE)) {
		List.Set(idx, 0);
	}
	return ok;
}

/*virtual*/int PPViewStyloQCommand::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = -2;
	uint   cur_idx = pHdr ? *static_cast<const uint *>(pHdr) : 0;
	const  uint preserve_idx = cur_idx;
	ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		switch(ppvCmd) {
			case PPVCMD_ADDITEM:
				{
					uint new_idx = 0;
					ok = AddItem(&new_idx);
				}
				break;
			case PPVCMD_EDITITEM:
				ok = EditItem(cur_idx);
				break;				
			case PPVCMD_DELETEITEM:
				ok = DeleteItem(cur_idx);
				break;			
			case PPVCMD_SAVE:
				ok = -1;
				{
					SString file_name;
					if(GetStyloQCommandFileName(file_name)) {
						if(List.Store(file_name)) {
							;
						}
					}
				}
				break;
			case PPVCMD_REFRESH:
				ok = 1;
				break;
		}
	}
	if(ok > 0) {
		MakeList(pBrw);
		if(pBrw) {
			AryBrowserDef * p_def = static_cast<AryBrowserDef *>(pBrw->getDef());
			if(p_def) {
				SArray * p_array = new SArray(*P_DsList);
				p_def->setArray(p_array, 0, 1);
				pBrw->setRange(p_array->getCount());
				uint   temp_pos = 0;
				long   update_pos = -1;
				if(preserve_idx > 0 && P_DsList->lsearch(&preserve_idx, &temp_pos, CMPF_LONG))
					update_pos = temp_pos;
				if(update_pos >= 0)
					pBrw->go(update_pos);
				else if(update_pos == MAXLONG)
					pBrw->go(p_array->getCount()-1);
			}
			pBrw->Update();
		}
	}
	return ok;
}