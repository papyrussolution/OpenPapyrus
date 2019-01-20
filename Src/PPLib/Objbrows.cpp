// OBJBROWS.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2013, 2015, 2016, 2017, 2018
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
//
// PPObjBrowser
//
PPObjBrowser::PPObjBrowser(uint rezID, DBQuery * q, PPObject * pObj, uint _flags, void * extraPtr) : BrowserWindow(rezID, q), flags(_flags), ExtraPtr(extraPtr)
{
	SetPPObjPtr(pObj);
}

void PPObjBrowser::SetPPObjPtr(PPObject * pObj)
{
	ppobj = pObj;
	if(ppobj) {
		if(!ppobj->CheckRights(PPR_INS))
			flags &= ~OLW_CANINSERT;
		if(!ppobj->CheckRights(PPR_MOD))
			flags &= ~OLW_CANEDIT;
		if(!ppobj->CheckRights(PPR_DEL))
			flags &= ~OLW_CANDELETE;
	}
}

PPID PPObjBrowser::currID()
{
	const void * p_row = view ? view->getCurItem() : 0;
	return p_row ? *(PPID *)p_row : 0L;
}

void PPObjBrowser::updateView()
{
	CALLPTRMEMB(view, refresh());
}

IMPL_HANDLE_EVENT(PPObjBrowser)
{
	PPID   id;
	BrowserWindow::handleEvent(event);
	if(TVCOMMAND) {
		switch(TVCMD) {
			case cmaInsert:
				if((flags & OLW_CANINSERT) && ppobj->Edit(&(id = 0), ExtraPtr) == cmOK)
					updateView();
				clearEvent(event);
				break;
			case cmaEdit:
				if(flags & OLW_CANEDIT) {
					if((id = currID()) != 0 && ppobj->Edit(&id, ExtraPtr) == cmOK)
						updateView();
					clearEvent(event);
				}
				break;
			case cmaDelete:
				if(flags & OLW_CANDELETE) {
					if((id = currID()) != 0 && ppobj->RemoveObjV(id, 0, PPObject::rmv_default, ExtraPtr) > 0)
						updateView();
					clearEvent(event);
				}
				break;
			default:
				break;
		}
	}
}
//
// PPObjListWindow
//
static ListBoxDef * __ListBoxDefFactory(PPID objType, StrAssocArray * pList, void * extraPtr)
{
	PPObject * p_obj = GetPPObject(objType, extraPtr);
	ListBoxDef * p_def = 0;
	if(p_obj && p_obj->GetImplementFlags() & PPObject::implTreeSelector)
		p_def = new StdTreeListBoxDef(pList, lbtDisposeData|lbtDblClkNotify, 0);
	else
		p_def = new StrAssocListBoxDef(pList, lbtDisposeData|lbtDblClkNotify);
	delete p_obj;
	return p_def;
}

PPObjListWindow::PPObjListWindow(PPID objType, StrAssocArray * pList, uint aFlags, void * extraPtr) :
	ListWindow(__ListBoxDefFactory(objType, pList, extraPtr), 0, 0)
{
	PPObject * p_obj = GetPPObject(objType, extraPtr);
	Init(p_obj, aFlags|OWL_OUTERLIST, extraPtr); // @v9.0.1 OWL_OUTERLIST
}

PPObjListWindow::PPObjListWindow(PPObject * aPPObj, uint aFlags, void * extraPtr) :
	ListWindow((aFlags & OLW_LOADDEFONOPEN) ? 0 : aPPObj->Selector(extraPtr), 0, 0)
{
	Init(aPPObj, aFlags, extraPtr);
}

void PPObjListWindow::Init(PPObject * aPPObj, uint aFlags, void * extraPtr)
{
	P_Obj = aPPObj;
	Flags = aFlags | OLW_CANEDIT;
	ExtraPtr = extraPtr;
	DefaultCmd = 0;
	if(P_Obj) {
		if(!P_Obj->CheckRights(PPR_INS))
			Flags &= ~OLW_CANINSERT;
		if(!P_Obj->CheckRights(PPR_DEL))
			Flags &= ~OLW_CANDELETE;
	}
}

PPObjListWindow::~PPObjListWindow()
{
	delete P_Obj;
}

int FASTCALL PPObjListWindow::valid(ushort command)
{
	if(command == cmOK) {
		PPID   id;
		int    r = 1;
		PPObject * p_obj = P_Obj;
		ListWindowSmartListBox * p_lb = P_Lb;
		if(!getResult(&id))
			id = 0;
		if(p_lb->isTreeList()) {
			r = ((StdTreeListBoxDef*)p_lb->def)->HasChild(id);
			r = BIN(r && (Flags & OLW_CANSELUPLEVEL) || !r);
			if(r)
				r = p_obj->ValidateSelection(id, Flags, ExtraPtr);
		}
		else if(p_obj)
			r = p_obj->ValidateSelection(id, Flags, ExtraPtr);
		if(r <= 0 && p_obj) {
			if(r < 0) {
				p_obj->UpdateSelector(p_lb->def, ExtraPtr);
				p_lb->setRange(p_lb->def->getRecsCount());
				p_lb->Draw_();
			}
			return 0;
		}
	}
	return ListWindow::valid(command);
}

IMPL_HANDLE_EVENT(PPObjListWindow)
{
	if(DefaultCmd) {
		if(TVCOMMAND && TVCMD == cmLBDblClk)
			TVCMD = DefaultCmd;
		else if(TVKEYDOWN && TVKEY == kbEnter) {
			event.what = TEvent::evCommand;
			TVCMD = DefaultCmd;
		}
	}
	ListWindow::handleEvent(event);
	PPObject * p_obj = P_Obj;
	if(p_obj) {
		int    update = 0; // Если установить указатель на элементе с id, то 2
		PPID   id = 0;
		PPID   preserve_focus_id = 0; // В некоторых случаях, при редактировании, идентификатор id может быть изменен.
			// Для таких случаев применяем preserve_focus_id чтобы правильно после спозиционировать текущий элемент.
		if(TVCOMMAND) {
			switch(TVCMD) {
				case cmLBLoadDef:
					if(!P_Def && (Flags & OLW_LOADDEFONOPEN)) {
						ListWindowSmartListBox * p_box = listBox();
						if(p_box) {
							setDef(p_obj->Selector(ExtraPtr));
							p_box->setDef(P_Def);
							ComboBox * p_combo = p_box->combo;
							if(p_combo) {
								p_combo->setDef(P_Def);
								p_combo->setDataByUndefID();
							}
						}
					}
					break;
				case cmaInsert:
					id = 0;
					if(Flags & OLW_CANINSERT && !(Flags & OWL_OUTERLIST)) { // @v9.0.1 !(Flags & OWL_OUTERLIST)
						if(p_obj->Edit(&id, ExtraPtr) == cmOK) {
							preserve_focus_id = id;
							update = 2;
						}
						else
							::SetFocus(H());
					}
					break;
				case cmaDelete:
					if(Flags & OLW_CANDELETE && !(Flags & OWL_OUTERLIST) && getResult(&id) && id) { // @9.0.1 !(Flags & OWL_OUTERLIST)
						preserve_focus_id = id;
						if(p_obj->RemoveObjV(id, 0, PPObject::rmv_default, ExtraPtr) > 0)
							update = 2;
					}
					break;
				case cmaEdit:
					if(Flags & OLW_CANEDIT && getResult(&id) && id) {
						RECT rc;
						if(GetWindowRect(H(), &rc)) {
                    		SLS.GetTLA().SetNextDialogLuPos(rc.right+1, rc.top);
						}
						preserve_focus_id = id;
						if(p_obj->Edit(&id, ExtraPtr) == cmOK) {
							if(!(Flags & OWL_OUTERLIST)) // @v9.0.1
								update = 2;
							else {
								// @todo Необходимо изменить строку, если текст объекта изменился
								::SetFocus(H());
							}
						}
						else
							::SetFocus(H());
					}
					break;
				case cmTransmit:
					if(getResult(&id) && id)
						Transmit(id);
					break;
				case cmSysJournalByObj:
					if(getResult(&id) && id) {
						((PPApp*)APPL)->LastCmd = TVCMD;
						ViewSysJournal(p_obj->Obj, id, 0);
					}
					break;
				default:
					return;
			}
			clearEvent(event);
		}
		else if(TVKEYDOWN)
			if(TVKEY == KB_CTRLENTER) {
				if(Flags & OLW_CANEDIT && getResult(&id) && id) {
					preserve_focus_id = id;
					if(p_obj->Edit(&id, ExtraPtr) == cmOK) {
						if(!(Flags & OWL_OUTERLIST)) // @v9.0.1
							update = 2;
						else {
							// @todo Необходимо изменить строку, если текст объекта изменился
							::SetFocus(H());
						}
					}
					else
						::SetFocus(H());
					clearEvent(event);
				}
			}
			else if(TVKEY == kbAltF2) {
				if(Flags & OLW_CANINSERT && p_obj->Obj == PPOBJ_GOODS && getResult(&id) && id) {
					PPID   new_id = 0;
					if(((PPObjGoods*)p_obj)->AddBySample(&new_id, id) == cmOK) {
						preserve_focus_id = new_id;
						update = 2;
					}
					else
						::SetFocus(H());
					clearEvent(event);
				}
			}
			else
				return;
		PostProcessHandleEvent(update, preserve_focus_id);
	}
}

void PPObjListWindow::PostProcessHandleEvent(int update, PPID focusID)
{
	if(update) {
		ListWindowSmartListBox * p_lb = P_Lb;
		P_Obj->UpdateSelector(p_lb->def, ExtraPtr);
		p_lb->Draw_();
		p_lb->setRange(p_lb->def->getRecsCount());
		if(update == 2)
			p_lb->search(&focusID, 0, srchFirst|lbSrchByID);
		::SetFocus(H());
	}
}

int PPObjListWindow::Transmit(PPID)
{
	int    ok = -1;
	if(P_Obj && IS_REF_OBJTYPE(P_Obj->Obj)) {
		ObjTransmitParam param;
		if(ObjTransmDialog(DLG_OBJTRANSM, &param) > 0) {
			PPID   id = 0;
			const PPIDArray & rary = param.DestDBDivList.Get();
			PPObjIDArray objid_ary;
			PPWait(1);
			for(id = 0; ((PPObjReference *)P_Obj)->EnumItems(&id, 0) > 0;)
				objid_ary.Add(P_Obj->Obj, id);
			THROW(PPObjectTransmit::Transmit(&rary, &objid_ary, &param));
			ok = 1;
		}
	}
	CATCHZOKPPERR
	PPWait(0);
	return ok;
}
//
// PPListDialog
//
SLAPI PPListDialog::PPListDialog(uint rezID, uint aCtlList, long flags) : TDialog(rezID), ctlList(aCtlList), Options(0)
{
	if(flags & fOnDblClkOk)
		Options |= oOnDblClkOk;
	if(getCtrlView(STDCTL_OKBUTTON))
		Options |= oHasOkButton;
	if(getCtrlView(STDCTL_EDITBUTTON))
		Options |= oHasEditButton;
	P_Box = (SmartListBox*)getCtrlView(ctlList);
	if(!SetupStrListBox(P_Box))
		PPError();
	else {
		// @v9.8.11 {
		if(flags & fOmitSearchByFirstChar)
			P_Box->SetOmitSearchByFirstChar();
		// } @v9.8.11 
		// @v10.3.0 {
		if(flags & fOwnerDraw)
			P_Box->SetOwnerDrawState();
		// } @v10.3.0 
	}
	if(isCurrCtlID(ctlList) && (Options & oHasOkButton) && (Options & oHasEditButton)) {
		SetDefaultButton(STDCTL_OKBUTTON,   0);
	   	SetDefaultButton(STDCTL_EDITBUTTON, 1);
	}
	updateList(-1);
}

IMPL_HANDLE_EVENT(PPListDialog)
{
	SmartListBox * p_box = P_Box;
	long   p, i;
	TDialog::handleEvent(event);
	if(TVCOMMAND) {
		switch(TVCMD) {
			case cmaInsert:
				if(p_box) {
					p = i = 0;
					int    r = addItem(&p, &i);
					if(r == 2)
						updateList(i, 0);
					else if(r > 0)
						updateList(p);
				}
				break;
			case cmaDelete:
				if(getCurItem(&p, &i) && delItem(p, i) > 0)
					updateList(-1);
				break;
			case cmaEdit:
				if(getCurItem(&p, &i) && editItem(p, i) > 0) {
					int is_tree_list = BIN(p_box && p_box->isTreeList());
					long id = (is_tree_list) ? i : p;
					updateList(id, !BIN(is_tree_list));
				}
				break;
			case cmaSendByMail:
				if(getCurItem(&p, &i))
					 sendItem(p, i);
				break;
			case cmDown:
			case cmUp:
				if(getCurItem(&p, &i)) {
					int    up = (TVCMD == cmUp) ? 1 : 0;
					if(moveItem(p, i, up) > 0)
						updateList(up ? p-1 : p+1);
				}
				break;
			case cmLBDblClk:
				if(p_box && p_box->def) {
					int    edit = 1, is_tree_list = 0;
					PPID   cur_id = 0;
					p_box->def->getCurID(&cur_id);
					if(p_box->isTreeList()) {
						is_tree_list = 1;
						if(((StdTreeListBoxDef*)p_box->def)->HasChild(cur_id))
							edit = 0;
					}
					if(event.isCtlEvent(ctlList)) {
						if(cur_id && Options & oOnDblClkOk) {
							TView::messageCommand(this, cmOK);
						}
						else if(edit && getCurItem(&p, &i) && editItem(p, i) > 0) {
							long id = is_tree_list ? i : p;
							updateList(id, !BIN(is_tree_list));
						}
						else
							return;
					}
					else
						return;
				}
				else
					return;
				break;
			case cmRightClick:
				{
					SString temp_buf;
					if(PPLoadTextWin(PPTXT_MENU_LISTBOX, temp_buf)) {
						getCurItem(&p, &i);
						TMenuPopup menu;
						menu.AddSubstr(temp_buf, 0, cmaInsert);     // Добавить
						menu.AddSubstr(temp_buf, 1, cmaEdit);       // Редактировать
						menu.AddSubstr(temp_buf, 2, cmaDelete);     // Удалить
						if(p_box && p_box->def && (p_box->def->Options & lbtExtMenu))
							menu.AddSubstr(temp_buf, 3, cmaSendByMail); // Послать по эл. почте
						int    cmd = menu.Execute(H(), TMenuPopup::efRet);
						if(cmd > 0)
							TView::messageCommand(this, cmd);
					}
				}
				break;
			default:
				return;
		}
	}
	else if(TVBROADCAST) {
		if((Options & oHasOkButton) && (Options & oHasEditButton) && ctlList) {
			if(TVCMD == cmReceivedFocus && event.isCtlEvent(ctlList)) {
				SetDefaultButton(STDCTL_OKBUTTON,   0);
				SetDefaultButton(STDCTL_EDITBUTTON, 1);
			}
			else if(TVCMD == cmReleasedFocus && event.isCtlEvent(ctlList)) {
				SetDefaultButton(STDCTL_OKBUTTON,   1);
				SetDefaultButton(STDCTL_EDITBUTTON, 0);
			}
		}
		return;
	}
	else if(TVKEYDOWN)
		if(TVKEY == KB_CTRLENTER) {
			if(Options & oHasOkButton) {
				if(IsInState(sfModal)) {
					endModal(cmOK);
					return; // После endModal не следует обращаться к this
				}
			}
			else
				TView::messageCommand(this, cmaMore, this);
		}
		else
			return;
	else
		return;
	clearEvent(event);
}

int SLAPI PPListDialog::getSelection(long * pID)
	{ return getCurItem(0, pID); }
int SLAPI PPListDialog::addStringToList(long itemId, const char * pText)
	{ return (!P_Box || !P_Box->addItem(itemId, pText)) ? PPSetErrorSLib() : 1; }

void SLAPI PPListDialog::updateList(long pos, int byPos /*= 1*/)
{
	SmartListBox * p_box = P_Box;
	if(p_box) {
		int    sav_pos = p_box->def ? (int)p_box->def->_curItem() : 0;
		p_box->freeAll();
		if(setupList()) {
			p_box->Draw_();
			if(byPos)
		   		p_box->focusItem((pos < 0) ? sav_pos : pos);
			else
				p_box->search(&pos, 0, srchFirst|lbSrchByID);
		}
		else
			PPError();
	}
}

int SLAPI PPListDialog::getCurItem(long * pPos, long * pID) const
{
	SmartListBox * p_box = P_Box;
	if(p_box && p_box->def) {
		long   i = 0;
		p_box->getCurID(&i);
		ASSIGN_PTR(pPos, p_box->def->_curItem());
		ASSIGN_PTR(pID, i);
		return 1;
	}
	else
		return 0;
}

int SLAPI PPListDialog::getCurString(SString & rBuf) const
{
	SmartListBox * p_box = P_Box;
	if(p_box && p_box->def) {
		p_box->getCurString(rBuf);
		return 1;
	}
	else
		return 0;
}

int SLAPI PPListDialog::getText(long itemN /* 0.. */, SString & rBuf)
{
	SmartListBox * p_box = P_Box;
	return (p_box && p_box->def) ? p_box->getText(itemN, rBuf) : 0;
}

int PPListDialog::setupList() { return -1; }
int PPListDialog::addItem(long *, long *) { return -1; }
int PPListDialog::editItem(long, long) { return -1; }
int PPListDialog::delItem(long, long) { return -1; }
int PPListDialog::moveItem(long pos, long id, int up) { return -1; }
int PPListDialog::sendItem(long, long) { return -1; }
//
// ObjRestrictListDialog
//
ObjRestrictListDialog::ObjRestrictListDialog(uint dlgID, uint listCtlID) : PPListDialog(dlgID, listCtlID), ObjType(0), P_ORList(0)
{
	updateList(-1);
}

void ObjRestrictListDialog::setParams(PPID objType, ObjRestrictArray * pData)
{
	ObjType = objType;
	P_ORList = pData;
}

IMPL_HANDLE_EVENT(ObjRestrictListDialog)
{
	ObjRestrictArray * p_orlist = P_ORList;
	long   p, i;
	PPListDialog::handleEvent(event);
	if(TVCOMMAND && p_orlist) {
		if(TVCMD == cmaLevelUp || TVCMD == cmUp) {
			if(getCurItem(&p, &i) && p > 0) {
				p_orlist->swap(p, p-1);
				updateList(p-1);
			}
		}
		else if(TVCMD == cmaLevelDown || TVCMD == cmDown) {
			if(getCurItem(&p, &i) && p < (long)p_orlist->getCount()-1) {
				p_orlist->swap(p, p+1);
				updateList(p+1);
			}
		}
		else
			return;
	}
	else
		return;
	clearEvent(event);
}

int ObjRestrictListDialog::getObjName(PPID objID, long, SString & rBuf)
{
	if(ObjType) {
		if(!GetObjectName(ObjType, objID, rBuf))
			rBuf.Cat(objID);
		return 1;
	}
	else {
		rBuf.Z();
		return -1;
	}
}

void ObjRestrictListDialog::getExtText(PPID, long /*objFlags*/, SString & rBuf)
{
	rBuf.Z();
}

int ObjRestrictListDialog::setupList()
{
	if(P_ORList) {
		ObjRestrictItem * p_item;
		SString sub;
		StringSet ss(SLBColumnDelim);
		for(uint i = 0; P_ORList->enumItems(&i, (void**)&p_item);) {
			ss.clear();
			getObjName(p_item->ObjID, p_item->Flags, sub);
			ss.add(sub);
			getExtText(p_item->ObjID, p_item->Flags, sub);
			ss.add(sub);
			if(!P_Box->addItem(p_item->ObjID, ss.getBuf()))
				return PPSetErrorSLib();
		}
		return 1;
	}
	else
		return -1;
}

int ObjRestrictListDialog::addItem(long * pPos, long * pID)
{
	if(P_ORList) {
		uint   pos = 0;
		ObjRestrictItem item;
		MEMSZERO(item);
		if(editItemDialog(&item) > 0)
			if(P_ORList->Add(item.ObjID, item.Flags, &pos)) {
				ASSIGN_PTR(pID, item.ObjID);
				ASSIGN_PTR(pPos, pos);
				return 1;
			}
			else
				return 0;
	}
	return -1;
}

int ObjRestrictListDialog::editItem(long pos, long id)
{
	ObjRestrictArray * p_orlist = P_ORList;
	if(p_orlist) {
		const uint p = (uint)pos;
		if(p < p_orlist->getCount()) {
			ObjRestrictItem item = p_orlist->at(p);
			if(editItemDialog(&item) > 0) {
				p_orlist->at(p) = item;
				return 1;
			}
		}
	}
	return -1;
}

int ObjRestrictListDialog::delItem(long pos, long id)
{
	ObjRestrictArray * p_orlist = P_ORList;
	if(p_orlist && ((uint)pos) < p_orlist->getCount()) {
		p_orlist->atFree((uint)pos);
		return 1;
	}
	else
		return -1;
}
