// CENTRIGO.CPP
// Copyright (c) A.Sobolev 2026
// @codepage UTF-8
// Основной модуль проекта centrigo
//
#include <pp.h>
#pragma hdrstop
//
// Descr: Список навигации для проекта Centrigo
//
class CentrigoNavBlock { // @v12.5.6 @centrigo
public:
	enum {
		ccatUndef      = 0,
		ccatCommand    = 1,
		ccatNotes      = 2,
		ccatActualToDo = 3,
		ccatWallet     = 4,
	};
	struct Entry {
		Entry() : Category(ccatUndef), LastAccsDtm(ZERODATETIME)
		{
		}
		int    Id; // Локальный ид для сопоставления с визуальным отображением //
		int    Category; // ccatXXX
		SObjID Oid; // Для команд (Category==ccatCommand) Oid.Obj==PPOBJ_UXCMD. Родительский узел для команд имеет Oid=={PPOBJ_UXCMD; 0}
		LDATETIME LastAccsDtm;
		SString Title;
		TSCollection <Entry> Children;
	};
	CentrigoNavBlock() : LastId(0)
	{
	}
	~CentrigoNavBlock()
	{
	}
	Entry * SearchEntry(int id)
	{
		return Helper_SearchEntry(L, id);
	}
	int    AddEntry(int parentId, int category)
	{
		SObjID zero_oid;
		LDATETIME zero_dtm = ZERODATETIME;
		return Helper_AddEntry(parentId, category, zero_oid, zero_dtm, 0);
	}
	int    AddEntry(int parentId, int category, const SObjID & rOid, const char * pTitle)
	{
		LDATETIME zero_dtm = ZERODATETIME;
		return Helper_AddEntry(parentId, category, rOid, zero_dtm, pTitle);
	}
	int    MakeStrAssocArray(StrAssocArray & rResult)
	{
		rResult.Z();
		return Helper_MakeStrAssocArray(L, 0, rResult);
	}
private:
	int    Helper_MakeStrAssocArray(const TSCollection <Entry> & rL, long parentId, StrAssocArray & rResult)
	{
		int    ok = 1;
		SString temp_buf;
		for(uint i = 0; i < rL.getCount(); i++) {
			const Entry * p_entry = rL.at(i);
			if(p_entry) {
				if(p_entry->Title.NotEmpty())
					temp_buf = p_entry->Title;
				else {
					temp_buf.Z().CatChar('#').Cat(p_entry->Id);
				}
				rResult.Add(p_entry->Id, parentId, temp_buf);
				if(p_entry->Children.getCount()) {
					Helper_MakeStrAssocArray(p_entry->Children, p_entry->Id, rResult); // @recursion
				}
			}
		}
		return ok;
	}
	Entry * Helper_SearchEntry(TSCollection <Entry> & rL, int id)
	{
		Entry * p_result = 0;
		if(id) {
			for(uint i = 0; !p_result && i < rL.getCount(); i++) {
				Entry * p_entry = rL.at(i);
				p_result = (p_entry->Id == id) ? p_entry : Helper_SearchEntry(p_entry->Children, id); // @recursion
			}
		}
		return p_result;
	}
	//
	// Descr: Реализуе создание нового элемента в иерархии навигации.
	// Returns:
	//   0 - error
	//  >0 - внутрений идентификатор созданного элемента
	//
	int    Helper_AddEntry(int parentId, int category, const SObjID & rOid, const LDATETIME & rLastAccsDtm, const char * pTitle)
	{
		int    result = 0;
		Entry * p_parent_entry = SearchEntry(parentId);
		TSCollection <Entry> & r_list = p_parent_entry ? p_parent_entry->Children : L;
		Entry * p_new_entry = r_list.CreateNewItem();
		if(p_new_entry) {
			p_new_entry->Id = ++LastId;
			p_new_entry->Category = category;
			p_new_entry->Oid = rOid;
			p_new_entry->LastAccsDtm = rLastAccsDtm;
			p_new_entry->Title = pTitle;
			result = p_new_entry->Id;
		}
		return result;
	}
	int    LastId;
	TSCollection <Entry> L;
};
//
//
//
class LocalStateBinderySelExtra : public WordSel_ExtraBlock {
public:
	explicit LocalStateBinderySelExtra(const LocalStateBinderyCore::StateIdent & rIdent);
	virtual StrAssocArray * GetList(const char * pText);
	virtual StrAssocArray * GetRecentList();
	virtual int Search(long id, SString & rBuf);
	virtual int SearchText(const char * pText, long * pID, SString & rBuf);
	virtual void OnAcceptInput(const char * pText, long id);
private:
	const LocalStateBinderyCore::StateIdent StI;
};

LocalStateBinderySelExtra::LocalStateBinderySelExtra(const LocalStateBinderyCore::StateIdent & rIdent) : WordSel_ExtraBlock(), StI(rIdent)
{
}

/*virtual*/StrAssocArray * LocalStateBinderySelExtra::GetList(const char * pText)
{
	StrAssocArray * p_result = 0;
	if(!isempty(pText)) {
		LocalStateBinderyCore * p_lstb = DS.GetTLA().GetLocalStateBindery();
		if(p_lstb) {
			SString temp_buf;
			SString key(pText);
			key.Transf(CTRANSF_INNER_TO_UTF8);
			TSCollection <LocalStateBinderyCore::SerialEntry> serial_list;
			LongArray pos_list; // [+1]
			p_lstb->FetchStateSerial(StI, &serial_list);
			LocalStateBinderyCore::SearchInSerial(serial_list, key, LocalStateBinderyCore::treatsSubStringUtf8List, pos_list);
			if(pos_list.getCount()) {
				for(uint i = 0; i < pos_list.getCount(); i++) {
					const  long iter_idx = pos_list.get(i);
					const  uint iter_pos = static_cast<uint>(iter_idx-1);
					const  LocalStateBinderyCore::SerialEntry * p_entry = serial_list.at(iter_pos);
					if(p_entry && p_entry->Buf.GetAvailableSize()) {
						if(!p_result) {
							p_result = new StrAssocArray();
						}
						LocalStateBinderyCore::GetStringFromStateBuf(LocalStateBinderyCore::treatbStringUtf8, p_entry->Buf, temp_buf);
						if(temp_buf.NotEmpty()) {
							if(!p_result->SearchByTextNcUtf8(temp_buf, 0))
								p_result->AddFast(p_entry->ID, temp_buf);
						}
					}
				}
			}
		}
	}
	return p_result;
}

/*virtual*/StrAssocArray * LocalStateBinderySelExtra::GetRecentList()
{
	StrAssocArray * p_result = 0;
	return p_result;
}

/*virtual*/int LocalStateBinderySelExtra::Search(long id, SString & rBuf)
{
	rBuf.Z();
	int    ok = 0;
	if(id) {
		LocalStateBinderyCore * p_lstb = DS.GetTLA().GetLocalStateBindery();
		if(p_lstb) {
			TSCollection <LocalStateBinderyCore::SerialEntry> serial_list;
			p_lstb->FetchStateSerial(StI, &serial_list);
			uint   pos = 0;
			if(serial_list.lsearch(&id, &pos, CMPF_LONG)) {
				const  LocalStateBinderyCore::SerialEntry * p_entry = serial_list.at(pos);
				if(p_entry) {
					LocalStateBinderyCore::GetStringFromStateBuf(LocalStateBinderyCore::treatbStringUtf8, p_entry->Buf, rBuf);
					ok = 1;
				}
			}
		}
	}
	return ok;
}

/*virtual*/int LocalStateBinderySelExtra::SearchText(const char * pText, long * pID, SString & rBuf)
{
	rBuf.Z();
	int    ok = 0;
	long   result_id = 0;
	if(!isempty(pText)) {
		LocalStateBinderyCore * p_lstb = DS.GetTLA().GetLocalStateBindery();
		if(p_lstb) {
			SString key(pText);
			TSCollection <LocalStateBinderyCore::SerialEntry> serial_list;
			LongArray pos_list; // [+1]
			p_lstb->FetchStateSerial(StI, &serial_list);
			LocalStateBinderyCore::SearchInSerial(serial_list, key, LocalStateBinderyCore::treatsStringUtf8List, pos_list);
			if(pos_list.getCount()) {
				const  long _idx = pos_list.get(0);
				const  uint _pos = static_cast<uint>(_idx-1);
				const  LocalStateBinderyCore::SerialEntry * p_entry = serial_list.at(_pos);
				if(p_entry && p_entry->Buf.GetAvailableSize()) {
					LocalStateBinderyCore::GetStringFromStateBuf(LocalStateBinderyCore::treatbStringUtf8, p_entry->Buf, rBuf);
					if(rBuf.NotEmpty()) {
						result_id = p_entry->ID;
						ok = 1;
					}
				}
			}
		}
	}
	ASSIGN_PTR(pID, result_id);
	return ok;
}

/*virtual*/void LocalStateBinderySelExtra::OnAcceptInput(const char * pText, long id)
{
	int    lstb_reg_state_result = 0;
	if(!isempty(pText)) {
		SString temp_buf(pText);
		if(temp_buf.NotEmptyS()) {
			LocalStateBinderyCore * p_lstb = DS.GetTLA().GetLocalStateBindery();
			if(p_lstb) {
				temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
				//LocalStateBinderyCore::StateIdent state_ident;
				//state_ident.Kind = LocalStateBinderyCore::kInput;
				//state_ident.Subj = UED::SetRaw_UXControlIdent(SObjID(WNDID_FACADEWINDOW, CTL_FACADEWINDOW_MAININPUT));
				//if(state_ident.Subj) {
				{
					PPID   sid = 0;
					SBuffer state_input_data;
					state_input_data.Write(temp_buf.cptr(), temp_buf.Len()+1);
					lstb_reg_state_result = p_lstb->RegisterState(&sid, StI, state_input_data, 1);
				}
			}
		}
	}
}

class TFacadeWindow : public TBaseBrowserWindow {
public:
	static constexpr int ViewId_Primary = 10001;

	TFacadeWindow();
	~TFacadeWindow();
private:
	DECL_HANDLE_EVENT;

	static const wchar_t * WndClsName;
	static int   RegWindowClass(HINSTANCE hInst);
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	int    WMHCreate();
	void   InitLayout();
	int    InsertWorkWindow(int ppviewId, const PPBaseFilt * pFilt); // @debug
	int    MakeNavList(CentrigoNavBlock & rBlk);
	int    DoNote(SObjID & rOid);
	int    DoContacts(const PersonFilt * pFilt);
	int    DoTasks(const PrjTaskFilt * pFilt);
	int    DoSecrets();
	int    HandleInputEnter(const SString & rInput);
	int    RemoveWorkingPanel();
	int    DrawNavTreeItem(void * pCustomDrawDescriptor);
	int    GetSecretsFilePath(SString & rBuf);
	int    LoadSecrets(SString * pFilePath, bool interactive);
	int    CloseSecrets();

	CentrigoNavBlock NavBlk;
	//
	PPObjWorkbook WbObj;
	SUiLayout * P_Lo_NavItem; // really const
	HWND  H_RecentFocusedChild; // @v12.6.0
	PPSecretSegmentPool SecPool; // @v12.6.9
};

//
// Descr: [Важно: это - модель. Продуктивный вариант предполагает вставку окна в общую панель centrigo].
//   Диалог просмотра и редактирования секретов.
//
class CentrigoSecretsDialog : public TDialog, public PPListDialogBaseInterface {
public:
	CentrigoSecretsDialog(void * hParentWindow, PPSecretSegmentPool & rSecPool, const SString & rFilePath);
	~CentrigoSecretsDialog();
private:
	struct TextFieldDescr {
		uint   CtlId;
		uint32 * P_DataIdx;
	};
	
	DECL_HANDLE_EVENT;
	virtual SmartListBox * GetListBoxCtl() const;
	virtual int  setupList();
	virtual int  addItem(long * pPos, long * pID);
	virtual int  editItem(long pos, long id);
	virtual int  delItem(long pos, long id);
	bool   MakeNewSegName(SString & rBuf) const;
	bool   MakeNewFolderName(SString & rBuf) const;
	bool   GetCurrentInput();
	void   ClearInputBlock();
	void   SetupParentList(uint currentIdent);
	void   SetupSegmentType(uint type);
	void   SetupSelectedSegment(uint segIdent);
	int    Helper_MakeStrAssocList(uint32 parentId, StrAssocArray * pList);
	StrAssocArray * MakeStrAssocList();

	PPSecretSegmentPool & R_SecPool;
	const  SString FilePath;
	//uint   CurrentSegIdx; // [1..], 0 - undef
	uint   CurrentSegIdent; 
	uint   CtlList;
	SmartListBox * P_Box;
};

int TFacadeWindow::MakeNavList(CentrigoNavBlock & rBlk)
{
	int    ok = 1;
	SString temp_buf;
	LongArray parent_list;
	{
		int    parent_id = 0;
		PPLoadStringUtf8("cmd_pl", temp_buf);
		parent_id = rBlk.AddEntry(0, CentrigoNavBlock::ccatCommand, SObjID(PPOBJ_UXCMD, 0), temp_buf);
		if(parent_id) {
			// @attention номера команд сейчас фиктивные только для отладки списка - потом установить правильные значения//
			static const SIntToSymbTabEntry cmd_list[] = {
				{ cmCentrigoNotes, "centrigo_cmd_notes" },
				{ cmCentrigoContacts, "centrigo_cmd_contacts" },
				{ cmCentrigoToDo, "centrigo_cmd_todo" },
				{ cmCentrigoWallet, "centrigo_cmd_wallet" },
				{ cmCentrigoSecrets, "centrigo_cmd_secrets" },
			};
			for(uint i = 0; i < SIZEOFARRAY(cmd_list); i++) {
				PPLoadStringUtf8(cmd_list[i].P_Symb, temp_buf);
				rBlk.AddEntry(parent_id, CentrigoNavBlock::ccatCommand, SObjID(PPOBJ_UXCMD, cmd_list[i].Id), temp_buf);
			}
			parent_list.add(parent_id);
		}
	}
	{
		struct IterNoteItem {
			PPID   ID;
			LDATETIME Dtm;
			char   Name[256];
		};
		SArray note_list(sizeof(IterNoteItem));
		WorkbookTbl::Rec rec;
		for(SEnum en = WbObj.P_Tbl->EnumByType(PPWBTYP_NOTE, 0); en.Next(&rec) > 0;) {
			IterNoteItem new_item;
			new_item.ID = rec.ID;
			new_item.Dtm.Set(rec.Dt, rec.Tm);
			(temp_buf = rec.Name).Transf(CTRANSF_INNER_TO_UTF8);
			STRNSCPY(new_item.Name, temp_buf);
			note_list.insert(&new_item);
		}
		if(note_list.getCount()) {
			CompFunc cf = [](const void * p1, const void * p2, void * pExtraData) -> int
			{
				const IterNoteItem * p_i1 = static_cast<const IterNoteItem *>(p1);
				const IterNoteItem * p_i2 = static_cast<const IterNoteItem *>(p2);
				int   ret = 0;
				if(p_i1->Dtm < p_i2->Dtm)
					ret = +1; // descending
				else if(p_i1->Dtm > p_i2->Dtm)
					ret = -1;
				else {
					ret = strcmp(p_i1->Name, p_i2->Name); // @todo Здесь должно быть сравнение без учета регистров в формате utf8
				}
				return ret;
			};

			note_list.sort2(cf);
			int    parent_id = 0;
			PPLoadStringUtf8("centrigo_cmd_notes", temp_buf);
			parent_id = rBlk.AddEntry(0, CentrigoNavBlock::ccatNotes, SObjID(PPOBJ_WORKBOOK, 0), temp_buf);
			if(parent_id) {
				for(uint i = 0; i < note_list.getCount(); i++) {
					const IterNoteItem * p_item = static_cast<const IterNoteItem *>(note_list.at(i));
					rBlk.AddEntry(parent_id, CentrigoNavBlock::ccatNotes, SObjID(PPOBJ_WORKBOOK, p_item->ID), p_item->Name);
				}
				parent_list.add(parent_id);
			}
		}
	}
	{
		SmartListBox * p_lb = static_cast<SmartListBox *>(getCtrlView(CTL_FACADEWINDOW_NAVPANE));
		if(p_lb) {
			StrAssocArray * p_list = new StrAssocArray();
			if(p_list) {
				rBlk.MakeStrAssocArray(*p_list);
				StdTreeListBoxDef * p_lb_def = new StdTreeListBoxDef(p_list, lbtDisposeData|lbtDblClkNotify|lbtTextUtf8);
				if(p_lb_def) {
					p_lb->setDef(p_lb_def);
					{
						for(uint ii = 0; ii < p_list->getCount(); ii++) {
							StrAssocArray::Item item = p_list->at_WithoutParent(ii);
							CentrigoNavBlock::Entry * p_entry = rBlk.SearchEntry(item.Id);
							if(p_entry && p_entry->Oid.Obj == PPOBJ_WORKBOOK && p_entry->Oid.Id) {
								p_lb_def->AddVecImageAssoc(item.Id, PPDV_DOCUMENT_TEXT01);
							}
						}
					}
					p_lb->SetExpandedTreeBranchList(&parent_list);
					if(P_Lo_NavItem) {
						float  height = 0.0f;
						const  int gfhr = P_Lo_NavItem->GetFullHeight(&height);
						if(gfhr) {
							HWND h_wnd_list = p_lb->getHandle();
							if(h_wnd_list) {
								::SendMessageW(h_wnd_list, TVM_SETITEMHEIGHT, static_cast<int>(height), 0);
							}
						}
					}
					p_lb->Draw_();
				}
			}
		}
	}
	return ok;
}

void TFacadeWindow::InitLayout()
{
	if(true) {
		SUiLayout * p_lo_main = new SUiLayout();
		SUiLayoutParam alb;
		p_lo_main->SetLayoutBlock(alb);
		p_lo_main->SetCallbacks(0, TWindowBase::SetupLayoutItemFrame, this);
		{
			// Ориентация горизонтальная потому что справа фиксированного размера полоса изменения размера, а слева - список, занимающий все оставшееся место
			SUiLayoutParam alb_left(DIREC_HORZ, 0, SUiLayoutParam::alignStretch);
			alb_left.GravityX = SIDE_LEFT;
			alb_left.GravityY = SIDE_CENTER;
			alb_left.SetFixedSizeX(128.0f);
			alb_left.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
			SUiLayout * p_lo = new SUiLayout(alb_left);
			p_lo->SetSymb("Facade_Left");
			p_lo_main->Insert(p_lo);
		}
		{
			SUiLayoutParam alb_ul(DIREC_HORZ, 0, SUiLayoutParam::alignStretch);
			alb_ul.GravityX = SIDE_LEFT;
			alb_ul.GravityY = SIDE_TOP;
			alb_ul.SetFixedSizeX(128.0f);
			alb_ul.SetFixedSizeY(64.0f);
			SUiLayout * p_lo = new SUiLayout(alb_ul);
			p_lo->SetSymb("Facade_UpperLeft");
			p_lo_main->Insert(p_lo);
		}
		{
			SUiLayoutParam alb_top(DIREC_VERT, 0, SUiLayoutParam::alignStretch);
			alb_top.GravityX = SIDE_CENTER;
			alb_top.GravityY = SIDE_TOP;
			alb_top.JustifyContent = SUiLayoutParam::alignCenter;
			alb_top.AlignItems = SUiLayoutParam::alignCenter;
			alb_top.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
			alb_top.SetFixedSizeY(96.0f);
			SUiLayout * p_lo = new SUiLayout(alb_top);
			p_lo->SetSymb("Facade_Top");
			p_lo_main->Insert(p_lo);
		}
		{
			SUiLayoutParam alb_center(DIREC_HORZ, 0, SUiLayoutParam::alignStretch);
			alb_center.GravityX = SIDE_CENTER;
			alb_center.GravityY = SIDE_CENTER;
			alb_center.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
			alb_center.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
			SUiLayout * p_lo = new SUiLayout(alb_center);
			p_lo->SetSymb("Facade_Center");
			p_lo_main->Insert(p_lo);
		}
		SetLayout(p_lo_main);
	}
}

int TFacadeWindow::DoContacts(const PersonFilt * pFilt)
{
	int    ok = -1;
	ok = InsertWorkWindow(PPVIEW_PERSON, pFilt);
	return ok;
}

int TFacadeWindow::DoTasks(const PrjTaskFilt * pFilt)
{
	int    ok = -1;
	ok = InsertWorkWindow(PPVIEW_PRJTASK, pFilt);
	return ok;
}

int TFacadeWindow::DoSecrets()
{
	int    ok = -1;
	SString file_path;
	CentrigoSecretsDialog * dlg = 0;
	if(LoadSecrets(&file_path, true/*interactive*/) > 0) {
		dlg = new CentrigoSecretsDialog(H(), SecPool, file_path);
		//ExecView(dlg);
		//
		///*
		{
			RemoveWorkingPanel();
			{
				SUiLayout * p_lo = P_Lfc->FindBySymb("Facade_Center");
				if(p_lo) {
					if(dlg) {
						InsertCtlWithCorrespondingNativeItem(dlg, ViewId_Primary, 0, 0);
						{
							SUiLayoutParam alb_;
							alb_.GrowFactor = 1.0;
							alb_.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
							{
								SUiLayout * p_result = 0;
								p_result = p_lo->InsertItem(dlg, &alb_);
								if(p_result) {
									p_result->SetCallbacks(0, TView::SetupLayoutItemFrameProc, dlg);
									::ShowWindow(dlg->H(), SW_SHOWNORMAL);
									//dlg->Launch_(this);
									{
										EvaluateLayout(getClientRect());
										invalidateAll(true);
										::UpdateWindow(H());
									}
									::PostMessageW(dlg->H(), WM_SETFOCUS, 0, 0);
								}
							}
						}
					}
				}
			}
		}
		//*/
	}
	// (диалог не модальный!) delete dlg;
	return ok;
}

int TFacadeWindow::DoNote(SObjID & rOid)
{
	int    ok = -1;
	const  LDATETIME now_dtm = getcurdatetime_();
	SString temp_buf;
	if(rOid.Obj == PPOBJ_WORKBOOK) {
		TBaseBrowserWindow * p_brw = 0;
		PPWorkbookPacket pack;
		if(!rOid.Id) {
			PPID   new_id = 0;
			PPWorkbookPacket new_pack;
			PPTransaction tra(1);
			THROW(tra);
			new_pack.Rec.Type = PPWBTYP_NOTE;
			new_pack.Rec.Dt = now_dtm.d;
			new_pack.Rec.Tm = now_dtm.t;
			THROW(WbObj.MakeUniqueName(temp_buf, 0, 0/*use_ta*/));
			STRNSCPY(new_pack.Rec.Name, temp_buf);
			THROW(WbObj.MakeUniqueCode(temp_buf, 0/*use_ta*/));
			STRNSCPY(new_pack.Rec.Symb, temp_buf);
			THROW(WbObj.PutPacket(&new_id, &new_pack, 0/*use_ta*/));
			THROW(tra.Commit());
			rOid.Id = new_id;
		}
		{
			WorkbookTbl::Rec rec;
			THROW(WbObj.Search(rOid.Id, &rec) > 0);
		}
		RemoveWorkingPanel();
		{
			SUiLayout * p_lo = P_Lfc->FindBySymb("Facade_Center");
			if(p_lo) {
				SObjTextRefIdent tri(rOid, PPTRPROP_MEMO);
				STextBrowser * p_tb = new STextBrowser(tri, /*pLexerSymb*/0, /*toolbarId*/-1);
				if(p_tb) {
					STextBrowser::Config cfg;
					p_tb->GetConfig(cfg);
					cfg.Flags |= (STextBrowser::Config::fAutoSaveText|STextBrowser::Config::fAutoSaveState);
					p_tb->SetConfig(cfg);
					p_tb->SetIdlePeriod(3);
					p_brw = p_tb;
				}
				if(p_brw) {
					InsertCtlWithCorrespondingNativeItem(p_brw, ViewId_Primary, 0, /*extraPtr*/0);
					{
						SUiLayoutParam alb_;
						alb_.GrowFactor = 1.0;
						alb_.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
						{
							SUiLayout * p_result = 0;
							p_result = p_lo->InsertItem(p_brw, &alb_);
							if(p_result) {
								p_result->SetCallbacks(0, TView::SetupLayoutItemFrameProc, p_brw);
								p_brw->Launch_(this);
								{
									EvaluateLayout(getClientRect());
									invalidateAll(true);
									::UpdateWindow(H());
								}
								::PostMessageW(p_brw->H(), WM_SETFOCUS, 0, 0);
							}
						}
					}
				}
			}
		}
	}
	CATCH
		ok = PPErrorZ();
	ENDCATCH
	return ok;
}

int TFacadeWindow::InsertWorkWindow(int ppviewId, const PPBaseFilt * pFilt)
{
	int    ok = -1;
	if(ppviewId) {
		SUiLayout * p_lo = P_Lfc->FindBySymb("Facade_Center");
		if(p_lo) {
			PPView * p_view = 0;
			RemoveWorkingPanel();
			if(PPView::Execute(ppviewId, pFilt, PPView::exefModeless|PPView::exefDontLaunchWindow, &p_view, 0)) {
				SUiLayoutParam alb_;
				alb_.GrowFactor = 1.0;
				alb_.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
				p_view->BrowseInLayout(this, "Facade_Center", alb_, /*10002*/ViewId_Primary);
				ok = 1;
			}
		}
	}
	return ok;
}

int TFacadeWindow::WMHCreate()
{
	SPaintToolBox * p_tb = APPL->GetUiToolBox();
	InitLayout();
	if(P_Lfc) {
		{
			SUiLayout * p_lo_top = P_Lfc->FindBySymb("Facade_Top");
			SUiLayout * p_lo_left = P_Lfc->FindBySymb("Facade_Left");
			if(p_lo_top) {
				const float input_fixed_y = SlConst::UiFixedInputY * 2.5f;
				TInputLine * p_il = 0;
				TInputLine * p_il_info = 0;
				{
					p_il = new TInputLine(TRect::_defr_, TInputLine::spcfSendReturnToOwner/*spcFlags*/, MKSTYPE(S_ZSTRING, 1024), MKSFMT(1024, 0));
					InsertCtlWithCorrespondingNativeItem(p_il, CTL_FACADEWINDOW_MAININPUT, 0, /*extraPtr*/0);
					{
						SUiLayoutParam alb_;
						alb_.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
						alb_.SetFixedSizeY(input_fixed_y);
						alb_.SetMargin(FRect(8.0f, 8.0f, 8.0f, 2.0f));
						{
							//
							// Если pView == 0, то мы вставляем лейаут без привязки к конкретному control'у. Обычно это
							// делается для вспомогательных разметочных лейаутов-контейнеров
							//
							SUiLayout * p_result = 0;
							p_result = p_lo_top->InsertItem(p_il, &alb_);
							if(p_result && p_il) {
								p_result->SetCallbacks(0, TView::SetupLayoutItemFrameProc, p_il);
								p_il->handleWindowsMessage(WM_INITDIALOG, 0, 0);
							}
						}
					}
					{
						LocalStateBinderyCore::StateIdent state_ident;
						state_ident.Kind = LocalStateBinderyCore::kInput;
						state_ident.Subj = UED::SetRaw_UXControlIdent(SObjID(WNDID_FACADEWINDOW, CTL_FACADEWINDOW_MAININPUT));
						SetupWordSelector(CTL_FACADEWINDOW_MAININPUT, new LocalStateBinderySelExtra(state_ident), 0, 1, 
							WordSel_ExtraBlock::fUtf8|WordSel_ExtraBlock::fFreeText/*|WordSel_ExtraBlock::fAlwaysSearchBySubStr*/);
					}
				}
				{
					p_il_info = new TInputLine(TRect::_defr_, TInputLine::spcfReadOnly, MKSTYPE(S_ZSTRING, 1024), MKSFMT(1024, 0));
					InsertCtlWithCorrespondingNativeItem(p_il_info, CTL_FACADEWINDOW_INFOLINE, 0, /*extraPtr*/0);
					{
						SUiLayoutParam alb_;
						alb_.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
						alb_.SetFixedSizeY(input_fixed_y);
						alb_.SetMargin(FRect(8.0f, 2.0f, 8.0f, 8.0f));
						{
							//
							// Если pView == 0, то мы вставляем лейаут без привязки к конкретному control'у. Обычно это
							// делается для вспомогательных разметочных лейаутов-контейнеров
							//
							SUiLayout * p_result = 0;
							p_result = p_lo_top->InsertItem(p_il_info, &alb_);
							if(p_result && p_il_info) {
								p_result->SetCallbacks(0, TView::SetupLayoutItemFrameProc, p_il_info);
								p_il_info->handleWindowsMessage(WM_INITDIALOG, 0, 0);
							}
						}
					}
				}
				{
					SPaintObj::Font * p_f = p_tb ? p_tb->GetFont(SDrawContext(static_cast<HDC>(0)), TProgram::tbiAccentInputFont) : 0;
					if(p_f) {
						HFONT f = static_cast<HFONT>(*p_f);
						if(p_il) {
							HWND local_hw = p_il->getHandle();
							::SendMessageW(local_hw, WM_SETFONT, reinterpret_cast<WPARAM>(f), TRUE);
						}
						if(p_il_info) {
							HWND local_hw = p_il_info->getHandle();
							::SendMessageW(local_hw, WM_SETFONT, reinterpret_cast<WPARAM>(f), TRUE);
						}
					}
				}
				//
				//InsertWorkWindow(PPVIEW_GOODSREST);

			}
			if(p_lo_left) {
				{
					StdTreeListBoxDef * p_lb_def = new StdTreeListBoxDef(0, lbtDisposeData|lbtDblClkNotify/*|lbtOwnerDraw*/);
					SmartListBox * p_lb = new SmartListBox(TRect::_defr_, p_lb_def, true/*is_tree*/);
					//p_lb->SetOwnerDrawState();
					/*if(p_lb->P_Def) {
						p_lb->P_Def->addItem(1, "The first tree-list item!");
					}*/
					InsertCtlWithCorrespondingNativeItem(p_lb, CTL_FACADEWINDOW_NAVPANE, 0, 0);
					{
						SUiLayoutParam alb_;
						alb_.SetGrowFactor(1.0f); 
						//alb_.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
						alb_.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
						//alb_.SetMargin(FRect(8.0f, 2.0f, 8.0f, 8.0f));
						{
							//
							// Если pView == 0, то мы вставляем лейаут без привязки к конкретному control'у. Обычно это
							// делается для вспомогательных разметочных лейаутов-контейнеров
							//
							SUiLayout * p_result = 0;
							p_result = p_lo_left->InsertItem(p_lb, &alb_);
							if(p_result && p_lb) {
								p_result->SetCallbacks(0, TView::SetupLayoutItemFrameProc, p_lb);
								p_lb->handleWindowsMessage(WM_INITDIALOG, 0, 0);
							}
						}
					}
				}
				{
					TFrame * p_gb = new TFrame(TFrame::fkSizeBar);
					p_gb->SetupSizing(H(), p_lo_left, DIREC_HORZ);
					InsertCtlWithCorrespondingNativeItem(p_gb, CTL_FACADEWINDOW_NAVPANE_SZF, 0, 0);
					{
						SUiLayoutParam alb_;
						alb_.SetFixedSizeX(6.0f);
						alb_.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
						//alb_.SetMargin(FRect(8.0f, 2.0f, 8.0f, 8.0f));
						{
							//
							// Если pView == 0, то мы вставляем лейаут без привязки к конкретному control'у. Обычно это
							// делается для вспомогательных разметочных лейаутов-контейнеров
							//
							SUiLayout * p_result = 0;
							p_result = p_lo_left->InsertItem(p_gb, &alb_);
							if(p_result && p_gb) {
								p_result->SetCallbacks(0, TView::SetupLayoutItemFrameProc, p_gb);
								p_gb->handleWindowsMessage(WM_INITDIALOG, 0, 0);
							}
						}						
					}
				}
			}
		}
		//InsertWorkWindow(0); // @debug
		{
			MakeNavList(NavBlk);
		}
	}
	return 1;
}

/*static*/const wchar_t * TFacadeWindow::WndClsName = L"TFacadeWindow"; // @global

int TFacadeWindow::DrawNavTreeItem(void * pCustomDrawDescriptor)
{
	int    result = 0;
	bool   debug_mark = false; // @debug
	if(pCustomDrawDescriptor) {
		NMTVCUSTOMDRAW * p_cd = reinterpret_cast<NMTVCUSTOMDRAW *>(pCustomDrawDescriptor);
		if(p_cd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
			if(P_Lo_NavItem) {
				HTREEITEM h_item = reinterpret_cast<HTREEITEM>(p_cd->nmcd.dwItemSpec);
				TVITEMW item;
				RECT   rc_item;
				RECT   rc_cli;
				wchar_t _text[512];
				MEMSZERO(item);
				item.hItem = h_item;
				item.mask = TVIF_TEXT|TVIF_PARAM|TVIF_STATE|TVIF_CHILDREN|TVIF_HANDLE|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
				item.stateMask = TVIS_EXPANDED|TVIS_EXPANDEDONCE;
				item.pszText = _text;
				item.cchTextMax = SIZEOFARRAY(_text);
				item.iImage = -1;
				if(TreeView_GetItem(p_cd->nmcd.hdr.hwndFrom, &item)) {
					// code: Value specifying the portion of the item for which to retrieve the bounding rectangle. 
					//   If this parameter is TRUE, the bounding rectangle includes only the text of the item. Otherwise, 
					//   it includes the entire line that the item occupies in the tree-view control.
					BOOL   has_valid_rect = TreeView_GetItemRect(p_cd->nmcd.hdr.hwndFrom, h_item, &rc_item, FALSE/*code*/);
					if(!has_valid_rect || rc_item.right <= rc_item.left || rc_item.bottom <= rc_item.top) { // Область элемента не определена - предварительная фаза
						HWND   h_focus = ::GetFocus();
						MEMSZERO(rc_cli);
					}
					else {
						SPaintToolBox * p_tb = APPL->GetUiToolBox();
						if(p_tb) {
							SUiLayout * p_lo = new SUiLayout(*P_Lo_NavItem);
							TCanvas2 canv(*p_tb, p_cd->nmcd.hdc);
							bool   is_there_image = false;
							int    item_state = TProgram::tbisBase;
							HIMAGELIST h_iml = 0;
							int    image_idx = -100;
							::GetClientRect(p_cd->nmcd.hdr.hwndFrom, &rc_cli);
							// @v12.7.3 {
							if(item.mask & TVIF_IMAGE) {
								image_idx = item.iImage;
								if(image_idx == I_IMAGECALLBACK) {
									;
								}
								else {
									h_iml = TreeView_GetImageList(p_cd->nmcd.hdr.hwndFrom, TVSIL_NORMAL);
									if(h_iml && image_idx >= 0 && image_idx < ImageList_GetImageCount(h_iml)) {
										is_there_image = true;
									}
								}
							}
							// } @v12.7.3 
							// Получение уровня вложенности элемента
							int    level = 0;
							{
								HTREEITEM h_parent = TreeView_GetParent(p_cd->nmcd.hdr.hwndFrom, h_item);
								while(h_parent) {
									level++;
									h_parent = TreeView_GetParent(p_cd->nmcd.hdr.hwndFrom, h_parent);
								}
							}
							const  int indent = TreeView_GetIndent(p_cd->nmcd.hdr.hwndFrom);
							if(p_cd->nmcd.uItemState & CDIS_FOCUS) {
								item_state = TProgram::tbisFocus;
							}
							else if(p_cd->nmcd.uItemState & CDIS_SELECTED) {
								item_state = TProgram::tbisSelect;
							}
							else if(p_cd->nmcd.uItemState & CDIS_HOT) {
								item_state = TProgram::tbisHover;
							}
							/*
								view LAYOUT_LI_CENTRIGONAV [horizontal] {
									view LOITEM_LI_CENTRIGONAV_HIND [size: (16, 16) margin: 4]; // Индикатор иерархии
									view LOITEM_LI_CENTRIGONAV_IMG [size: (16, 16) margin: 4]; // Изображение
									view LOITEM_LI_CENTRIGONAV_MAINTEXT [growfactor: 1 height: bycontainer margin: 4]; // Основной текст
								}
							*/ 
							SUiLayout * p_lo_hind = p_lo->FindById(LOITEM_LI_CENTRIGONAV_HIND);
							SUiLayout * p_lo_img = p_lo->FindById(LOITEM_LI_CENTRIGONAV_IMG);
							SUiLayout * p_lo_text = p_lo->FindById(LOITEM_LI_CENTRIGONAV_MAINTEXT);
							if(p_lo_img && !is_there_image) {
								p_lo_img->SetExcludedStatus();
							}
							{
								SUiLayout::Param evp;
								evp.ForceSize.x = static_cast<float>(rc_item.right - rc_item.left);
								evp.ForceSize.y = static_cast<float>(rc_item.bottom - rc_item.top);
								SUiLayoutParam & r_lp = p_lo->GetLayoutBlock();
								r_lp.Padding.a.x = static_cast<float>(indent * level);
								p_lo->Evaluate(&evp);
							}
							{
								int   state_brush_id = TProgram::tbiListBkgBrush;
								int   state_pen_id = TProgram::tbiListBkgPen;
								if(item_state == TProgram::tbisSelect) {
									state_brush_id = TProgram::tbiListSelBrush;
									state_pen_id = TProgram::tbiListSelPen;
								}
								else if(item_state == TProgram::tbisFocus) {
									state_brush_id = TProgram::tbiListFocBrush;
									state_pen_id = TProgram::tbiListFocPen;
								}
								else if(item_state == TProgram::tbisHover) {
									state_brush_id = TProgram::tbiListFocBrush;
									state_pen_id = TProgram::tbiListFocPen;
								}
								{
									FRect rect_elem_f(rc_item);
									rect_elem_f.Grow(-0.5f, -0.5f);
									// canv.RoundRect(rect_elem_f, 3, pen_id, brush_id);
									canv.Rect(rect_elem_f, state_pen_id, state_brush_id);
								}
								if(p_lo_hind && item.cChildren) {
									const  bool is_expanded = LOGIC(item.state & TVIS_EXPANDED);
									uint   dv_id = is_expanded ? PPDV_TRIANGLELEFT03 : PPDV_TRIANGLEDOWN03;
									if(dv_id) {
										TWhatmanToolArray::Item tool_item;
										const SDrawFigure * p_fig = APPL->LoadDrawFigureById(dv_id, &tool_item);
										if(p_fig) {
											if(!tool_item.ReplacedColor.IsEmpty()) {
												SColor replacement_color = p_tb->GetColor(TProgram::tbiIconRegColor);
												canv.SetColorReplacement(tool_item.ReplacedColor, replacement_color);
											}
											FRect fr = p_lo_hind->GetFrameAdjustedToParent();
											fr.Move__(static_cast<float>(rc_item.left), static_cast<float>(rc_item.top));
											LMatrix2D mtx;
											SViewPort vp;
											canv.PushTransform();
											p_fig->GetViewPort(&vp);
											canv.AddTransform(vp.GetMatrix(fr, mtx));
											canv.Draw(p_fig);
											canv.PopTransform();
											canv.ResetColorReplacement();
										}
									}
								}
								if(p_lo_text) {
									HFONT  hf = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);
									int    temp_font_id = p_tb->CreateFont_(0, hf, 0);
									if(temp_font_id) {
										STextLayout tlo;
										SDrawContext dctx = canv;
										int   text_pen_id = TProgram::tbiListFgPen;
										if(item_state == TProgram::tbisSelect) {
											text_pen_id = TProgram::tbiListSelFgPen;
										}
										else if(item_state == TProgram::tbisFocus) {
											text_pen_id = TProgram::tbiListFocFgPen;
										}
										else if(item_state == TProgram::tbisHover) {
											text_pen_id = TProgram::tbiListFocPen;
										}
										if(APPL->GetDialogTextLayoutU(_text, temp_font_id, text_pen_id, tlo, ADJ_LEFT) > 0) {
											FRect fr = p_lo_text->GetFrameAdjustedToParent();
											fr.Move__(static_cast<float>(rc_item.left), static_cast<float>(rc_item.top));
											tlo.SetBounds(fr);
											tlo.SetOptions(tlo.fVCenter, -1, -1);
											tlo.Arrange(dctx, *p_tb);
											canv.DrawTextLayout(&tlo);
										}
										//*/
									}
								}
								if(p_lo_img && is_there_image) {
									// ICON here
									FRect fr = p_lo_img->GetFrameAdjustedToParent();
									fr.Move__(static_cast<float>(rc_item.left), static_cast<float>(rc_item.top));
									//ImageList_Draw(h_iml, image_idx, p_cd->nmcd.hdc, fr.a.x, fr.a.y, 0);
									HICON  h_ico = ImageList_GetIcon(h_iml, image_idx, 0/*flags*/);
									if(h_ico) {
										SImageBuffer img_buf;
										if(img_buf.LoadIco(h_ico)) {
											LMatrix2D mtx;
											SViewPort vp;
											vp.Flags &= ~SViewPort::fEmpty;
											// @v12.7.7 @ctr vp.a.Z();
											// @v12.7.7 vp.b.Set(static_cast<float>(img_buf.GetWidth()), static_cast<float>(img_buf.GetHeight()));
											vp.b = img_buf.GetDimF(); // @v12.7.7
											canv.PushTransform();
											canv.AddTransform(vp.GetMatrix(fr, mtx));
											canv.Draw(&img_buf);
											canv.PopTransform();											
										}
									}
								}
							}
							debug_mark = true; // @debug
							// (если мы все сами нарисовали, то возвращаем это) 
							result = CDRF_SKIPDEFAULT;
							delete p_lo;
						}
					}
					debug_mark = true; // @debug
				}
			}
		}
	}
	return result;
}

/*static*/LRESULT CALLBACK TFacadeWindow::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	CREATESTRUCT * p_init_data;
	TFacadeWindow * p_view = 0;
	bool   debug_mark = false; // @debug
	switch(message) {
		case WM_CREATE:
			{
				int    ret = TWindowBase::OnCreate(hWnd, message, wParam, lParam);
				if(ret == 0) {
					p_view = static_cast<TFacadeWindow *>(Helper_InitCreation(lParam, (void **)&p_init_data));
					assert(p_view); // Функция TWindowBase::OnCreate должна была это проверить 
					if(p_view) {
						//p_view->HW = hWnd;
						//TView::SetWindowProp(hWnd, GWLP_USERDATA, p_view);
						::SetFocus(hWnd);
						::SendMessageW(hWnd, WM_NCACTIVATE, TRUE, 0L);
						p_view->WMHCreate();
						::PostMessageW(hWnd, WM_PAINT, 0, 0);
						{
							SString temp_buf;
							TView::SGetWindowText(hWnd, temp_buf);
							APPL->AddItemToMenu(temp_buf, p_view);
						}
					}
				}
				return ret;
			}
		case WM_NOTIFY:
			{
				p_view = static_cast<TFacadeWindow *>(TView::GetWindowUserData(hWnd));
				if(p_view) {
					NMHDR * p_nm = reinterpret_cast<NMHDR *>(lParam);
					if(p_nm) {
						if(p_nm->code == NM_CUSTOMDRAW) {
							TView * p_ctl = p_view->getCtrlView(p_nm->idFrom);
							if(p_ctl && p_ctl->IsSubSign(TV_SUBSIGN_LISTBOX)) {
								NMTVCUSTOMDRAW * p_cd = reinterpret_cast<NMTVCUSTOMDRAW *>(p_nm);
								long   result = CDRF_DODEFAULT;
								switch(p_cd->nmcd.dwDrawStage) {
	    							case CDDS_PREPAINT:
										result = CDRF_NOTIFYITEMDRAW;
										break;
									case CDDS_ITEMPREPAINT:
										result = p_view->DrawNavTreeItem(p_cd);
										break;
								}
								return result;
							}
						}
						else {
							TView * p_iter_view = p_view->P_Last;
							if(p_iter_view != 0) {
								do {
									if(p_iter_view->TestId(wParam)) {
										p_iter_view->handleWindowsMessage(message, wParam, lParam);
										break;
									}
								} while((p_iter_view = p_iter_view->prev()) != p_view->P_Last);
							}
						}
					}
				}
			}
			break;
		case WM_COMMAND:
			{
				// Блок почти один-в-один скопирован из TDialog::DialogProc
				uint16 hiw = HIWORD(wParam);
				uint16 low = LOWORD(wParam);
				p_view = static_cast<TFacadeWindow *>(TView::GetWindowUserData(hWnd));
				if(GetKeyState(VK_CONTROL) & 0x8000 && low != cmaCalculate && hiw != EN_UPDATE && hiw != EN_CHANGE)
					return 0;
				else if(p_view) {
					if(hiw == 0 && low == IDCANCEL) {
						TView::messageCommand(p_view, cmCancel, p_view);
						return 0;
					}
					else {
						if(!lParam) {
							if(hiw == 0) // from menu
								TView::messageKeyDown(p_view, low);
							else if(hiw == 1) { // from accelerator
								TEvent event;
								event.what = TEvent::evCommand;
								event.message.command = low;
								p_view->handleEvent(event);
							}
						}
						TView * local_p_view = p_view->CtrlIdToView(CLUSTER_ID(low));
						if(local_p_view && local_p_view->IsConsistent())
							local_p_view->handleWindowsMessage(message, wParam, lParam);
					}
				}
				else
					return 0;
			}
			break;
		case WM_USER_KEYDOWN:
			{
				p_view = static_cast<TFacadeWindow *>(TView::GetWindowUserData(hWnd));
				if(p_view) {
					uint   key = static_cast<uint>(wParam);
					HWND   h_ctl = reinterpret_cast<HWND>(lParam);
					TView * p_ctl = static_cast<TView *>(TView::GetWindowUserData(h_ctl));
					if(key == VK_RETURN && p_ctl && p_ctl->IsSubSign(TV_SUBSIGN_INPUTLINE)) {
						TInputLine * p_il = static_cast<TInputLine *>(p_ctl);
						char   temp_b[1024];
						p_il->TransmitData(-1, temp_b);
						SString input_buf(temp_b);
						p_view->HandleInputEnter(input_buf);
					}
				}
			}
			return 0;
		case WM_DESTROY:
			p_view = static_cast<TFacadeWindow *>(TView::GetWindowUserData(hWnd));
			if(p_view && p_view->IsConsistent()) {
				//p_view->SaveChanges();
				TWindowBase::Helper_Finalize(hWnd, p_view);
			}
			return 0;
		case WM_SETFOCUS:
			p_view = static_cast<TFacadeWindow *>(TView::GetWindowUserData(hWnd));
			TWindowBase::Helper_SetFocus(hWnd, p_view);
			if(p_view) {
				if(p_view->H_RecentFocusedChild && p_view->H_RecentFocusedChild != hWnd) {
					::SetFocus(p_view->H_RecentFocusedChild);
				}
			}
			break;
		case WM_KILLFOCUS:
			if(!(TView::SGetWindowStyle(hWnd) & WS_CAPTION))
				APPL->NotifyFrame(0);
			p_view = static_cast<TFacadeWindow *>(TView::GetWindowUserData(hWnd));
			if(p_view) {
				TView::messageBroadcast(p_view, cmReleasedFocus);
				p_view->ResetOwnerCurrent();
			}
			break;
		case WM_KEYDOWN:
			if(wParam == VK_TAB) {
				p_view = static_cast<TFacadeWindow *>(TView::GetWindowUserData(hWnd));
				if(p_view) {
					//TView * p_primary_view = p_view->getCtrlView(ViewId_Primary);
					if(GetKeyState(VK_CONTROL) & 0x8000 && !p_view->IsInState(sfModal)) {
						SetFocus(GetNextBrowser(hWnd, (GetKeyState(VK_SHIFT) & 0x8000) ? 0 : 1));
						return 0;
					}
				}
			}
			return 0;
		case WM_SIZE:
			p_view = static_cast<TFacadeWindow *>(TView::GetWindowUserData(hWnd));
			if(!TView::Helper_SendCmSizeAsReplyOnWmSize(p_view, wParam, lParam))
				return 0;
			break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

/*static*/int TFacadeWindow::RegWindowClass(HINSTANCE hInst)
{
	WNDCLASSEXW wc;
	TBaseBrowserWindow::MakeDefaultWindowClassBlock(&wc, hInst);
	{
		const UiDescription * p_uid = SLS.GetUiDescription();
		const SColorSet * p_cs = p_uid ? p_uid->GetColorSetC("papyrus_style") : 0;
		SColor clr = UiDescription::GetColorR(p_uid, p_cs, "main_window_bg", SColor(0xEE, 0xEE, 0xEE));
		wc.hbrBackground = ::CreateSolidBrush(static_cast<COLORREF>(clr));
	}
	wc.lpfnWndProc   = TFacadeWindow::WndProc;
	wc.hIcon         = LoadIconW(hInst, MAKEINTRESOURCE(/*ICON_TIMEGRID*/172));
	wc.lpszClassName = TFacadeWindow::WndClsName;
	return RegisterClassExW(&wc);
}

TFacadeWindow::TFacadeWindow() : TBaseBrowserWindow(WndClsName), P_Lo_NavItem(0), H_RecentFocusedChild(0)
{
	BbState |= (bbsWoScrollbars|bbsCtlParent);
	static bool win_cls_registered = false;
	if(!win_cls_registered) {
		TFacadeWindow::RegWindowClass(TProgram::GetInst());
		win_cls_registered = true;
	}
	{ // @v12.5.7 @construction
		SString temp_buf;
		temp_buf.Z().Cat(LAYOUT_LI_CENTRIGONAV);
		P_Lo_NavItem = PPLoadDl600Layout(temp_buf, 0); 
	}
}

TFacadeWindow::~TFacadeWindow()
{
	ZDELETE(P_Lo_NavItem);
	H_RecentFocusedChild = 0;
}

int TFacadeWindow::RemoveWorkingPanel()
{
	int    ok = -1;
	TView * p_view = getCtrlView(ViewId_Primary);
	if(p_view) {
		removeView(p_view);
		P_Lfc->DeleteItemByManagedPtr(p_view);
		delete p_view;
		ok = 1;
	}
	return ok;
}

int TFacadeWindow::HandleInputEnter(const SString & rInput)
{
	int    ok = -1;
	const  SString org_input(rInput);
	SString _input(org_input);
	SString _result_text;
	if(_input.NotEmptyS()) {
		/*
		int    lstb_reg_state_result = 0;
		_input.Transf(CTRANSF_INNER_TO_UTF8);
		LocalStateBinderyCore * p_lstb = DS.GetTLA().GetLocalStateBindery();
		if(p_lstb) {
			LocalStateBinderyCore::StateIdent state_ident;
			state_ident.Kind = LocalStateBinderyCore::kInput;
			state_ident.Subj = UED::SetRaw_UXControlIdent(SObjID(WNDID_FACADEWINDOW, CTL_FACADEWINDOW_MAININPUT));
			if(state_ident.Subj) {
				PPID   sid = 0;
				SBuffer state_input_data;
				state_input_data.Write(_input.cptr(), _input.Len()+1);
				lstb_reg_state_result = p_lstb->RegisterState(&sid, state_ident, state_input_data, 1);
			}
		}
		*/
		TView::CallOnAcceptInputForWordSelExtraBlocks(this); // @v12.5.10
		if(_input.IsEqiAscii("close")) { // @debug
			RemoveWorkingPanel();
		}
		else if(_input.HasPrefixIAscii("=")) {
			_input.ShiftLeft();
			double result = 0.0;
			if(PPExprParser::CalcExpression(_input, &result, 0, 0)) {
				_result_text.Cat(_input).CatDiv('=', 1).Cat(result, MKSFMTD(0, 6, NMBF_NOTRAILZ));
			}
			else {
				_result_text.Cat("Bad expression to evaluate").CatDiv(':', 2).Cat(_input);
			}
		}
		else {
			const uint32 fav_nt_list[] =  {
				SNTOK_GUID, SNTOK_EMAIL, SNTOK_PHONE
			};
			STokenRecognizer tr;
			SNaturalTokenArray nta;
			uint32 fav_nt_id = 0;
			SString temp_buf;
			tr.Run(_input.ucptr(), _input.Len(), nta, 0);
			for(uint i = 0; !fav_nt_id && i < nta.getCount(); i++) {
				const SNaturalToken & r_nt = nta.at(i);
				for(uint j = 0; j < SIZEOFARRAY(fav_nt_list); j++) {
					if(fav_nt_list[j] == r_nt.ID) {
						r_nt.GetSymb(temp_buf);
						if(temp_buf.NotEmpty()) {
							_result_text.CatDivIfNotEmpty(' ', 0).Cat(temp_buf);
						}
						break;
					}
				}
			}
		}
	}
	setCtrlString(CTL_FACADEWINDOW_MAININPUT, _input.Z());
	setCtrlString(CTL_FACADEWINDOW_INFOLINE, _result_text);
	return ok;
}

IMPL_HANDLE_EVENT(TFacadeWindow)
{
	bool   debug_mark = false; // @debug
	TWindowBase::handleEvent(event);
	if(TVKEYDOWN) {
		if(oneof4(TVKEY, kbAltLeft, kbAltRight, kbAltUp, kbAltDown)) {
			HWND   h_focus = ::GetFocus();
			if(h_focus) {
				//TView * p_view_p = getCtrlView(ViewId_Primary);
				const TView * p_view_ = getCtrlViewByHandleC(h_focus);
				if(p_view_) {
					if(p_view_->GetId() == ViewId_Primary) {
						if(TVKEY == kbAltLeft) {
							TView * p_new_focus = getCtrlView(CTL_FACADEWINDOW_NAVPANE);
							if(p_new_focus) {
								::SetFocus(p_new_focus->getHandle());
							}
						}
						else if(TVKEY == kbAltUp) {
							TView * p_new_focus = getCtrlView(CTL_FACADEWINDOW_MAININPUT);
							if(p_new_focus) {
								::SetFocus(p_new_focus->getHandle());
							}
						}
					}
					else if(p_view_->GetId() == CTL_FACADEWINDOW_NAVPANE) {
						if(TVKEY == kbAltRight) {
							TView * p_new_focus = getCtrlView(ViewId_Primary);
							if(p_new_focus) {
								HWND   h_new_focus = p_new_focus->getHandle();
								::SetFocus(h_new_focus);
							}
						}
						else if(TVKEY == kbAltUp) {
							TView * p_new_focus = getCtrlView(CTL_FACADEWINDOW_MAININPUT);
							if(p_new_focus) {
								HWND   h_new_focus = p_new_focus->getHandle();
								::SetFocus(h_new_focus);
							}
						}
					}
					else if(p_view_->GetId() == CTL_FACADEWINDOW_MAININPUT) {
						if(TVKEY == kbAltDown) {
							TView * p_new_focus = getCtrlView(ViewId_Primary);
							if(p_new_focus) {
								::SetFocus(p_new_focus->getHandle());
							}
						}
						else if(TVKEY == kbAltLeft) {
							TView * p_new_focus = getCtrlView(CTL_FACADEWINDOW_NAVPANE);
							if(p_new_focus) {
								::SetFocus(p_new_focus->getHandle());
							}
						}
					}
				}
				//getCtrlView
				debug_mark = true; // @debug
			}
		}
	}
	else if(TVINFOPTR) {
		if(event.isCmd(cmInit)) {
			;
		}
		else if(event.isCmd(cmPaint)) {
			;
		}
		else if(event.isCmd(cmSize)) {
			;
		}
		else if(event.isCmd(cmMouse)) {
			MouseEvent * p_me = static_cast<MouseEvent *>(TVINFOPTR);
			if(p_me) {
				debug_mark = true; // @debug
			}
		}
		else if(event.isCmd(cmChildFocusReceived)) { // @v12.6.0
			TView * p_child = static_cast<TView *>(TVINFOPTR);
			HWND   h_c = p_child->getHandle();
			if(h_c && h_c != H()) {
				H_RecentFocusedChild = h_c;
			}
		}
		else if(event.isCmd(cmRightClick)) {
			TView * p_view = static_cast<TView *>(TVINFOPTR);
			if(p_view->IsConsistent() && p_view->IsSubSign(TV_SUBSIGN_LISTBOX)) {
				SmartListBox * p_lb = static_cast<SmartListBox *>(p_view);
				int    _id = 0;
				if(p_lb->getCurID(&_id)) {
					if(_id) {
						const CentrigoNavBlock::Entry * p_entry = NavBlk.SearchEntry(_id);
						if(p_entry) {
							debug_mark = true; // @debug
							if(p_entry->Oid.Obj == PPOBJ_WORKBOOK) {
								
							}
						}
					}
				}
				clearEvent(event);
			}
		}
		else if(event.isCmd(cmLBDblClk)) {
			TView * p_view = static_cast<TView *>(TVINFOPTR);
			if(p_view->IsConsistent() && p_view->IsSubSign(TV_SUBSIGN_LISTBOX)) {
				SmartListBox * p_lb = static_cast<SmartListBox *>(p_view);
				int    _id = 0;
				if(p_lb->getCurID(&_id)) {
					if(_id) {
						const CentrigoNavBlock::Entry * p_entry = NavBlk.SearchEntry(_id);
						if(p_entry) {
							debug_mark = true; // @debug
							switch(p_entry->Oid.Obj) {
								case PPOBJ_UXCMD:
									switch(p_entry->Oid.Id) {
										case cmCentrigoNotes:
											{
												SObjID oid(PPOBJ_WORKBOOK, 0);
												DoNote(oid);
											}
											break;
										case cmCentrigoContacts:
											{
												PersonFilt filt;
												filt.Flags |= (PersonFilt::fInMemView|PersonFilt::fCentrigoContacts);
												DoContacts(&filt);
											}
											break;
										case cmCentrigoToDo:
											{
												PrjTaskFilt filt;
												filt.Flags |= PrjTaskFilt::fInMemView;
												DoTasks(&filt);
											}
											break;
										case cmCentrigoWallet:
											break;
										case cmCentrigoSecrets:
											{
												DoSecrets();
											}
											break;
									}
									break;
								case PPOBJ_WORKBOOK:
									{
										SObjID oid(p_entry->Oid);
										DoNote(oid);
									}
									break;
							}
						}
					}
				}
				
			}
		}
	}
}
//
//
//
static SIntToSymbTabEntry SecSegTypeList[] = {
	{ PPSecretSegment::sectypUndef, "undef" },
	{ PPSecretSegment::sectypFolder, "folder" },
	{ PPSecretSegment::sectypGeneric, "generic" },
	{ PPSecretSegment::sectypPassword, "password" },
	{ PPSecretSegment::sectypAuthSecret, "authsecret" },
	{ PPSecretSegment::sectypOpenKey, "openkey" },
	{ PPSecretSegment::sectypBankCard, "bankcard" },
	{ PPSecretSegment::sectypSSH, "ssh" },
	{ PPSecretSegment::sectypESignature, "esignature" },
	{ PPSecretSegment::sectypPlainText, "plaintext" },
};

PPSecretSegment::CoreEntry::CoreEntry()
{
	THISZERO();
}
		
PPSecretSegment::CoreEntry & PPSecretSegment::CoreEntry::Z()
{
	THISZERO();
	return *this;
}
		
bool PPSecretSegment::CoreEntry::IsEmpty() const
{
	return (!STextOpenP && !STextHiddenP);
}

PPSecretSegment::PPSecretSegment() : P_OwnSg(0), P_OuterSg(0)
{
	memzero(&InternalID, offsetof(PPSecretSegment, Reserve) + sizeof(Reserve) - offsetof(PPSecretSegment, InternalID));
}

PPSecretSegment::PPSecretSegment(const PPSecretSegment & rS) : P_OwnSg(0), P_OuterSg(0)
{
	Copy(rS);
}
	
PPSecretSegment::PPSecretSegment(SStrGroup * pOuterSg) : P_OwnSg(0), P_OuterSg(pOuterSg)
{
	memzero(&InternalID, offsetof(PPSecretSegment, Reserve) + sizeof(Reserve) - offsetof(PPSecretSegment, InternalID));
}
	
PPSecretSegment::~PPSecretSegment()
{
	if(P_OwnSg) {
		P_OwnSg->DestroySecureS(); // @v12.7.0
		delete P_OwnSg;
	}
}

bool PPSecretSegment::IsEmpty() const 
{
	return (!SecType && !NameP);
}

PPSecretSegment & PPSecretSegment::Z()
{
	memzero(&InternalID, offsetof(PPSecretSegment, Reserve) + sizeof(Reserve) - offsetof(PPSecretSegment, InternalID));
	History.clear();
	if(P_OwnSg) {
		P_OwnSg->ClearS();
	}
	return *this;
}

bool FASTCALL PPSecretSegment::IsEq(const PPSecretSegment & rS) const
{
	bool   eq = true;
	if(InternalID != rS.InternalID)
		eq = false;
	else if(ParentID != rS.ParentID)
		eq = false;
	else if(SecType != rS.SecType)
		eq = false;
	else if(UedRefOid != rS.UedRefOid)
		eq = false;
	else if(UedEnterTm != rS.UedEnterTm)
		eq = false;
	else if(UedHashAlg != rS.UedHashAlg)
		eq = false;
	else if(CE.UedBeforeTm != rS.CE.UedBeforeTm)
		eq = false;
	else if(History.getCount() != rS.History.getCount())
		eq = false;
	else {
		{
			for(uint i = 0; eq && i < History.getCount(); i++) {
				const  CoreEntry & r_ce = History.at(i);
				const  CoreEntry & r_ce2 = rS.History.at(i);
				if(r_ce.UedBeforeTm != r_ce2.UedBeforeTm) {
					eq = false;
				}
			}
		}
		if(eq) {
			TSVector <uint> spl1;
			TSVector <uint> spl2;
			spl1.insert(&NameP);
			spl1.insert(&DescrP);
			spl1.insert(&ExecCmdLineP);
			spl1.insert(&CE.STextOpenP);
			spl1.insert(&CE.STextHiddenP);
			spl1.insert(&CE.STextExpiryP);
			spl1.insert(&CE.STextExt1P);
			spl1.insert(&CE.STextExt2P);
			spl1.insert(&CE.STextExt3P);
			assert(History.getCount() == rS.History.getCount()); // Проверено выше!
			{
				for(uint i = 0; i < History.getCount(); i++) {
					const  CoreEntry & r_ce = History.at(i);
					spl1.insert(&r_ce.STextOpenP);
					spl1.insert(&r_ce.STextHiddenP);
					spl1.insert(&r_ce.STextExpiryP);
					spl1.insert(&r_ce.STextExt1P);
					spl1.insert(&r_ce.STextExt2P);
					spl1.insert(&r_ce.STextExt3P);
				}
			}
			spl2.insert(&rS.NameP);
			spl2.insert(&rS.DescrP);
			spl2.insert(&rS.ExecCmdLineP);
			spl2.insert(&rS.CE.STextOpenP);
			spl2.insert(&rS.CE.STextHiddenP);
			spl2.insert(&rS.CE.STextExpiryP);
			spl2.insert(&rS.CE.STextExt1P);
			spl2.insert(&rS.CE.STextExt2P);
			spl2.insert(&rS.CE.STextExt3P);
			{
				for(uint i = 0; i < rS.History.getCount(); i++) {
					const  CoreEntry & r_ce = rS.History.at(i);
					spl2.insert(&r_ce.STextOpenP);
					spl2.insert(&r_ce.STextHiddenP);
					spl2.insert(&r_ce.STextExpiryP);
					spl2.insert(&r_ce.STextExt1P);
					spl2.insert(&r_ce.STextExt2P);
					spl2.insert(&r_ce.STextExt3P);
				}
			}
			assert(spl1.getCount() == spl2.getCount()); // Если не выполняется, то я ошибся где-то выше!
			if(spl1.getCount() != spl2.getCount()) {
				eq = false;
			}
			else {
				const  SStrGroup & r_sg1 = GetSgC();
				const  SStrGroup & r_sg2 = rS.GetSgC();
				SString t1;
				SString t2;
				for(uint i = 0; eq && i < spl1.getCount(); i++) {
					const  uint p1 = spl1.at(i);
					const  uint p2 = spl2.at(i);
					r_sg1.GetS(p1, t1);
					r_sg2.GetS(p2, t2);
					if(t1 != t2)
						eq = false;
				}
			}
		}
	}
	return eq;
}

PPSecretSegment & FASTCALL PPSecretSegment::operator = (const PPSecretSegment & rS)
{
	Copy(rS);
	return *this;
}

bool FASTCALL PPSecretSegment::Copy(const PPSecretSegment & rS)
{
	Z();
	ZDELETE(P_OwnSg);
	bool   ok = true;
	#define CPYFLD(f) f = rS.f
	CPYFLD(InternalID);
	CPYFLD(ParentID);
	CPYFLD(SecType);
	CPYFLD(UedRefOid);
	CPYFLD(UedEnterTm);
	CPYFLD(UedHashAlg);
	CPYFLD(NameP);        // Наименование сегмента (позиция в SStrGroup) 
	CPYFLD(CE);
	CPYFLD(DescrP);
	CPYFLD(ExecCmdLineP);
	CPYFLD(History);
	#undef CPYFLD
	if(rS.P_OwnSg) {
		P_OwnSg = new SStrGroup(*rS.P_OwnSg);
	}
	if(rS.P_OuterSg) {
		P_OuterSg = rS.P_OuterSg;
	}
	return ok;
}

bool PPSecretSegment::GetText(uint32 textP, SString & rBuf) const
{
	return GetSgC().GetS(textP, rBuf);
}

SStrGroup & PPSecretSegment::GetSg()
{
	SStrGroup * p_result = NZOR(P_OuterSg, P_OwnSg);
	assert(p_result);
	return *p_result;
}

const SStrGroup & PPSecretSegment::GetSgC() const
{
	SStrGroup * p_result = NZOR(P_OuterSg, P_OwnSg);
	assert(p_result);
	return *p_result;
}

SJson * PPSecretSegment::ToJsonObj() const
{
	SJson * p_result = SJson::CreateObj();
	if(p_result) {
		SString temp_buf;
		if(InternalID)
			p_result->InsertUInt("InternalID", InternalID);
		if(ParentID)
			p_result->InsertUInt("ParentID", ParentID);
		if(SecType)
			p_result->InsertUInt("SecType", SecType);
		if(UedRefOid)
			p_result->InsertUInt64("UedRefOid", UedRefOid);
		if(UedEnterTm)
			p_result->InsertUInt64("UedEnterTm", UedEnterTm);
		if(CE.UedBeforeTm)
			p_result->InsertUInt64("UedExpiryTm", CE.UedBeforeTm);
		if(UedHashAlg)
			p_result->InsertUInt64("UedHashAlg", UedHashAlg);
		{
			struct SecretSegment_TextFldMapEntry_Out {
				const  char * P_Symb;
				uint32 Var;
			};
			{
				/*non-static*/const SecretSegment_TextFldMapEntry_Out tfm[] = {
					{ "Name", NameP },
					{ "Descr", DescrP },
					{ "ExecCmdLine", ExecCmdLineP},
					{ "STextOpen", CE.STextOpenP },
					{ "STextHidden", CE.STextHiddenP },
					{ "STextExpiry", CE.STextExpiryP },
					{ "STextExt1", CE.STextExt1P },
					{ "STextExt2", CE.STextExt2P },
					{ "STextExt3", CE.STextExt3P },
				};
				for(uint i = 0; i < SIZEOFARRAY(tfm); i++) {
					const  SecretSegment_TextFldMapEntry_Out & r_tfmi = tfm[i];
					if(GetText(r_tfmi.Var, temp_buf)) {
						temp_buf.Escape();
						p_result->InsertStringNe(r_tfmi.P_Symb, temp_buf);
					}
				}
			}
			if(History.getCount()) {
				SJson * p_js_hist = SJson::CreateArr();
				if(p_js_hist) {
					for(uint hi = 0; hi < History.getCount(); hi++) {
						const CoreEntry & r_he = History.at(hi);
						if(!r_he.IsEmpty()) {
							SJson * p_js_hi = SJson::CreateObj();
							if(p_js_hi) {
								/*non-static*/const SecretSegment_TextFldMapEntry_Out tfm[] = {
									{ "STextOpen", r_he.STextOpenP },
									{ "STextHidden", r_he.STextHiddenP },
									{ "STextExpiry", r_he.STextExpiryP },
									{ "STextExt1", r_he.STextExt1P },
									{ "STextExt2", r_he.STextExt2P },
									{ "STextExt3", r_he.STextExt3P },
								};
								if(r_he.UedBeforeTm)
									p_js_hi->InsertUInt64("UedExpiryTm", r_he.UedBeforeTm);
								for(uint i = 0; i < SIZEOFARRAY(tfm); i++) {
									const  SecretSegment_TextFldMapEntry_Out & r_tfmi = tfm[i];
									if(GetText(r_tfmi.Var, temp_buf)) {
										temp_buf.Escape();
										p_js_hi->InsertStringNe(r_tfmi.P_Symb, temp_buf);
									}
								}
								p_js_hist->InsertChild(p_js_hi);
							}
						}
					}
					p_result->Insert("history", p_js_hist);
				}
			}
		}
	}
	return p_result;
}
	
bool PPSecretSegment::FromJsonObj(const SJson * pJs)
{
	Z();
	if(SJson::IsObject(pJs)) {
		SString temp_buf;
		struct SecretSegment_TextFldMapEntry_In {
			const  char * P_Symb;
			uint32 * P_Var;
		};
		SecretSegment_TextFldMapEntry_In tfm[] = {
			{ "Name", &NameP },
			{ "Descr", &DescrP },
			{ "ExecCmdLine", &ExecCmdLineP},
			{ "STextOpen", &CE.STextOpenP },
			{ "STextHidden", &CE.STextHiddenP },
			{ "STextExpiry", &CE.STextExpiryP },
			{ "STextExt1", &CE.STextExt1P },
			{ "STextExt2", &CE.STextExt2P },
			{ "STextExt3", &CE.STextExt3P },
		};
		SStrGroup & r_sg = GetSg();
		enum {
			occfEnterTm  = 0x0001,
			occfExpiryTm = 0x0002,
		};
		uint   occur_flags = 0;
		for(const SJson * p_cur = pJs->P_Child; p_cur; p_cur = p_cur->P_Next) {
			bool   local_done = false;
			{
				for(uint i = 0; !local_done && i < SIZEOFARRAY(tfm); i++) {
					const  SecretSegment_TextFldMapEntry_In & r_tfmi = tfm[i];
					if(p_cur->Text.IsEqiAscii(r_tfmi.P_Symb)) {
						(temp_buf = p_cur->P_Child->Text).Unescape();
						r_sg.AddS(temp_buf, r_tfmi.P_Var);
						local_done = true;
					}
				}
			}
			if(!local_done) {
				if(p_cur->Text.IsEqiAscii("InternalID")) {
					InternalID = p_cur->P_Child->Text.ToULong();
				}
				else if(p_cur->Text.IsEqiAscii("ParentID")) {
					ParentID = p_cur->P_Child->Text.ToULong();
				}
				else if(p_cur->Text.IsEqiAscii("SecType")) {
					if(p_cur->P_Child->Text.IsDec()) {
						SecType = p_cur->P_Child->Text.ToULong();
					}
					else {
						int   st = SIntToSymbTab_GetId(SecSegTypeList, SIZEOFARRAY(SecSegTypeList), p_cur->P_Child->Text);
						if(st) {
							SecType = st;
						}
						else {
							// @todo @err
						}
					}
				}
				else if(p_cur->Text.IsEqiAscii("UedRefOid")) {
					UedRefOid = p_cur->P_Child->Text.ToUInt64();
				}
				else if(p_cur->Text.IsEqiAscii("UedEnterTm")) {
					UedEnterTm = p_cur->P_Child->Text.ToUInt64();
					occur_flags |= occfEnterTm;
				}
				else if(p_cur->Text.IsEqiAscii("UedEnter")) {
					(temp_buf = p_cur->P_Child->Text).Unescape();
					LDATETIME dtm;
					if(strtodatetime(temp_buf, dtm, DATF_ISO8601CENT, 0)) {
						SUniTime_Internal uti(dtm);
						UedEnterTm = UED::_SetRaw_Time(UED_META_TIME_MSEC, uti);
					}
					occur_flags |= occfEnterTm;
				}
				else if(p_cur->Text.IsEqiAscii("UedExpiryTm")) {
					CE.UedBeforeTm = p_cur->P_Child->Text.ToUInt64();
					occur_flags |= occfExpiryTm;
				}
				else if(p_cur->Text.IsEqiAscii("UedExpiry")) {
					(temp_buf = p_cur->P_Child->Text).Unescape();
					LDATETIME dtm;
					if(strtodatetime(temp_buf, dtm, DATF_ISO8601CENT, 0)) {
						SUniTime_Internal uti(dtm);
						CE.UedBeforeTm = UED::_SetRaw_Time(UED_META_TIME_MSEC, uti);
					}
					occur_flags |= occfExpiryTm;
				}
				else if(p_cur->Text.IsEqiAscii("UedHashAlg")) {
					UedHashAlg = p_cur->P_Child->Text.ToUInt64();
				}
				else if(p_cur->Text.IsEqiAscii("history")) {
					if(SJson::IsArray(p_cur->P_Child)) {
						for(const SJson * p_js_hi = p_cur->P_Child->P_Child; p_js_hi; p_js_hi = p_js_hi->P_Next) {
							if(SJson::IsObject(p_js_hi)) {
								uint   hist_occur_flags = 0;
								for(const SJson * p_js_f = p_js_hi->P_Child; p_js_f; p_js_f = p_js_f->P_Next) {
									if(p_js_f->P_Child) {
										CoreEntry hi;
										if(p_js_f->Text.IsEqiAscii("UedExpiryTm")) {
											hi.UedBeforeTm = p_js_f->P_Child->Text.ToUInt64();
											hist_occur_flags |= occfExpiryTm;
										}
										else if(p_js_f->Text.IsEqiAscii("ExpiryTm")) {
											(temp_buf = p_js_f->P_Child->Text).Unescape();
											LDATETIME dtm;
											if(strtodatetime(temp_buf, dtm, DATF_ISO8601CENT, 0)) {
												SUniTime_Internal uti(dtm);
												hi.UedBeforeTm = UED::_SetRaw_Time(UED_META_TIME_MSEC, uti);
											}
											hist_occur_flags |= occfExpiryTm;
										}
										else {
											SecretSegment_TextFldMapEntry_In htfm[] = {
												{ "STextOpen", &hi.STextOpenP },
												{ "STextHidden", &hi.STextHiddenP },
												{ "STextExpiry", &hi.STextExpiryP },
												{ "STextExt1", &hi.STextExt1P },
												{ "STextExt2", &hi.STextExt2P },
												{ "STextExt3", &hi.STextExt3P },
											};
											for(uint i = 0; i < SIZEOFARRAY(htfm); i++) {
												const  SecretSegment_TextFldMapEntry_In & r_tfmi = htfm[i];
												if(p_js_f->Text.IsEqiAscii(r_tfmi.P_Symb)) {
													(temp_buf = p_js_f->P_Child->Text).Unescape();
													r_sg.AddS(temp_buf, r_tfmi.P_Var);
													break;
												}
											}
										}
										if(!hi.IsEmpty()) {
											History.insert(&hi);
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return !IsEmpty();
}

static constexpr uint64 PPSecretSegmentPool_Signature = 0xA0C7E0A913D9C152ULL;

PPSecretSegmentPool::PPSecretSegmentPool() : SSignaturePrefix64(PPSecretSegmentPool_Signature), P_Vault(0)
{
}

PPSecretSegmentPool::~PPSecretSegmentPool()
{
	ZDELETE(P_Vault);
}

bool PPSecretSegmentPool::IsConsistent() const { return (this && this->SSignaturePrefix64::CheckSignature(PPSecretSegmentPool_Signature)); }

PPSecretSegmentPool & PPSecretSegmentPool::Z()
{
	if(IsConsistent()) {
		ZDELETE(P_Vault);
		freeAll();
		Sg.DestroySecureS();
	}
	return *this;
}

bool PPSecretSegmentPool::IsInWork() const { return (P_Vault && P_Vault->GetKeyRef()); }

bool FASTCALL PPSecretSegmentPool::IsEq(const PPSecretSegmentPool & rS) const
{
	return TSCollection_IsEq(this, &rS);
}

bool FASTCALL PPSecretSegmentPool::HasChildrenByID(uint32 id) const
{
	bool   result = false;
	if(id) {
		for(uint i = 0; !result && i < getCount(); i++) {
			const  PPSecretSegment * p_seg = at(i);
			if(p_seg && p_seg->ParentID == id) {
				result = true;
			}
		}
	}
	return result;
}

bool PPSecretSegmentPool::PutText(const char * pText, uint * pTextP)
{
	return LOGIC(Sg.AddS(pText, pTextP));
}

bool PPSecretSegmentPool::GetText(uint textP, SString & rBuf) const
{
	return Sg.GetS(textP, rBuf);
}

int PPSecretSegmentPool::DescryptSegments(const char * pMasterPassword, size_t masterPasswordLen)
{
	freeAll();
	int    ok = -1;
	SString temp_buf;
	SJson * p_js = 0;
	THROW(P_Vault); // @todo @err
	THROW_SL(P_Vault->CheckInPrimaryPassword(pMasterPassword, masterPasswordLen));
	THROW(P_Vault->GetKeyRef()); // @todo @err
	{
		const  uint64 key_ref = P_Vault->GetKeyRef();
		uint32 vp_id = 0;
		SBinaryChunk bc;
		for(size_t vp = 0; P_Vault->Enum(&vp, &vp_id, 0);) {
			const  uint32 outer_id = P_Vault->MakeOuterId(vp_id);
			if(outer_id) {
				const  int re = P_Vault->GetEncrypted(vp_id, reinterpret_cast<void *>(key_ref), &bc);
				if(re) {
					temp_buf.Z().CatN(static_cast<const char *>(bc.PtrC()), bc.Len());
					uint   new_seg_pos = 0;
					PPSecretSegment * p_new_seg = CreateNewSegment(&new_seg_pos);
					THROW(p_new_seg);
					{
						p_js = SJson::Parse(temp_buf);
						THROW(p_js);
						const  int fjr = p_new_seg->FromJsonObj(p_js);
						THROW(fjr);
						p_js->Destroy(true);
						ZDELETE(p_js);
						p_new_seg->InternalID = outer_id;
						ok = 1;
					}
				}
			}
		}
	}
	CATCH
		freeAll();
		ok = 0;
	ENDCATCH
	if(p_js) {
		p_js->Destroy(true);
		ZDELETE(p_js);
	}
	temp_buf.Obfuscate();
	temp_buf.Z();
	return ok;
}

int PPSecretSegmentPool::LoadStorage(const char * pFileName, const char * pMasterPassword, size_t masterPasswordLen)
{
	int   ok = 1;
	freeAll();
	ZDELETE(P_Vault);
	THROW(fileExists(pFileName));
	{
		SSerializeContext sctx;
		SBuffer sbuf;
		SFile f(pFileName, SFile::mRead|SFile::mBinary|SFile::mNoStd);
		THROW(f.IsValid());
		{
			uint64 signature = 0;
			uint64 data_size = 0;
			THROW_SL(f.Read(&signature, sizeof(signature)));
			THROW(signature == PPSecretSegmentPool_Signature); // @todo @err
			THROW_SL(f.Read(&data_size, sizeof(data_size)));
			if(data_size) {
				STempBuffer temp_buf(SMEGABYTE(1));
				uint64 read_size = 0;
				THROW_SL(temp_buf.IsValid());
				while(read_size < data_size) {
					size_t actual_size = 0;
					THROW_SL(f.Read(temp_buf, temp_buf.GetSize(), &actual_size));
					THROW_SL(sbuf.Write(temp_buf, actual_size));
					read_size += actual_size;
				}
				THROW(P_Vault = new SVaultPool());
				THROW_SL(P_Vault->Serialize(-1, sbuf, &sctx));
				if(pMasterPassword) {
					THROW(DescryptSegments(pMasterPassword, masterPasswordLen));
				}
			}
		}
	}
	CATCH
		ZDELETE(P_Vault);
		ok = 0;
	ENDCATCH
	return ok;
}

PPSecretSegment * PPSecretSegmentPool::CreateNewSegment(uint * pPos)
{
	uint   pos = 0;
	PPSecretSegment * p_result = new PPSecretSegment(&Sg);
	if(p_result) {
		uint   max_id = 0;
		for(uint i = 0; i < getCount(); i++) {
			const  PPSecretSegment * p_item = at(i);
			if(p_item) {
				SETMAX(max_id, p_item->InternalID);
			}
		}
		p_result->InternalID = (max_id > 0) ? (max_id+1) : 1;
		{
			SUniTime_Internal ut;
			ut.SetCurrent();
			p_result->UedEnterTm = UED::_SetRaw_Time(UED_META_TIME_MSEC, ut);
		}
		if(insert(p_result)) {
			assert(getCount());
			pos = getCount() - 1;
		}
		else {
			ZDELETE(p_result);
		}
	}
	ASSIGN_PTR(pPos, pos);
	return p_result;
}

const PPSecretSegment * PPSecretSegmentPool::SearchSegmentByID(uint id, uint * pPos) const
{
	const  PPSecretSegment * p_result = 0;
	uint   pos = 0;
	if(id) {
		for(uint i = 0; !p_result && i < getCount(); i++) {
			const  PPSecretSegment * p_item = at(i);
			if(p_item && p_item->InternalID == id) {
				p_result = p_item;
				pos = i;
			}
		}
	}
	ASSIGN_PTR(pPos, pos);
	return p_result;
}

PPSecretSegment * PPSecretSegmentPool::SearchSegmentByName(const char * pKey, uint * pPos)
{
	PPSecretSegment * p_result = 0;
	uint   pos = 0;
	if(!isempty(pKey)) {
		SString temp_buf;
		for(uint i = 0; !p_result && i < getCount(); i++) {
			PPSecretSegment * p_item = at(i);
			if(p_item && GetText(p_item->NameP, temp_buf) && temp_buf.IsEqiUtf8(pKey)) {
				p_result = p_item;
				pos = i;
			}
		}
	}
	return p_result;
}

int PPSecretSegmentPool::IsThereStorage(const char * pFileName)
{
	int    ok = -1;
	THROW(!isempty(pFileName));
	if(fileExists(pFileName)) {
		SFile f(pFileName, SFile::mRead|SFile::mBinary|SFile::mNoStd);
		uint64 signature = 0;
		if(f.IsValid() && f.Read(&signature, sizeof(signature)) && signature == PPSecretSegmentPool_Signature) {
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

void PPSecretSegmentPool::GetChildren(uint32 id, bool recursive, LongArray & rChildrenList) const
{
	if(id) {
		for(uint i = 0; i < getCount(); i++) {
			const  PPSecretSegment * p_seg = at(i);
			if(p_seg && p_seg->ParentID == id) {
				rChildrenList.add(p_seg->InternalID);
				if(recursive) {
					GetChildren(p_seg->InternalID, recursive, rChildrenList); // @recursion
				}
			}
		}
	}
}

void PPSecretSegmentPool::GetHierarchyByID(uint32 id, LongArray & rHierarchyList) const
{
	const  PPSecretSegment * p_seg = SearchSegmentByID(id, 0);
	if(p_seg) {
		rHierarchyList.add(p_seg->InternalID);
		if(p_seg->ParentID) {
			GetHierarchyByID(p_seg->ParentID, rHierarchyList); // @recursion
		}
	}
}

int PPSecretSegmentPool::Helper_MakeFolderList(uint32 parentId, uint currentSegmentIdent, StrAssocArray & rList) const
{
	int   ok = 1;
	SString temp_buf;
	LongArray children_of_current;
	if(currentSegmentIdent) {
		GetChildren(currentSegmentIdent, true, children_of_current);
		children_of_current.add(currentSegmentIdent);
		children_of_current.sortAndUndup();
	}
	for(uint i = 0; i < getCount(); i++) {
		const  PPSecretSegment * p_seg = at(i);
		if(p_seg && p_seg->SecType == PPSecretSegment::sectypFolder && p_seg->ParentID == parentId) {
			if(!children_of_current.bsearch(p_seg->InternalID)) {
				GetText(p_seg->NameP, temp_buf);
				if(temp_buf.IsEmpty()) {
					temp_buf.Cat("Entry").Space().CatChar('#').Cat(p_seg->InternalID);
				}
				rList.Add(p_seg->InternalID, p_seg->ParentID, temp_buf);
				if(p_seg->SecType == PPSecretSegment::sectypFolder) {
					THROW(Helper_MakeFolderList(p_seg->InternalID, currentSegmentIdent, rList)); // @recursion
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPSecretSegmentPool::MakeFolderList(uint currentSegmentIdent, StrAssocArray & rList) const
{
	rList.Z();
	return Helper_MakeFolderList(0, currentSegmentIdent, rList);
}

int PPSecretSegmentPool::CreateStorage(const char * pFileName, const char * pMasterPassword, size_t masterPasswordLen)
{
	ZDELETE(P_Vault);
	int   ok = 1;
	bool  is_file_created = false;
	THROW(!isempty(pFileName));
	THROW(!fileExists(pFileName));
	{
		SFile f(pFileName, SFile::mWrite|SFile::mBinary|SFile::mNoStd);
		THROW(f.IsValid());
		is_file_created = true;
		{
			THROW_SL(P_Vault = new SVaultPool());
			THROW(P_Vault->SetupPrimaryPassword(pMasterPassword, masterPasswordLen));
		}
		{
			SSerializeContext sctx;
			SBuffer sbuf;
			THROW_SL(P_Vault->Serialize(+1, sbuf, &sctx));
			THROW_SL(f.Write(&PPSecretSegmentPool_Signature, sizeof(PPSecretSegmentPool_Signature)));
			const  uint64 data_size = sbuf.GetAvailableSize();
			THROW_SL(f.Write(&data_size, sizeof(data_size)));
			THROW_SL(f.Write(sbuf.GetBufC(), static_cast<size_t>(data_size)));
		}
	}
	CATCH
		if(is_file_created) {
			SFile::Remove(pFileName);
		}
		ZDELETE(P_Vault);
		ok = 0;
	ENDCATCH
	return ok;
}

int PPSecretSegmentPool::SaveStorage(const char * pFileName)
{
	int    ok = 1;
	bool   is_file_created = false;
	if(!P_Vault) {
		ok = -1;
	}
	else {
		THROW(!isempty(pFileName));
		if(fileExists(pFileName)) {
			/*
			SFile::Stat fst;
			if(SFile::GetStat(pFileName, 0, &fst, 0) && fst.Size > 0) {
				SFsPath ps(pFileName);
				ps.Ext.DotCat("bak");
				SString bak_file_path;
				ps.Merge(bak_file_path);
				SCopyFile(pFileName, bak_file_path, 0, FILE_SHARE_READ, 0);
			}
			*/
		}
		{
			SFile f(pFileName, SFile::mWrite|SFile::mBinary|SFile::mNoStd);
			THROW(f.IsValid());
			{
				THROW_SL(P_Vault->RemoveNonSystemItems());
				for(uint segi = 0; segi < getCount(); segi++) {
					const PPSecretSegment * p_seg = at(segi);
					if(p_seg) {
						THROW(PutSegmentIntoPool(p_seg));
					}
				}
			}
			{
				SSerializeContext sctx;
				SBuffer sbuf;
				THROW_SL(P_Vault->Serialize(+1, sbuf, &sctx));
				THROW_SL(f.Write(&PPSecretSegmentPool_Signature, sizeof(PPSecretSegmentPool_Signature)));
				const  uint64 data_size = sbuf.GetAvailableSize();
				THROW_SL(f.Write(&data_size, sizeof(data_size)));
				THROW_SL(f.Write(sbuf.GetBufC(), static_cast<size_t>(data_size)));
			}
		}
	}
	CATCH
		ok = 0;
	ENDCATCH
	return ok;
}

int PPSecretSegmentPool::PutSegmentIntoPool(const PPSecretSegment * pSeg)
{
	int    ok = -1;
	SJson * p_js = 0;
	if(pSeg && !pSeg->IsEmpty()) {
		SString temp_buf;
		THROW(P_Vault); // @todo @errr
		THROW(P_Vault->GetKeyRef()); // @todo @errr
		THROW(p_js = pSeg->ToJsonObj());
		p_js->ToStr(temp_buf);
		{
			uint32   _id = SVaultPool::MakeInternalId(pSeg->InternalID);
			SBinarySet::DeflateStrategy ds(128);
			int    r = P_Vault->PutEncrypted(_id, temp_buf.cptr(), temp_buf.Len(), P_Vault->GetUedSymmCipher(), reinterpret_cast<void *>(P_Vault->GetKeyRef()), &ds);
			THROW(r);
			ok = 1;
		}
	}
	CATCHZOK
	delete p_js;
	return ok;
}

int PPSecretSegmentPool::ReadTestJson(const char * pJsFileName)
{
	/*
		{
		  "version": 2,
		  "generated_at": "2026-06-16T18:28:27",
		  "total_segments": 1500,
		  "note": "Folders (SecType=1) come first in hierarchical order, then secrets in random order. All ParentID values reference existing folder InternalIDs or 0 (root).",
		  "segments": [
			{
			  "InternalID": 1,
			  "ParentID": 0,
			  "SecType": 1,
			  "Name": "Работа",
			  "EnterTm": "2022-08-27T13:35:25"
			},
			{
			  "InternalID": 6,
			  "ParentID": 0,
			  "SecType": 1,
			  "Name": "Документы",
			  "EnterTm": "2026-02-04T12:47:13"
			},
	*/ 
	int    ok = 0;
	SJson * p_js = SJson::ParseFile(pJsFileName);
	if(SJson::IsObject(p_js)) {
		LongArray cidlist;
		for(const SJson * p_cur = p_js->P_Child; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Text.IsEqiAscii("segments")) {
				if(SJson::IsArray(p_cur->P_Child)) {
					for(const SJson * p_js_item = p_cur->P_Child->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
						if(SJson::IsObject(p_js_item)) {
							uint   new_seg_pos = 0;
							PPSecretSegment * p_new_seg = CreateNewSegment(&new_seg_pos);
							if(p_new_seg) {
								if(p_new_seg->FromJsonObj(p_js_item)) {
									if(P_Vault) {
										if(!p_new_seg->InternalID) {
											uint32 new_id = 0;
											THROW_SL(P_Vault->GetChunkIdList(cidlist));
											if(cidlist.getCount()) {
												cidlist.sortAndUndup();
												const  long max_internal_id = smax(cidlist.getLast(), static_cast<long>(SVaultPool::GetMinInternalId()-1));
												new_id = SVaultPool::MakeOuterId(max_internal_id+1);
											}
											else {
												new_id = SVaultPool::MakeOuterId(SVaultPool::GetMinInternalId());
											}
											THROW(new_id); // @todo @err
											p_new_seg->InternalID = new_id;
										}
										THROW(PutSegmentIntoPool(p_new_seg));
									}
									ok = 1;
								}
								else {
									atFree(new_seg_pos);
								}
							}
						}
					}
				}
			}
		}
	}
	CATCHZOK
	delete p_js;
	return ok;
}

int PPSecretSegmentPool::Pack()
{
	int    ok = -1;
	if(Sg.GetPoolDataLen()) {
		void * p_pack_handle = Sg.Pack_Start();
		if(p_pack_handle) {
			const uint c = getCount();
			for(uint i = 0; i < c; i++) {
				PPSecretSegment * p_item = at(i);
				if(p_item) {
					Sg.Pack_Replace(p_pack_handle, p_item->NameP);
					Sg.Pack_Replace(p_pack_handle, p_item->DescrP);
					Sg.Pack_Replace(p_pack_handle, p_item->ExecCmdLineP);
					Sg.Pack_Replace(p_pack_handle, p_item->CE.STextOpenP);
					Sg.Pack_Replace(p_pack_handle, p_item->CE.STextHiddenP);
					Sg.Pack_Replace(p_pack_handle, p_item->CE.STextExpiryP);
					Sg.Pack_Replace(p_pack_handle, p_item->CE.STextExt1P);
					Sg.Pack_Replace(p_pack_handle, p_item->CE.STextExt2P);
					Sg.Pack_Replace(p_pack_handle, p_item->CE.STextExt3P);
					for(uint j = 0; j < p_item->History.getCount(); j++) {
						PPSecretSegment::CoreEntry & r_ce = p_item->History.at(j);
						Sg.Pack_Replace(p_pack_handle, r_ce.STextOpenP);
						Sg.Pack_Replace(p_pack_handle, r_ce.STextHiddenP);
						Sg.Pack_Replace(p_pack_handle, r_ce.STextExpiryP);
						Sg.Pack_Replace(p_pack_handle, r_ce.STextExt1P);
						Sg.Pack_Replace(p_pack_handle, r_ce.STextExt2P);
						Sg.Pack_Replace(p_pack_handle, r_ce.STextExt3P);
					}
				}
			}
			Sg.Pack_Finish(p_pack_handle);
			ok = 1;
		}
		else
			ok = 0;
	}
	return ok;
}

int TFacadeWindow::GetSecretsFilePath(SString & rBuf)
{
	rBuf.Z();
	int    ok = 0;
	//PPGetFilePath(PPPATH_DAT)
	DBS.GetDbPath(DBS.GetDbPathID(), rBuf);
	if(rBuf.NotEmpty()) {
		rBuf.SetLastSlash().Cat("sesf").DotCat("bin");
		ok = 1;
	}
	return ok;
}

static const char * P_PasswordStorageCredentialSymbol = "sobolev.centrigo.sm.01";

int TFacadeWindow::LoadSecrets(SString * pFilePath, bool interactive)
{
	int    ok = -1;
	SString file_path;
	if(GetSecretsFilePath(file_path) > 0) {
		char    _password[128];
		_password[0] = 0;
		if(fileExists(file_path)) {
			bool   done = false;
			PasswordDialogParam param;
			param.MinLen = 0;
			param.Flags |= (PasswordDialogParam::fWithoutEncrypt|PasswordDialogParam::fNoConfirmation);
			param.StorePasswordIdent = P_PasswordStorageCredentialSymbol;
			{
				SSystemCredential scp;
				scp.Persist = SSystemCredential::persistMachine;
				scp.TargetNameUtf8 = P_PasswordStorageCredentialSymbol;
				SBinaryChunk bc;
				const  int r = SSystemCredentialRead(scp, bc);
				if(r > 0) {
					SUniTime_Internal ut;
					SUniTime_Internal now_ut;
					bool   is_expired = false;
					if(UED::_GetRaw_Time(scp.UedModTime, ut) && now_ut.SetCurrent()) {
						int   diff_days = 0;
						int   diff_result = 0;
						if(now_ut.Difference(ut, SUOM_SECOND, &diff_days, &diff_result)) {
							if(abs(diff_days) > 7) {
								is_expired = true;
							}
						}
					}
					if(!is_expired && SecPool.LoadStorage(file_path, static_cast<const char *>(bc.PtrC()), bc.Len())) {
						done = true;
						ok = 1;
					}
				}
				bc.DestroySecure();
			}
			if(!done) {
				const   int pwdr = PasswordDialog2(DLG_SECRETPOOLPASSWORD, _password, sizeof(_password), param);
				if(pwdr > 0) {
					if(isempty(_password)) {
						PPError(PPERR_NONEMPTYPASSWORDNEEDED);
						ok = 0;
					}
					else if(SecPool.LoadStorage(file_path, _password, sstrlen(_password))) {
						if(pwdr == 101) {
							SSystemCredential scp;
							scp.Persist = SSystemCredential::persistMachine;
							scp.TargetNameUtf8 = P_PasswordStorageCredentialSymbol;
							bool r = SSystemCredentialWrite(scp, _password, sstrlen(_password));
						}
						ok = 1;
					}
					else {
						ok = PPErrorZ();
					}
				}
			}
		}
		else if(interactive) {
			// Создание нового хранилища
			PasswordDialogParam param;
			param.MinLen = 0;
			param.Flags |= PasswordDialogParam::fWithoutEncrypt;
			if(PasswordDialog2(DLG_SECRETPOOLPASSWORD, _password, sizeof(_password), param) > 0) {
				if(isempty(_password)) {
					PPError(PPERR_NONEMPTYPASSWORDNEEDED);
					ok = 0;
				}
				else if(SecPool.CreateStorage(file_path, _password, sstrlen(_password))) {
					ok = 2;
				}
				else {
					ok = PPErrorZ();
				}
			}
		}
	}
	ASSIGN_PTR(pFilePath, file_path);
	return ok;
}

int TFacadeWindow::CloseSecrets()
{
	int    ok = -1;
	return ok;
}

CentrigoSecretsDialog::CentrigoSecretsDialog(void * hParentWindow, PPSecretSegmentPool & rSecPool, const SString & rFilePath) : 
	TDialog(DLG_SECRETPOOL, hParentWindow), R_SecPool(rSecPool), CurrentSegIdent(0), FilePath(rFilePath)
{
	P_Box = static_cast<SmartListBox *>(getCtrlView(CTL_SECRETPOOL_LIST));
	if(!SetupStrListBox(P_Box))
		PPError();
	updateList(-1);
	{
		//SetupStringCombo(this, CTLSEL_SECRETPOOL_TYPE, PPTXT_SECSEGTYPES, 0);
		//int STDCALL SetupStringComboWithAllowedList(TDialog * dlg, uint ctlID, const char * pStrSignature, const LongArray * pAllowedList, long initID) // @v12.6.11
	}
	{
		const uint input_ctl_fld_id_list[] = {
			CTL_SECRETPOOL_NAME, CTL_SECRETPOOL_DESCR, CTL_SECRETPOOL_TOPEN, CTL_SECRETPOOL_THIDDEN,
			CTL_SECRETPOOL_TEXPIRY, CTL_SECRETPOOL_TEXT1, CTL_SECRETPOOL_TEXT2, CTL_SECRETPOOL_TEXT3
		};
		for(uint i = 0; i < SIZEOFARRAY(input_ctl_fld_id_list); i++) {
			TView * p_view = getCtrlView(input_ctl_fld_id_list[i]);
			if(TView::IsSubSign(p_view, TV_SUBSIGN_INPUTLINE)) {
				p_view->ViewOptions |= ofUtf8;
			}
		}
	}
}

CentrigoSecretsDialog::~CentrigoSecretsDialog()
{
	bool    debug_mark = false;
	if(R_SecPool.IsConsistent()) {
		/*if(FilePath.NotEmpty()) {
			if(!R_SecPool.SaveStorage(FilePath))
				PPError();
		}*/
		R_SecPool.Z();
	}
	else {
		debug_mark = true;
	}
}

bool CentrigoSecretsDialog::MakeNewSegName(SString & rBuf) const
{
	long   _counter = 1;
	SString name_template("New secret");
	rBuf.Z().Cat(name_template);
	while(R_SecPool.SearchSegmentByName(rBuf, 0)) {
		rBuf.Z().Cat(name_template).Space().CatChar('#').CatLongZ(++_counter, 3);
	}
	return true;
}

bool CentrigoSecretsDialog::MakeNewFolderName(SString & rBuf) const
{
	long   _counter = 1;
	SString name_template("New folder");
	rBuf.Z().Cat(name_template);
	while(R_SecPool.SearchSegmentByName(rBuf, 0)) {
		rBuf.Z().Cat(name_template).Space().CatChar('#').CatLongZ(++_counter, 3);
	}
	return true;
}

/*virtual*/int CentrigoSecretsDialog::addItem(long * pPos, long * pID)
{
	int    ok = 0;
	uint   new_seg_pos = 0;
	PPSecretSegment * p_new_seg = R_SecPool.CreateNewSegment(&new_seg_pos);
	if(p_new_seg) {
		SString name_buf;
		MakeNewSegName(name_buf);
		R_SecPool.PutText(name_buf, &p_new_seg->NameP);
		p_new_seg->SecType = PPSecretSegment::sectypGeneric;
		{
			if(FilePath.NotEmpty()) {
				if(!R_SecPool.SaveStorage(FilePath))
					PPError();
			}
		}
		ASSIGN_PTR(pPos, static_cast<long>(new_seg_pos));
		ASSIGN_PTR(pID, static_cast<long>(p_new_seg->InternalID));
		ok = 1;
	}
	return ok;
}

/*virtual*/int CentrigoSecretsDialog::editItem(long pos, long id)
{
	int    ok = -1;
	return ok;
}

/*virtual*/int CentrigoSecretsDialog::delItem(long pos, long id)
{
	int    ok = -1;
	uint   item_idx = 0;
	const  PPSecretSegment * p_item = R_SecPool.SearchSegmentByID(id, &item_idx);
	if(p_item) {
		SString temp_buf;
		if(R_SecPool.HasChildrenByID(id)) {
			//PPERR_SECSEGHASCHLDRN_UNABLEDEL     "Невозможно удалить '%s'. Существуют дочерние элементы"
			R_SecPool.GetText(p_item->NameP, temp_buf);
			ok = PPSetError(PPERR_SECSEGHASCHLDRN_UNABLEDEL, temp_buf);
		}
		else {
			R_SecPool.atFree(item_idx);
			{
				if(FilePath.NotEmpty()) {
					if(!R_SecPool.SaveStorage(FilePath))
						ok = 0;
				}
			}
			if(ok < 0)
				ok = 1;
		}
	}
	if(!ok)
		PPError();
	return ok;
}

void CentrigoSecretsDialog::ClearInputBlock()
{
}

void CentrigoSecretsDialog::SetupParentList(uint currentIdent)
{
	StrAssocArray list;
	const  PPSecretSegment * p_current_item = R_SecPool.SearchSegmentByID(currentIdent, 0);
	R_SecPool.MakeFolderList(currentIdent, list);
	SetupStrAssocTreeCombo(this, CTLSEL_SECRETPOOL_PARENT, list, p_current_item ? p_current_item->ParentID : 0, 0/*flags*/, 0/*ownerDrawListBox*/);
}
	
void CentrigoSecretsDialog::SetupSegmentType(uint type)
{
	/*
		struct SecretFieldLabels {
			const char* OpenLabel;
			const char* HiddenLabel;
			const char* ExpiryLabel;
			const char* Ext1Label;
			const char* Ext2Label;
			const char* Ext3Label;
		};

		static SecretFieldLabels GetFieldLabels(uint32 secType) 
		{
			switch (secType) {
				case sectypGeneric:
					return {"Открытая часть", "Скрытая часть", "Срок действия", "Дополнительно 1", "Дополнительно 2", "Дополнительно 3"};
				case sectypPassword:
					return {"Подсказка", "Пароль", "", "", "", ""};
				case sectypAuthSecret:
					return {"Логин / E-mail", "Пароль", "Срок действия пароля", "2FA / Секретный вопрос", "", ""};
				case sectypOpenKey:
					return {"Значение / Номер", "", "Срок действия документа", "", "", ""};
				case sectypBankCard:
					return {"Номер карты", "PIN-код", "Срок действия карты", "CVV/CVC-код", "", ""};
				case sectypSSH:
					return {"Публичный ключ", "Приватный ключ", "", "Парольная фраза (Passphrase)", "", ""};
				case sectypESignature:
					return {"Сертификат", "Закрытый ключ", "Срок действия сертификата", "Пароль к контейнеру", "", ""};
				default:
					return {"Открытая часть", "Скрытая часть", "Срок действия", "Дополнительно 1", "Дополнительно 2", "Дополнительно 3"};
			}
		}
	*/ 
	bool   enable_type_selection = true;
		/*
@secretpool_textopen     "Открытый текст"
@secretpool_texthidden   "Скрытый текст"
@secretpool_textexpiry   "Срок действия (текст)"
@secretpool_bcard_n      "Номер карты"
@secretpool_bcard_pin    "PIN-код"
@secretpool_bcard_cvv    "CVV/CVC-код"
@secretpool_bcard_expiry "Срок действия карты"
		*/ 
	SUiLayout * p_lo_core = FindLayoutBySymb("LO_CORE");
	switch(type) {
		case PPSecretSegment::sectypUndef:
			ResetChildLayoutExcludedStatus(p_lo_core);
			showCtrl(CTL_SECRETPOOL_EXPIRY, true);
			showCtrl(CTL_SECRETPOOL_TOPEN, true);
			setLabelText(CTL_SECRETPOOL_TOPEN, "@secretpool_textopen");
			showCtrl(CTL_SECRETPOOL_THIDDEN, true);
			setLabelText(CTL_SECRETPOOL_THIDDEN, "@secretpool_texthidden");
			showCtrl(CTL_SECRETPOOL_TEXPIRY, false);
			showCtrl(CTL_SECRETPOOL_TEXT1, false);
			showCtrl(CTL_SECRETPOOL_TEXT2, false);
			showCtrl(CTL_SECRETPOOL_TEXT3, false);
			break;
		case PPSecretSegment::sectypFolder:
			SetChildLayoutExcludedStatus(p_lo_core);
			showCtrl(CTL_SECRETPOOL_EXPIRY, false);
			showCtrl(CTL_SECRETPOOL_TOPEN, false);
			showCtrl(CTL_SECRETPOOL_THIDDEN, false);
			showCtrl(CTL_SECRETPOOL_TEXPIRY, false);
			showCtrl(CTL_SECRETPOOL_TEXT1, false);
			showCtrl(CTL_SECRETPOOL_TEXT2, false);
			showCtrl(CTL_SECRETPOOL_TEXT3, false);
			enable_type_selection = false;
			break;
		case PPSecretSegment::sectypGeneric:
			ResetChildLayoutExcludedStatus(p_lo_core);
			showCtrl(CTL_SECRETPOOL_EXPIRY, true);
			showCtrl(CTL_SECRETPOOL_TOPEN, true);
			setLabelText(CTL_SECRETPOOL_TOPEN, "@secretpool_textopen");
			showCtrl(CTL_SECRETPOOL_THIDDEN, true);
			setLabelText(CTL_SECRETPOOL_THIDDEN, "@secretpool_texthidden");
			showCtrl(CTL_SECRETPOOL_TEXPIRY, false);
			showCtrl(CTL_SECRETPOOL_TEXT1, false);
			showCtrl(CTL_SECRETPOOL_TEXT2, false);
			showCtrl(CTL_SECRETPOOL_TEXT3, false);
			break;
		case PPSecretSegment::sectypPassword:
			ResetChildLayoutExcludedStatus(p_lo_core);
			showCtrl(CTL_SECRETPOOL_EXPIRY, true);
			showCtrl(CTL_SECRETPOOL_TOPEN, false);
			showCtrl(CTL_SECRETPOOL_THIDDEN, true);
			setLabelText(CTL_SECRETPOOL_THIDDEN, "@secretpool_texthidden");
			showCtrl(CTL_SECRETPOOL_TEXPIRY, false);
			showCtrl(CTL_SECRETPOOL_TEXT1, false);
			showCtrl(CTL_SECRETPOOL_TEXT2, false);
			showCtrl(CTL_SECRETPOOL_TEXT3, false);
			break;
		case PPSecretSegment::sectypAuthSecret:
			ResetChildLayoutExcludedStatus(p_lo_core);
			showCtrl(CTL_SECRETPOOL_EXPIRY, true);
			showCtrl(CTL_SECRETPOOL_TOPEN, true);
			setLabelText(CTL_SECRETPOOL_TOPEN, "@secretpool_textopen");
			showCtrl(CTL_SECRETPOOL_THIDDEN, true);
			setLabelText(CTL_SECRETPOOL_THIDDEN, "@secretpool_texthidden");
			showCtrl(CTL_SECRETPOOL_TEXPIRY, false);
			showCtrl(CTL_SECRETPOOL_TEXT1, false);
			showCtrl(CTL_SECRETPOOL_TEXT2, false);
			showCtrl(CTL_SECRETPOOL_TEXT3, false);
			break;
		case PPSecretSegment::sectypOpenKey:
			ResetChildLayoutExcludedStatus(p_lo_core);
			showCtrl(CTL_SECRETPOOL_EXPIRY, true);
			showCtrl(CTL_SECRETPOOL_TOPEN, true);
			setLabelText(CTL_SECRETPOOL_TOPEN, "@secretpool_textopen");
			showCtrl(CTL_SECRETPOOL_THIDDEN, true);
			setLabelText(CTL_SECRETPOOL_THIDDEN, "@secretpool_texthidden");
			showCtrl(CTL_SECRETPOOL_TEXPIRY, false);
			showCtrl(CTL_SECRETPOOL_TEXT1, false);
			showCtrl(CTL_SECRETPOOL_TEXT2, false);
			showCtrl(CTL_SECRETPOOL_TEXT3, false);
			break;
		case PPSecretSegment::sectypBankCard:
			ResetChildLayoutExcludedStatus(p_lo_core);
			showCtrl(CTL_SECRETPOOL_EXPIRY, false);
			showCtrl(CTL_SECRETPOOL_TOPEN, true);
			setLabelText(CTL_SECRETPOOL_TOPEN, "@secretpool_bcard_n");
			showCtrl(CTL_SECRETPOOL_THIDDEN, true);
			setLabelText(CTL_SECRETPOOL_THIDDEN, "@secretpool_bcard_pin");
			showCtrl(CTL_SECRETPOOL_TEXPIRY, true);
			setLabelText(CTL_SECRETPOOL_TEXPIRY, "@secretpool_bcard_expiry");
			showCtrl(CTL_SECRETPOOL_TEXT1, true);
			setLabelText(CTL_SECRETPOOL_TEXT1, "@secretpool_bcard_cvv");
			showCtrl(CTL_SECRETPOOL_TEXT2, false);
			showCtrl(CTL_SECRETPOOL_TEXT3, false);
			break;
		case PPSecretSegment::sectypSSH:
			ResetChildLayoutExcludedStatus(p_lo_core);
			showCtrl(CTL_SECRETPOOL_EXPIRY, true);
			showCtrl(CTL_SECRETPOOL_TOPEN, true);
			setLabelText(CTL_SECRETPOOL_TOPEN, "@secretpool_ssh_pubkey");
			showCtrl(CTL_SECRETPOOL_THIDDEN, true);
			setLabelText(CTL_SECRETPOOL_THIDDEN, "@secretpool_ssh_privkey");
			showCtrl(CTL_SECRETPOOL_TEXPIRY, false);
			showCtrl(CTL_SECRETPOOL_TEXT1, true);
			setLabelText(CTL_SECRETPOOL_TEXT1, "@secretpool_ssh_passphrase");
			showCtrl(CTL_SECRETPOOL_TEXT2, false);
			showCtrl(CTL_SECRETPOOL_TEXT3, false);
			break;
		case PPSecretSegment::sectypESignature:
			ResetChildLayoutExcludedStatus(p_lo_core);
			showCtrl(CTL_SECRETPOOL_EXPIRY, true);
			showCtrl(CTL_SECRETPOOL_TOPEN, true);
			setLabelText(CTL_SECRETPOOL_TOPEN, "@secretpool_textopen");
			showCtrl(CTL_SECRETPOOL_THIDDEN, true);
			setLabelText(CTL_SECRETPOOL_THIDDEN, "@secretpool_texthidden");
			showCtrl(CTL_SECRETPOOL_TEXPIRY, false);
			showCtrl(CTL_SECRETPOOL_TEXT1, false);
			showCtrl(CTL_SECRETPOOL_TEXT2, false);
			showCtrl(CTL_SECRETPOOL_TEXT3, false);
			break;
		case PPSecretSegment::sectypPlainText:
			SetChildLayoutExcludedStatus(p_lo_core);
			showCtrl(CTL_SECRETPOOL_EXPIRY, false);
			showCtrl(CTL_SECRETPOOL_TOPEN, false);
			showCtrl(CTL_SECRETPOOL_THIDDEN, false);
			showCtrl(CTL_SECRETPOOL_TEXPIRY, false);
			showCtrl(CTL_SECRETPOOL_TEXT1, false);
			showCtrl(CTL_SECRETPOOL_TEXT2, false);
			showCtrl(CTL_SECRETPOOL_TEXT3, false);
			break;
	}
}

void CentrigoSecretsDialog::SetupSelectedSegment(uint segIdent)
{
	SString temp_buf;
	//if(segIdx < R_SecPool.getCount()) 
	{
		//const  PPSecretSegment * p_item = R_SecPool.at(segIdx);
		uint   seg_pos = 0;
		const  PPSecretSegment * p_item = R_SecPool.SearchSegmentByID(segIdent, &seg_pos);
		if(p_item) {
			//CurrentSegIdx = segIdx+1;
			CurrentSegIdent = segIdent;
			setCtrlLong(CTL_SECRETPOOL_ID, p_item->InternalID);
			setCtrlLong(CTLSEL_SECRETPOOL_TYPE, p_item->SecType);
			{
				LongArray allowed_type_list;
				if(p_item->SecType == PPSecretSegment::sectypFolder) {
					allowed_type_list.add(PPSecretSegment::sectypFolder);
					disableCtrl(CTLSEL_SECRETPOOL_TYPE, true);
				}
				else {
					allowed_type_list.addzlist(PPSecretSegment::sectypGeneric, PPSecretSegment::sectypPassword, PPSecretSegment::sectypAuthSecret,
						PPSecretSegment::sectypOpenKey, PPSecretSegment::sectypBankCard, PPSecretSegment::sectypSSH, 
						PPSecretSegment::sectypESignature, PPSecretSegment::sectypPlainText, 0L);
					disableCtrl(CTLSEL_SECRETPOOL_TYPE, false);
				}
				SetupStringComboWithAllowedList(this, CTLSEL_SECRETPOOL_TYPE, PPTXT_SECSEGTYPES, &allowed_type_list, p_item->SecType);
			}
			SetupParentList(segIdent);
			{
				SUniTime_Internal ut;
				UED::_GetRaw_Time(p_item->UedEnterTm, ut);
				setCtrlString(CTL_SECRETPOOL_CRTM, ut.ToStr(DATF_ISO8601CENT, TIMF_HMS, temp_buf));
			}
			{
				SUniTime_Internal ut;
				UED::_GetRaw_Time(p_item->CE.UedBeforeTm, ut);
				LDATE  dt = ZERODATE;
				ut.GetDate(&dt);
				setCtrlDate(CTL_SECRETPOOL_EXPIRY, dt);
			}
			// input CTL_SECRETPOOL_NAME [growfactor: 1 height: 21 margin: 4 tabstop label: "@appellation"] string[128];
			// input CTL_SECRETPOOL_ID [width: 60 height: 21 margin: 4 tabstop readonly fmtf: (nozero) label: "@id"] uint64;
			// combobox CTLSEL_SECRETPOOL_TYPE [width: bycontainer height: 21 margin: 4 tabstop label: "@type" cblinesymb: CTL_SECRETPOOL_TYPE];
			// combobox CTLSEL_SECRETPOOL_PARENT [width: bycontainer height: 21 margin: 4 tabstop label: "Parent" cblinesymb: CTL_SECRETPOOL_PARENT];
			// input CTL_SECRETPOOL_CRTM [width: 80 height: 21 margin: 4 tabstop readonly label: "Creation Time"] string[64];
			// input CTL_SECRETPOOL_EXPIRY [width: 80 height: 21 margin: 4 tabstop label: "Expiry"] date;
			// input CTL_SECRETPOOL_TOPEN [width: bycontainer height: 21 margin: 4 tabstop label: "Open Text"] string[128];
			// input CTL_SECRETPOOL_THIDDEN [width: bycontainer height: 21 margin: 4 tabstop label: "Hidden Text"] string[128];
			// input CTL_SECRETPOOL_TEXPIRY [growfactor: 1 height: 21 margin: 4 tabstop label: "Expiry Text"] string[128];
			// input CTL_SECRETPOOL_TEXT1 [growfactor: 1 height: 21 margin: 4 tabstop label: "Ext 1"] string[128];
			// input CTL_SECRETPOOL_TEXT2 [growfactor: 1 height: 21 margin: 4 tabstop label: "Ext 2"] string[128];
			// input CTL_SECRETPOOL_TEXT3 [growfactor: 1 height: 21 margin: 4 tabstop label: "Ext 3"] string[128];
			// input CTL_SECRETPOOL_DESCR [width: bycontainer growfactor: 1 margin: 4 tabstop multiline wantreturn label: "@memo"] string[252];
			{
				const TextFieldDescr tctl_tab[] = {
					{ CTL_SECRETPOOL_NAME, const_cast<uint32 *>(&p_item->NameP) },
					{ CTL_SECRETPOOL_TOPEN, const_cast<uint32 *>(&p_item->CE.STextOpenP) },
					{ CTL_SECRETPOOL_THIDDEN, const_cast<uint32 *>(&p_item->CE.STextHiddenP) },
					{ CTL_SECRETPOOL_TEXPIRY, const_cast<uint32 *>(&p_item->CE.STextExpiryP) },
					{ CTL_SECRETPOOL_TEXT1, const_cast<uint32 *>(&p_item->CE.STextExt1P) },
					{ CTL_SECRETPOOL_TEXT2, const_cast<uint32 *>(&p_item->CE.STextExt2P) },
					{ CTL_SECRETPOOL_TEXT3, const_cast<uint32 *>(&p_item->CE.STextExt3P) },
					{ CTL_SECRETPOOL_DESCR, const_cast<uint32 *>(&p_item->DescrP) },
				};
				for(uint i = 0; i < SIZEOFARRAY(tctl_tab); i++) {
					const TextFieldDescr & r_entry = tctl_tab[i];
					R_SecPool.GetText(*r_entry.P_DataIdx, temp_buf);
					setCtrlString(r_entry.CtlId, temp_buf);
				}
			}
			SetupSegmentType(p_item->SecType);
		}
	}
}

bool CentrigoSecretsDialog::GetCurrentInput()
{
	bool   ok = false;
	SString temp_buf;
	if(CurrentSegIdent) {
		PPSecretSegment * p_item = const_cast<PPSecretSegment *>(R_SecPool.SearchSegmentByID(CurrentSegIdent, 0)); // @badcast
		if(p_item) {
			const  PPSecretSegment preserve_segment(*p_item);
			p_item->SecType = getCtrlLong(CTLSEL_SECRETPOOL_TYPE);
			p_item->ParentID = getCtrlLong(CTLSEL_SECRETPOOL_PARENT);
			{
				LDATE  dt = getCtrlDate(CTL_SECRETPOOL_EXPIRY);
				SUniTime_Internal ut;
				ut.SetDate(dt);
				p_item->CE.UedBeforeTm = UED::_SetRaw_Time(UED_META_DATE_DAY, ut);
			}
			{
				TextFieldDescr tctl_tab[] = {
					{ CTL_SECRETPOOL_NAME, &p_item->NameP },
					{ CTL_SECRETPOOL_TOPEN, &p_item->CE.STextOpenP },
					{ CTL_SECRETPOOL_THIDDEN, &p_item->CE.STextHiddenP },
					{ CTL_SECRETPOOL_TEXPIRY, &p_item->CE.STextExpiryP },
					{ CTL_SECRETPOOL_TEXT1, &p_item->CE.STextExt1P },
					{ CTL_SECRETPOOL_TEXT2, &p_item->CE.STextExt2P },
					{ CTL_SECRETPOOL_TEXT3, &p_item->CE.STextExt3P },
					{ CTL_SECRETPOOL_DESCR, &p_item->DescrP },
				};
				for(uint i = 0; i < SIZEOFARRAY(tctl_tab); i++) {
					TextFieldDescr & r_entry = tctl_tab[i];
					getCtrlString(r_entry.CtlId, temp_buf);
					R_SecPool.PutText(temp_buf, r_entry.P_DataIdx);
				}
			}
			//
			if(!p_item->IsEq(preserve_segment)) {
				if(FilePath.NotEmpty()) {
					if(!R_SecPool.SaveStorage(FilePath))
						PPError();
				}
				SString preserve_name;
				R_SecPool.GetText(preserve_segment.NameP, preserve_name);
				if(temp_buf != preserve_name || p_item->ParentID != preserve_segment.ParentID) {
					updateList(-1); // -1 принципиально, поскольку функция возможно была вызвана в ответ на изменение фокус списка.
				}
			}
			ok = true;
		}
	}
	return ok;
}

IMPL_HANDLE_EVENT(CentrigoSecretsDialog)
{
	SmartListBox * p_box = P_Box;
	long   p;
	long   i;
	TDialog::handleEvent(event);
	if(TVCOMMAND) {
		switch(TVCMD) {
			case cmLBItemFocused:
				if(event.isCtlEvent(CTL_SECRETPOOL_LIST)) {
					long   pos = 0;
					long   id;
					if(getCurItem(&pos, &id)) {
						if(pos >= 0 && pos < R_SecPool.getCountI()) {
							if(CurrentSegIdent && CurrentSegIdent != id) {
								GetCurrentInput();	
							}
							SetupSelectedSegment(id);
						}
					}
				}
				clearEvent(event);
				break;
			case cmCBSelected:
				if(event.isCtlEvent(CTLSEL_SECRETPOOL_TYPE)) {
					uint   sec_type = static_cast<uint>(getCtrlLong(CTLSEL_SECRETPOOL_TYPE));
					SetupSegmentType(sec_type);
				}
				clearEvent(event);
				break;
			case cmaInsert:
				if(p_box) {
					p = i = 0;
					int    r = addItem(&p, &i);
					if(r > 0)
						updateList(p);
				}
				clearEvent(event);
				break;
			case cmInsertFolder:
				if(p_box) {
					p = i = 0;
					int    r = 0;
					{
						uint   new_seg_pos = 0;
						PPSecretSegment * p_new_seg = R_SecPool.CreateNewSegment(&new_seg_pos);
						if(p_new_seg) {
							SString name_buf;
							MakeNewFolderName(name_buf);
							R_SecPool.PutText(name_buf, &p_new_seg->NameP);
							p_new_seg->SecType = PPSecretSegment::sectypFolder;
							{
								if(FilePath.NotEmpty()) {
									if(!R_SecPool.SaveStorage(FilePath))
										PPError();
								}
							}
							p = static_cast<long>(new_seg_pos);
							i = static_cast<long>(p_new_seg->InternalID);
							r = 1;
						}
					}
					if(r > 0)
						updateList(p);						
				}
				clearEvent(event);
				break;
			case cmaDelete:
				if(getCurItem(&p, &i) && delItem(p, i) > 0) {
					updateList(-1);
				}
				clearEvent(event);
				break;
			case cmaEdit:
				if(getCurItem(&p, &i) && editItem(p, i) > 0) {
					const bool is_tree_list = (p_box && p_box->IsTreeList());
					const long id = is_tree_list ? i : p;
					if(is_tree_list)
						updateListById(id);
					else
						updateList(id);
				}
				clearEvent(event);
				break;
		}
	}
}

/*virtual*/SmartListBox * CentrigoSecretsDialog::GetListBoxCtl() const { return P_Box; }

/*virtual*/int CentrigoSecretsDialog::setupList()
{
	int    ok = 1;
	if(P_Box) {
		StrAssocArray * p_list = MakeStrAssocList();
		if(p_list) {
			ListBoxDef * p_def = new StdTreeListBoxDef(p_list, lbtDblClkNotify|lbtFocNotify|lbtDisposeData);
			{
				if(p_def && p_def->IsValid()) {
					LongArray list;
					p_def->ClearImageAssocList();
					if(p_def->getIdList(list) > 0) {
						Goods2Tbl::Rec gg_rec;
						for(uint i = 0; i < list.getCount(); i++) {
							const  PPID id = list.at(i);
							long   img_id = 0;
							uint   item_idx = 0;
							const  PPSecretSegment * p_item = R_SecPool.SearchSegmentByID(id, &item_idx);
							if(p_item) {
								switch(p_item->SecType) {
									case PPSecretSegment::sectypFolder:  img_id = PPDV_FOLDER01; break;
									case PPSecretSegment::sectypGeneric: img_id = PPDV_BOX01; break;
									case PPSecretSegment::sectypSSH: img_id = PPDV_FTP01; break;
									case PPSecretSegment::sectypBankCard:  img_id = PPDV_CARD02; break;
									default: img_id = PPDV_KEY01; break;
								}
								/*if(rec.Flags & PPInternetAccount::fFtpAccount)
									img_id = PPDV_FTP01;
								else 
									img_id = PPDV_MAIL01;
								*/
							}
							if(img_id)
								p_def->AddVecImageAssoc(id, img_id);
						}
					}
				}
			}
			P_Box->setDef(p_def);
		}
	}
	return ok;
}

int CentrigoSecretsDialog::Helper_MakeStrAssocList(uint32 parentId, StrAssocArray * pList)
{
	int    ok = -1;
	if(pList) {
		SString temp_buf;
		for(uint i = 0; i < R_SecPool.getCount(); i++) {
			const  PPSecretSegment * p_item = R_SecPool.at(i);
			if(p_item && p_item->ParentID == parentId) {
				if(R_SecPool.GetText(p_item->NameP, temp_buf)) {
					;
				}
				else {
					temp_buf.Z().CatChar('#').CatLongZ(p_item->InternalID, 6);
				}
				pList->Add(p_item->InternalID, p_item->ParentID, temp_buf);
				ok = 1;
				if(p_item->InternalID) {
					Helper_MakeStrAssocList(p_item->InternalID, pList); // @recursion
				}
			}
		}
	}
	return ok;
}

StrAssocArray * CentrigoSecretsDialog::MakeStrAssocList()
{
	StrAssocArray * p_result = new StrAssocArray;
	Helper_MakeStrAssocList(0, p_result);
	return p_result;
}

int Launch_TFacadeWindow()
{
	int    ok = -1;
	TFacadeWindow * p_win = new TFacadeWindow();
	InsertView(p_win);
	return ok;
}
