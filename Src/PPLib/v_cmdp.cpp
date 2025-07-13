// V_CMDP.CPP
// Copyright (c) A.Starodub 2006, 2007, 2008, 2009, 2011, 2012, 2013, 2014, 2016, 2017, 2018, 2019, 2020, 2021, 2023, 2024, 2025
// @codepage UTF-8
// Редактирование списка команд
//
#include <pp.h>
#pragma hdrstop

int MenuResToMenu(uint resMenuID, PPCommandFolder * pMenu);
//
//
//
#define USEDEFICON         0x01L
#define DEFAULT_MENUS_OFFS 100000L
//
//
//
int EditCmdItem(const PPCommandGroup * pGrp, PPCommand * pData, /*int isDekstopCmd*/PPCommandGroupCategory kind)
{
	class CmdItemDialog : public TDialog {
		DECL_DIALOG_DATA(PPCommand);
		enum {
			ctlgroupFbg = 1
		};
	public:
		CmdItemDialog(const PPCommandGroup * pGrp, /*int isDesktopCmd*/PPCommandGroupCategory cmdgrpc) : TDialog(DLG_CMDITEM), 
			/*IsDesktopCmd(isDesktopCmd)*/CmdGrpC(cmdgrpc), P_Grp(pGrp)
		{
			assert(oneof2(CmdGrpC, cmdgrpcMenu, cmdgrpcDesktop));
			CmdDescr.GetResourceList(1, CmdSymbList);
			CmdSymbList.SortByText(); // @v12.3.7
			if(/*IsDesktopCmd*/CmdGrpC == cmdgrpcDesktop)
				FileBrowseCtrlGroup::Setup(this, CTLBRW_CMDITEM_ICON, CTL_CMDITEM_ICON, ctlgroupFbg, PPTXT_SELCMDICON, PPTXT_FILPAT_ICONS, FileBrowseCtrlGroup::fbcgfFile);
			disableCtrl(CTL_CMDITEM_ICON, true);
			disableCtrl(CTL_CMDITEM_USEDEFICON, /*!IsDesktopCmd*/CmdGrpC != cmdgrpcDesktop);
			disableCtrl(CTLBRW_CMDITEM_ICON, /*!IsDesktopCmd*/CmdGrpC != cmdgrpcDesktop);
			enableCommand(cmCmdParam, /*IsDesktopCmd*/CmdGrpC == cmdgrpcDesktop);
		}
		DECL_DIALOG_SETDTS()
		{
			int    ok = 1;
			// @v12.3.7 (мы в конструкторе уже инициализировали такой объект CmdSymbList) StrAssocArray cmd_txt_list;
			RVALUEPTR(Data, pData);
			setCtrlString(CTL_CMDITEM_NAME, Data.Name);
			setCtrlLong(CTL_CMDITEM_ID, Data.GetID());
			// @v12.3.7 (мы в конструкторе уже инициализировали такой объект CmdSymbList) CmdDescr.GetResourceList(1, cmd_txt_list);
			uint   pos = 0;
			// @v12.3.7 (мы в конструкторе уже инициализировали такой объект CmdSymbList) cmd_txt_list.SortByText();
			SetupStrAssocCombo(this, CTLSEL_CMDITEM_CMD, /*cmd_txt_list*/CmdSymbList, Data.CmdID, 0);
			SetupWordSelector(CTLSEL_CMDITEM_CMD, 0, Data.CmdID, 2, WordSel_ExtraBlock::fAlwaysSearchBySubStr);
			setCtrlString(CTL_CMDITEM_ICON, Data.Icon);
			AddClusterAssoc(CTL_CMDITEM_USEDEFICON, 0, USEDEFICON);
			SetClusterData(CTL_CMDITEM_USEDEFICON, Data.Icon.ToLong() || !Data.Icon.Len());
			AddClusterAssoc(CTL_CMDITEM_FLAGS, 0, PPCommand::fAllowEditFilt);
			SetClusterData(CTL_CMDITEM_FLAGS, Data.Flags);
			disableCtrl(CTLBRW_CMDITEM_ICON, Data.Icon.ToLong() || !Data.Icon.Len());
			disableCtrl(CTLSEL_CMDITEM_CMD, Data.CmdID);
			disableCtrl(CTL_CMDITEM_ID, true);
			if(Data.GetID() && CmdDescr.LoadResource(Data.CmdID) > 0)
				enableCommand(cmCmdParam, !(CmdDescr.Flags & PPCommandDescr::fNoParam));
			else
				enableCommand(cmCmdParam, 0);
			{
				FileBrowseCtrlGroup * p_fbg = static_cast<FileBrowseCtrlGroup *>(getGroup(ctlgroupFbg));
				if(p_fbg) {
					SString spath;
					if(!Data.Icon.Len() || Data.Icon.ToLong()) {
						PPGetPath(PPPATH_BIN, spath);
						spath.SetLastSlash().Cat("Icons").SetLastSlash();
					}
					else
						spath = Data.Icon;
					p_fbg->setInitPath(spath);
				}
			}
			SetupCtrls();
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			PPID   cmd_id = 0;
			PPCommandDescr cmd_descr;
			getCtrlString(CTL_CMDITEM_NAME, Data.Name);
			THROW_PP(Data.Name.NotEmptyS(), PPERR_NAMENEEDED);
			getCtrlData(CTLSEL_CMDITEM_CMD, &cmd_id);
			getCtrlString(CTL_CMDITEM_ICON, Data.Icon);
			THROW_PP(CmdSymbList.Search(cmd_id), PPERR_INVJOBCMD);
			THROW(cmd_descr.LoadResource(cmd_id));
			Data.CmdID  = cmd_id;
			Data.Flags = 0;
			GetClusterData(CTL_CMDITEM_FLAGS, &Data.Flags);
			if(!Data.Icon.Len())
				Data.Icon.Cat(cmd_descr.IconId);
			ASSIGN_PTR(pData, Data);
			CATCHZOKPPERR
			return ok;
		}
	private:
		struct _CmdID {
			long   CmdID;
			PPIDArray Ary;
		};
		static int _GetIdListByCommand(const PPCommandItem * pItem, long parentID, void * extraPtr)
		{
			if(pItem) {
				_CmdID * p_e = static_cast<_CmdID *>(extraPtr);
				PPCommand * p_cmd = pItem->IsKind(PPCommandItem::kCommand) ? static_cast<PPCommand *>(pItem->Dup()) : 0;
				if(p_cmd && p_cmd->CmdID == p_e->CmdID)
					p_e->Ary.addUnique(p_cmd->GetID());
				ZDELETE(p_cmd);
			}
			return 1;
		}
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCbSelected(CTLSEL_CMDITEM_CMD)) {
				const  PPID cmd_id = getCtrlLong(CTLSEL_CMDITEM_CMD);
				if(cmd_id && CmdDescr.LoadResource(cmd_id) > 0) {
					SString name = CmdDescr.Text;
					if(P_Grp) {
						_CmdID _e;
						_e.CmdID = cmd_id;
						P_Grp->Enumerate(_GetIdListByCommand, 0, &_e);
						if(_e.Ary.getCount())
							name.Space().CatChar('#').Cat(_e.Ary.getCount());
					}
					setCtrlString(CTL_CMDITEM_NAME, name);
					if(/*IsDesktopCmd*/CmdGrpC == cmdgrpcDesktop)
						enableCommand(cmCmdParam, !(CmdDescr.Flags & PPCommandDescr::fNoParam));
				}
				else
					enableCommand(cmCmdParam, 0);
			}
			else if(event.isCmd(cmCmdParam)) {
				const uint sav_offs = Data.Param.GetRdOffs();
				if(CmdDescr.EditCommandParam(getCtrlLong(CTLSEL_JOBITEM_CMD), Data.GetID(), &Data.Param, 0)) {
					Data.Param.SetRdOffs(sav_offs);
					SetupCtrls();
				}
				else
					PPError();
			}
			else if(event.isCmd(cmClearFilt)) {
				Data.Param.Z();
				SetupCtrls();
			}
			else if(event.isClusterClk(CTL_CMDITEM_USEDEFICON)) {
				long   f = 0;
				GetClusterData(CTL_CMDITEM_USEDEFICON, &f);
				disableCtrl(CTLBRW_CMDITEM_ICON, (f & USEDEFICON));
				if(f & USEDEFICON) {
					long   cmd_id = getCtrlLong(CTLSEL_CMDITEM_CMD);
					PPCommandDescr cmd_descr;
					if(cmd_id && CmdSymbList.Search(cmd_id) > 0 && cmd_descr.LoadResource(cmd_id) > 0) {
						SString icon;
						icon.Cat(cmd_descr.IconId);
						setCtrlString(CTL_CMDITEM_ICON, icon);
					}
				}
			}
			else
				return;
			clearEvent(event);
		}
		void   SetupCtrls()
		{
			showCtrl(CTL_CMDITEM_CLEARFILT, LOGIC(Data.Param.GetAvailableSize()));
		}

		const PPCommandGroupCategory CmdGrpC;
		PPCommandDescr CmdDescr;
		const PPCommandGroup * P_Grp;
		StrAssocArray CmdSymbList;
	};
	DIALOG_PROC_BODY_P2(CmdItemDialog, pGrp, /*isDekstopCmd*/kind, pData);
}
//
// CommandsDialog
//
class CommandsDialog : public PPListDialog {
	DECL_DIALOG_DATA(PPCommandGroup);
public:
	explicit CommandsDialog(PPCommandGroupCategory cmdgrpc) : PPListDialog(DLG_LBXSELT, CTL_LBXSEL_LIST), CmdGrpC(cmdgrpc)
	{
		SString title;
		setTitle(PPLoadTextS(PPTXT_EDITCMDLIST, title));
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		updateList(-1);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		ASSIGN_PTR(pData, Data);
		return 1;
	}
private:
	virtual int setupList()
	{
		int    ok = -1;
		StrAssocArray * p_list = 0;
		StdTreeListBoxDef * p_def = 0;
		if(P_Box) {
			THROW_MEM(p_list = new StrAssocArray);
			THROW(Data.GetCommandList(p_list, 0));
			THROW_MEM(p_def = new StdTreeListBoxDef(p_list, lbtDisposeData|lbtDblClkNotify, 0));
			P_Box->setDef(p_def);
			P_Box->Draw_();
			ok = 1;
		}
		CATCH
			if(p_def)
				delete p_def;
			else
				delete p_list;
			ok = 0;
		ENDCATCH
		return ok;
	}
	virtual int addItem(long * pPos, long * pID)
	{
		int    ok = -1;
		long   parent_id = 0;
		long   parent_id2 = 0;
		long   v = 1;
		const  PPCommandItem * p_selitem = 0;
		TDialog * p_dlg = new TDialog(DLG_ADDCMD);
		StrAssocArray cmd_list;
		THROW(Data.GetCommandList(&cmd_list, 1));
		THROW(CheckDialogPtr(&p_dlg));
		p_dlg->AddClusterAssocDef(CTL_ADDCMD_WHAT,  0, 1);
		p_dlg->AddClusterAssoc(CTL_ADDCMD_WHAT,  1, 2);
		p_dlg->AddClusterAssoc(CTL_ADDCMD_WHAT,  2, 3);
		p_dlg->SetClusterData(CTL_ADDCMD_WHAT, v);
		P_Box->getCurID(&parent_id);
		if(!(p_selitem = Data.SearchByIDRecursive_Const(parent_id, &parent_id2)) || !p_selitem->IsKind(PPCommandItem::kFolder))
			parent_id = parent_id2;
		SetupStrAssocCombo(p_dlg, CTLSEL_ADDCMD_PARENT, cmd_list, parent_id, 0, 0);
		if(ExecView(p_dlg) == cmOK) {
			PPCommandItem * p_item = 0;
			PPCommandItem new_sep(PPCommandItem::kSeparator);
			PPCommand new_cmd;
			PPCommandFolder new_cmdfolder;
			p_dlg->GetClusterData(CTL_ADDCMD_WHAT, &v);
			p_dlg->getCtrlData(CTLSEL_ADDCMD_PARENT, &parent_id);
			if(v == 1) {
				if(EditCmdItem(&Data, &new_cmd, CmdGrpC) > 0)
					p_item = static_cast<PPCommandItem *>(&new_cmd);
			}
			else if(v == 2) {
				if(EditName(new_cmdfolder.Name) > 0)
					p_item = static_cast<PPCommandItem *>(&new_cmdfolder);
			}
			else {
				new_sep.Name.Z().CatCharN('-', 40);
				p_item = &new_sep;
			}
			if(p_item) {
				uint p = 0;
				p_item->ID = Data.GetUniqueID();
				if(parent_id) {
					PPCommandItem * p_fi = Data.SearchByIDRecursive(parent_id, 0);
					PPCommandFolder * p_folder = (p_fi && p_fi->IsKind(PPCommandItem::kFolder)) ? static_cast<PPCommandFolder *>(p_fi) : 0;
					if(p_folder)
						THROW(p_folder->Add(-1, p_item));
				}
				else {
					THROW(Data.Add(-1, p_item));
				}
				{
					THROW(Data.GetCommandList(&cmd_list, 0));
					cmd_list.Search(p_item->GetID(), &(p = 0));
					ASSIGN_PTR(pPos, p);
				}
				ASSIGN_PTR(pID,  p_item->GetID());
				ok = 1;
			}
		}
		CATCHZOKPPERR
		delete p_dlg;
		return ok;
	}
	virtual int editItem(long pos, long id)
	{
		int    ok = -1;
		PPCommandItem * p_item = Data.SearchByIDRecursive(id, 0);
		if(p_item) {
			if(p_item->IsKind(PPCommandItem::kCommand))
				ok = EditCmdItem(&Data, static_cast<PPCommand *>(p_item), CmdGrpC);
			else if(oneof2(p_item->GetKind(), PPCommandItem::kFolder, PPCommandItem::kGroup))
				ok = EditName(p_item->Name);
		}
		return ok;
	}
	virtual int delItem(long pos, long id)
	{
		int    ok = -1;
		if(CONFIRM(PPCFM_DELITEM)) {
			long   parent_id = 0;
			const  PPCommandItem * p_item = Data.SearchByIDRecursive_Const(id, &parent_id);
			PPCommandFolder * p_folder = 0;
			if(p_item) {
				uint   p = 0;
				if(parent_id) {
					PPCommandItem * p_pitem = Data.SearchByIDRecursive(parent_id, 0);
					p_folder = (p_pitem && oneof2(p_pitem->GetKind(), PPCommandItem::kFolder, PPCommandItem::kGroup)) ? static_cast<PPCommandFolder *>(p_pitem) : 0;
				}
				else
					p_folder = &Data;
				if(p_folder && p_folder->SearchByID(id, &p))
					ok = p_folder->Remove(p);
			}
		}
		return ok;
	}
	virtual int moveItem(long pos, long id, int up)
	{
		int    ok = -1;
		if(id > 0) {
			long   pos = 0;
			PPCommandFolder * p_folder = 0;
			PPCommandItem * p_item = GetItem(id, &pos, &p_folder);
			if(p_folder) {
				const uint items_count = p_folder->GetCount();
				if(items_count > 1) {
					uint   p;
					uint   nb_pos = 0;
					PPID   mm = 0;
					const  PPCommandItem * p_citem = 0;
					for(p = 0; (p_citem = p_folder->Next(&p));) {
						if(up)
							mm = (p_citem->GetID() > mm && p_citem->GetID() < id) ? p_citem->GetID() : mm;
						else
							mm = ((!mm || p_citem->GetID() < mm) && p_citem->GetID() > id) ? p_citem->GetID() : mm;
					}
					p_citem = p_folder->SearchByID(mm, &nb_pos);
					if(p_citem) {
						PPCommandItem * p_nbitem = p_citem->Dup();
						if(p_nbitem) {
							const uint _upos = static_cast<uint>(pos);
							p_folder->Remove(_upos > nb_pos ? _upos : nb_pos);
							p_folder->Remove(_upos > nb_pos ? nb_pos : _upos);
							id = p_item->GetID();
							p_item->ID   = p_nbitem->GetID();
							p_nbitem->ID = id;
							p_folder->Add(-1, p_item);
							p_folder->Add(-1, p_nbitem);
							ZDELETE(p_nbitem);
							ok = 1;
						}
					}
				}
			}
			ZDELETE(p_item);
		}
		return ok;
	}
	PPCommandItem * GetItem(long id, long * pPos, PPCommandFolder ** ppFolder)
	{
		long   parent_id = 0;
		const  PPCommandItem * p_item = Data.SearchByIDRecursive_Const(id, &parent_id);
		PPCommandItem * p_retitem = 0;
		if(p_item) {
			uint   p = 0;
			if(parent_id) {
				PPCommandItem * p_pitem = Data.SearchByIDRecursive(parent_id, 0);
				(*ppFolder) = (p_pitem && oneof2(p_pitem->GetKind(), PPCommandItem::kFolder, PPCommandItem::kGroup)) ? static_cast<PPCommandFolder *>(p_pitem) : 0;
			}
			else
				(*ppFolder) = &Data;
			if(*ppFolder) {
				p_retitem = ((*ppFolder)->SearchByID(id, &p))->Dup();
				ASSIGN_PTR(pPos, p);
			}
		}
		return p_retitem;
	}
	const PPCommandGroupCategory CmdGrpC;
};

/*static*/int PPDesktop::EditAssocCmdList(/*long desktopID*/const S_GUID & rDesktopUuid)
{
	class DesktopAssocCmdsDialog : public PPListDialog {
		DECL_DIALOG_DATA(PPDesktopAssocCmdPool);
	public:
		DesktopAssocCmdsDialog() : PPListDialog(DLG_DESKCMDA, CTL_DESKCMDA_LIST)
		{
			PPCommandDescr cmd_descr;
			cmd_descr.GetResourceList(1, CmdList);
			disableCtrl(CTLSEL_DESKCMDA_DESKTOP, true);
		}
		DECL_DIALOG_SETDTS()
		{
			StrAssocArray list;
			if(!RVALUEPTR(Data, pData)) {
				Data.Init(ZEROGUID);
			}
			PPCommandFolder::GetCommandGroupList(0, cmdgrpcDesktop, DesktopList);
			DesktopList.GetStrAssocList(list);
			long  surr_id = DesktopList.GetSurrIdByUuid(Data.GetDesktopUuid());
			SetupStrAssocCombo(this, CTLSEL_DESKCMDA_DESKTOP, list, /*Data.GetDesktopID()*/surr_id, 0, 0);
			updateList(-1);
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			const  PPID surr_id = getCtrlLong(CTLSEL_DESKCMDA_DESKTOP);
			S_GUID uuid = DesktopList.GetUuidBySurrId(surr_id);
			Data.SetDesktopUuid(uuid);
			ASSIGN_PTR(pData, Data);
			return 1;
		}
	private:
		virtual int setupList()
		{
			int    ok = 1;
			PPDesktopAssocCmd assci;
			StringSet ss(SLBColumnDelim);
			for(uint i = 0; ok && i < Data.GetCount(); i++) {
				if(Data.GetItem(i, assci)) {
					uint pos = 0;
					if(CmdList.Search(assci.CmdID, &pos) > 0) {
						ss.Z();
						ss.add(assci.Code);
						ss.add(CmdList.Get(pos).Txt);
						if(!addStringToList(i+1, ss.getBuf()))
							ok = 0;
					}
				}
			}
			return ok;
		}
		virtual int addItem(long * pPos, long * pID)
		{
			int    ok = -1;
			uint   pos = 0;
			PPDesktopAssocCmd cmd_assc;
			if(EditCommandAssoc(-1, &cmd_assc, &Data) > 0) {
				ok = Data.AddItem(&cmd_assc);
				if(ok > 0) {
					pos = Data.GetCount()-1;
					ASSIGN_PTR(pPos, pos);
					ASSIGN_PTR(pID, pos+1);
				}
			}
			return ok;
		}
		virtual int editItem(long pos, long id)
		{
			int    ok = -1;
			PPDesktopAssocCmd cmd_assc;
			if(Data.GetItem(pos, cmd_assc)) {
				ok = EditCommandAssoc(pos, &cmd_assc, &Data);
				if(ok > 0) {
					Data.SetItem(pos, &cmd_assc);
				}
			}
			return ok;
		}
		virtual int delItem(long pos, long id)
		{
			return (pos >= 0 && pos < (long)Data.GetCount()) ? Data.SetItem(pos, 0) : -1;
		}
		int    EditCommandAssoc(long pos, PPDesktopAssocCmd * pAsscCmd, PPDesktopAssocCmdPool * pCmdList)
		{
			class DesktopAssocCommandDialog : public TDialog {
				DECL_DIALOG_DATA(PPDesktopAssocCmd);
			public:
				DesktopAssocCommandDialog(long pos, PPDesktopAssocCmdPool * pCmdList) : TDialog(DLG_DESKCMDAI), Pos(pos), P_CmdList(pCmdList)
				{
				}
				DECL_DIALOG_SETDTS()
				{
					StrAssocArray cmd_list;
					PPCommandDescr cmd_descr;
					if(!RVALUEPTR(Data, pData))
						MEMSZERO(Data);
					cmd_descr.GetResourceList(1, cmd_list);
					cmd_list.SortByText();
					setCtrlString(CTL_DESKCMDAI_CODE, Data.Code);
					setCtrlString(CTL_DESKCMDAI_DVCSERIAL, Data.DvcSerial);
					setCtrlString(CTL_DESKCMDAI_PARAM, Data.CmdParam);
					SetupStrAssocCombo(this, CTLSEL_DESKCMDAI_COMMAND, cmd_list, Data.CmdID, 0, 0);
					AddClusterAssoc(CTL_DESKCMDAI_FLAGS, 0, PPDesktopAssocCmd::fSpecCode);
					AddClusterAssoc(CTL_DESKCMDAI_FLAGS, 1, PPDesktopAssocCmd::fSpecCodePrefx);
					AddClusterAssoc(CTL_DESKCMDAI_FLAGS, 2, PPDesktopAssocCmd::fNonInteractive);
					SetClusterData(CTL_DESKCMDAI_FLAGS, Data.Flags);
					SetupCtrls();
					return 1;
				}
				DECL_DIALOG_GETDTS()
				{
					int    ok = 1;
					uint   sel = 0, pos = 0;
					SString buf;
					GetClusterData(CTL_DESKCMDAI_FLAGS, &Data.Flags);
					sel = CTL_DESKCMDAI_COMMAND;
					getCtrlData(CTLSEL_DESKCMDAI_COMMAND, &Data.CmdID);
					THROW_PP(Data.CmdID, PPERR_INVCOMMAND);
					getCtrlString(sel = CTL_DESKCMDAI_CODE, Data.Code);
					THROW_PP(Data.Code.Len(), PPERR_INVSPECCODE);
					getCtrlString(CTL_DESKCMDAI_DVCSERIAL, Data.DvcSerial);
					getCtrlString(CTL_DESKCMDAI_PARAM, Data.CmdParam);
					ASSIGN_PTR(pData, Data);
					CATCH
						sel = (sel == CTL_DESKCMDAI_CODE && !(Data.Flags & PPDesktopAssocCmd::fSpecCode)) ? CTL_DESKCMDAI_COMMAND : sel;
						ok = (selectCtrl(sel), 0);
					ENDCATCH
					return ok;
				}
			private:
				DECL_HANDLE_EVENT
				{
					TDialog::handleEvent(event);
					if(event.isCmd(cmClusterClk) && event.isCtlEvent(CTL_DESKCMDAI_FLAGS)) {
						SetupCtrls();
					}
					else if(event.isCmd(cmWinKeyDown)) {
						long    flags = 0;
						GetClusterData(CTL_DESKCMDAI_FLAGS, &flags);
						if(!(flags & PPDesktopAssocCmd::fSpecCode)) {
							SString buf;
							const KeyDownCommand * p_cmd = static_cast<const KeyDownCommand *>(event.message.infoPtr);
							if(p_cmd && p_cmd->GetKeyName(buf, 1) > 0)
								p_cmd->GetKeyName(buf);
							setCtrlString(CTL_DESKCMDAI_CODE, buf);
						}
					}
					else
						return;
					clearEvent(event);
				}
				void SetupCtrls()
				{
					const long flags = GetClusterData(CTL_DESKCMDAI_FLAGS);
					disableCtrl(CTL_DESKCMDAI_CODE, !(flags & PPDesktopAssocCmd::fSpecCode));
					DisableClusterItem(CTL_DESKCMDAI_FLAGS, 1, !(flags & PPDesktopAssocCmd::fSpecCode));
				}
				const long Pos;
				const PPDesktopAssocCmdPool * P_CmdList;
			};
			DIALOG_PROC_BODY_P2ERR(DesktopAssocCommandDialog, pos, pCmdList, pAsscCmd)
		}
		StrAssocArray CmdList;
		PPCommandFolder::CommandGroupList DesktopList;
	};
	int    ok = -1;
	PPDesktopAssocCmdPool list;
	DesktopAssocCmdsDialog * p_dlg = 0;
	THROW(list.ReadFromProp(/*desktopID*/rDesktopUuid));
	THROW(CheckDialogPtr(&(p_dlg = new DesktopAssocCmdsDialog())));
	p_dlg->setDTS(&list);
	for(int valid_data = 0; !valid_data && ExecView(p_dlg) == cmOK;) {
		if(p_dlg->getDTS(&list) > 0) {
			THROW(list.WriteToProp(1));
			valid_data = ok = 1;
		}
	}
	CATCHZOKPPERR
	delete p_dlg;
	return ok;
}
//
// MenusDialog
//
int EditName(SString & rName)
{
	int    ok = -1;
	SString name = rName;
	SString org_name = name;
	PPInputStringDialogParam isd_param;
	PPLoadText(PPTXT_NEWLABEL, isd_param.InputTitle);
	if(name.C(0) == '@') {
		SString temp_buf;
		if(PPLoadString(name.ShiftLeft(), temp_buf) > 0) {
			name = temp_buf;
			org_name = name;
		}
	}
	if(InputStringDialog(&isd_param, name) > 0 && name.Len()) {
		if(name != org_name) {
			rName = name;
			ok = 1;
		}
	}
	return ok;
}

int EditCommandGroup(PPCommandGroup * pData, const S_GUID & rInitUuid, PPCommandGroupCategory kind)
{
	class EditMenusDlg : public PPListDialog {
		DECL_DIALOG_DATA(PPCommandGroup);
		enum {
			ctlgroupImg   = 1,
			ctlgroupBkgnd = 2
		};
	public:
		EditMenusDlg(PPCommandGroupCategory cmdgrpc, const S_GUID & rInitUuid) : PPListDialog(DLG_MENULIST, CTL_MENULIST_LIST), 
			IsMaster(PPMaster), CmdGrpC(cmdgrpc)
		{
			assert(oneof2(CmdGrpC, cmdgrpcMenu, cmdgrpcDesktop));
			SString title;
			uint   title_id = 0;
			if(CmdGrpC == cmdgrpcDesktop) {
				CurDict->GetDbSymb(DbSymb);
				showCtrl(CTL_MENULIST_EDMBTN, false);
				title_id = PPTXT_EDITDESKTOP;
				setSmartListBoxOption(CTL_MENULIST_LIST, lbtSelNotify);
				addGroup(ctlgroupImg, new ImageBrowseCtrlGroup(CTL_MENULIST_IMAGE, cmAddImage, cmDelImage, 1));
				addGroup(ctlgroupBkgnd, new ColorCtrlGroup(CTL_MENULIST_BKGND, CTLSEL_MENULIST_BKGND, cmSelBkgnd, CTL_MENULIST_SELBKGND));
				disableCtrls(!CheckRight(PPR_INS), CTL_MENULIST_EDASSCBTN, CTL_MENULIST_EDASSCCBTN, 0L);
			}
			else if(CmdGrpC == cmdgrpcMenu) {
				RECT   rect, img_rect;
				::GetWindowRect(H(), &rect);
				::GetWindowRect(::GetDlgItem(H(), CTL_MENULIST_IMAGE), &img_rect);
				rect.bottom -= (img_rect.bottom - img_rect.top);
				::MoveWindow(H(), rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, 1);
				showCtrl(CTL_MENULIST_EDASSCBTN,  false);
				showCtrl(CTL_MENULIST_EDASSCCBTN, false);
				showCtrl(CTL_MENULIST_IMAGE,      false);
				showCtrl(STDCTL_IMGADDBUTTON,     false);
				showCtrl(STDCTL_IMGDELBUTTON,     false);
				showCtrl(STDCTL_IMGPSTBUTTON,     false);
				showCtrl(CTL_MENULIST_BKGND,      false);
				showCtrl(CTLSEL_MENULIST_BKGND,   false);
				showCtrl(CTL_MENULIST_SELBKGND,   false);
				title_id = PPTXT_EDITMENU;
				setupPosition();
			}
			PPLoadText(title_id, title);
			setTitle(title);
			//PrevID = 0;
			//InitID = initID;
			PrevUuid.Z();
			InitUuid = rInitUuid;
			updateList(-1);
		}
		DECL_DIALOG_SETDTS()
		{
			PPID   cur_id = 0;
			RVALUEPTR(Data, pData);
			AddClusterAssoc(CTL_MENULIST_GRPFLAGS, 0, PPCommandItem::fNotUseDefDesktop);
			SetClusterData(CTL_MENULIST_GRPFLAGS, static_cast<long>(Data.Flags));
			AddClusterAssoc(CTL_MENULIST_FLAGS, 0, PPCommandItem::fBkgndGradient);
			if(!!InitUuid)
				updateListById(List.GetSurrIdByUuid(InitUuid));
			else
				updateList(-1);
			getSelection(&cur_id);
			LoadCfg(!!InitUuid ? InitUuid : List.GetUuidBySurrId(cur_id));
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			LoadCfg(ZEROGUID);
			Data.Flags = static_cast<int16>(GetClusterData(CTL_MENULIST_GRPFLAGS));
			ASSIGN_PTR(pData, Data);
			return 1;
		}
	private:
		DECL_HANDLE_EVENT
		{
			PPListDialog::handleEvent(event);
			long   id = 0;
			if(event.isCmd(cmEditMenu)) {
				if(P_Box && P_Box->getCurID(&id) > 0)
					if(!EditMenu(id))
						PPError();
			}
			else if(event.isCmd(cmEditAsscCmdCommonList) || event.isCmd(cmEditAsscCmdList)) {
				const int edit_by_desk = BIN(TVCMD == cmEditAsscCmdList);
				if(edit_by_desk)
					getSelection(&id);
				if(!edit_by_desk || edit_by_desk && id)
					PPDesktop::EditAssocCmdList(List.GetUuidBySurrId(id));
			}
			else if(event.isCmd(cmLBItemSelected)) {
				getSelection(&id);
				LoadCfg(List.GetUuidBySurrId(id));
			}
			else
				return;
			clearEvent(event);
		}
		virtual int setupList()
		{
			int    ok = 1;
			StrAssocArray items;
			List.Z();
			PPCommandFolder::GetCommandGroupList(&Data, CmdGrpC, List);
			List.GetStrAssocList(items);
			for(uint i = 0; i < items.getCount(); i++) {
				StrAssocArray::Item item = items.Get(i);
				StringSet ss(SLBColumnDelim);
				ss += item.Txt;
				THROW_SL(P_Box->addItem(item.Id, ss.getBuf()))
			}
			CATCHZOK
			return ok;
		}
		virtual int addItem(long * pPos, long * pID)
		{
			int    ok = -1;
			if(CheckRight(PPR_INS)) {
				S_GUID parent_uuid;
				long   resource_template_id = 0; // Если в качестве шаблона выбра ресурс меню, то он должен быть присвоен этой переменной
				SString name;
				while(ok < 0 && SelectCommandGroup(parent_uuid, &resource_template_id, &name, CmdGrpC, true/*asTemplate*/, &Data)) {
					const  PPCommandItem * p_item = Data.SearchByUuid(parent_uuid, 0);
					PPCommandGroup new_desk;
					PPCommandGroup new_menu2;
					PPCommandGroup * p_new_group = 0;
					if(CmdGrpC == cmdgrpcDesktop) {
						if(p_item && p_item->IsKind(PPCommandItem::kGroup)) {
							new_desk = *static_cast<const PPCommandGroup *>(p_item);
							new_desk.SetLogo(0);
						}
						new_desk.Type = cmdgrpcDesktop;
						new_desk.SetDbSymb(DbSymb);
						new_desk.ID = new_desk.GetUniqueID();
						p_new_group = &new_desk;
					}
					else if(CmdGrpC == cmdgrpcMenu) {
						if(p_item && p_item->IsKind(PPCommandItem::kFolder)) {
							new_menu2.PPCommandFolder::Copy(*static_cast<const PPCommandFolder *>(p_item));
							new_menu2.Kind = PPCommandItem::kGroup; // PPCommandFolder::Copy has changed Kind so we have to revert it
							new_menu2.Flags = p_item->Flags;
							new_menu2.Icon = p_item->Icon;
						}
						else if(resource_template_id > DEFAULT_MENUS_OFFS) {
							PPCommandFolder new_menu_folder;
							MenuResToMenu(resource_template_id - DEFAULT_MENUS_OFFS, &new_menu_folder);
							new_menu2.PPCommandFolder::Copy(new_menu_folder);
							new_menu2.Kind = PPCommandItem::kGroup; // PPCommandFolder::Copy has changed Kind so we have to revert it
							new_menu2.Flags = new_menu_folder.Flags;
							new_menu2.Icon = new_menu_folder.Icon;
						}
						new_menu2.Type = cmdgrpcMenu;
						new_menu2.ID = Data.GetUniqueID();
						{
							long uniq_id = new_menu2.GetID() + 1;
							new_menu2.SetUniqueID(&uniq_id);
						}
						new_menu2.DbSymb = "undefined";
						p_new_group = &new_menu2;
					}
					assert(p_new_group);
					if(p_new_group) {
						p_new_group->Name = name;
						p_new_group->GenerateGuid();
						if(Data.Add(-1, p_new_group))
							ok = 1;
						else
							PPErrorZ();
					}
				}
			}
			else
				ok = PPErrorZ();
			return ok;
		}
		virtual int editItem(long pos, long id)
		{
			int    ok = -1;
			if(CheckRight(PPR_MOD)) {
				uint ipos = 0;
				const S_GUID uuid = List.GetUuidBySurrId(id);
				const PPCommandItem * p_item = Data.SearchByUuid(uuid, &ipos);
				PPCommandItem * p_savitem = (p_item && p_item->IsKind(PPCommandItem::kGroup)) ? p_item->Dup() : 0;
				if(p_savitem) {
					while(ok < 0 && EditName(p_savitem->Name) > 0) {
						if(Data.Update(ipos, p_savitem)) {
							ok = 1;
						}
						else
							PPError();
					}
				}
				ZDELETE(p_savitem);
			}
			else
				ok = PPErrorZ();
			return ok;
		}
		virtual int delItem(long pos, long id)
		{
			int    ok = -1;
			uint   ipos = 0;
			SString str_guid;
			if(CheckRight(PPR_MOD)) {
				const S_GUID uuid = List.GetUuidBySurrId(id);
				const PPCommandItem * p_item = Data.SearchByUuid(uuid, &ipos);
				if(p_item) {
					const int is_confirmed = (/*IsDesktop*/CmdGrpC == cmdgrpcDesktop) ? CONFIRM(PPCFM_DELDESKTOP) : CONFIRM(PPCFM_DELMENU);
					if(is_confirmed) {
						if(IsMenuUsed(PPOBJ_USR, id/*, IsDesktop*/) || IsMenuUsed(PPOBJ_USRGRP, id/*, IsDesktop*/)) {
							ok = 0;
							if(CmdGrpC == cmdgrpcDesktop)
								PPErrCode = PPERR_DESKTOPBLOCKED;
							else
								PPErrCode = PPERR_MENUBLOCKED;
						}
						else {
							// @erik {
							const PPCommandGroup * p_cgroup = Data.GetGroup(CmdGrpC, uuid);
							if(p_cgroup && !!p_cgroup->GetGuid()/*.ToStr(S_GUID::fmtIDL, str_guid)*/) {
								// @v10.9.3 Возможно, здесь была ошибка: безусловная 1(desktop) не зависимо от значения CmdGrpC (ранее IsDesktop)
								PPCommandMngr * p_mgr = GetCommandMngr(PPCommandMngr::ctrfSkipObsolete, /*1*/CmdGrpC, 0); 
								if(p_mgr->DeleteGroupByUuid(CmdGrpC, p_cgroup->GetGuid())) {
									ok = Data.Remove(ipos);
								}
								delete p_mgr;
							}
							// } @erik 
						}
					}
				}
			}
			else
				ok = 0;
			if(!ok)
				PPError();
			return ok;
		}
		int EditMenu(long id)
		{
			int    ok = -1;
			uint   pos = 0;
			CommandsDialog * p_dlg = 0;
			const S_GUID uuid = List.GetUuidBySurrId(id);
			PPCommandItem * p_item = const_cast<PPCommandItem *>(Data.SearchByUuid(uuid, &pos)); // @badcast
			PPCommandGroup * p_edited_group = 0;
			PPCommandGroup * p_menus = 0;
			if(p_item) {
				/*if(p_item->Kind == PPCommandItem::kFolder) 
					p_folder = static_cast<PPCommandFolder *>(p_item->Dup());
				else*/
				if(p_item->IsKind(PPCommandItem::kGroup))
					p_edited_group = static_cast<PPCommandGroup *>(p_item);
			}
			if(p_edited_group) {
				p_menus = static_cast<PPCommandGroup *>(Data.Dup());
				THROW(CheckDialogPtr(&(p_dlg = new CommandsDialog(CmdGrpC))));
				p_dlg->setDTS(p_edited_group);
				if(ExecView(p_dlg) == cmOK) {
					if(p_dlg->getDTS(p_edited_group)) {
						//Data = *p_menus;
						ok = 1;
					}
				}
			}
			CATCHZOK
			ZDELETE(p_menus);
			delete p_dlg;
			return ok;
		}
		int    IsMenuUsed(PPID obj, PPID menuID/*, int isDesktop*/)
		{
			Reference * p_ref = PPRef;
			const S_GUID uuid = List.GetUuidBySurrId(menuID);
			int    used = (CmdGrpC == cmdgrpcDesktop) ? BIN(LConfig.DesktopUuid_ == uuid) : 0;
			for(PPID id = 0; !used && p_ref->EnumItems(obj, &id) > 0;) {
				PPConfig cfg;
				if(p_ref->GetProperty(obj, id, PPPRP_CFG, &cfg, sizeof(cfg)) > 0)
					used = (CmdGrpC == cmdgrpcDesktop) ? BIN(cfg.DesktopUuid_ == uuid) : BIN(cfg.MenuUuid == uuid);
			}
			return used;
		}
		int    LoadCfg(const S_GUID & rUuid)
		{
			if(CmdGrpC == cmdgrpcDesktop && PrevUuid != rUuid) {
				uint   pos = 0;
				long   flags = 0;
				ImageBrowseCtrlGroup::Rec rec;
				ColorCtrlGroup::Rec color_rec;
				PPCommandGroup * p_desk = 0;
				if(!!PrevUuid) {
					p_desk = Data.GetGroup(CmdGrpC, PrevUuid);
					if(p_desk) {
						GetClusterData(CTL_MENULIST_FLAGS, &flags);
						p_desk->Flags = static_cast<uint16>(flags);
						getGroupData(ctlgroupBkgnd, &color_rec);
						p_desk->Icon.Z().Cat(color_rec.C);
						getGroupData(ctlgroupImg, &rec);
						p_desk->SetLogo(rec.Path);
					}
				}
				rec.Path.Z();
				color_rec.C = 0;
				flags    = 0;
				PrevUuid = rUuid;
				p_desk   = Data.GetGroup(CmdGrpC, rUuid);
				if(p_desk) {
					rec.Path = p_desk->GetLogo();
					flags    = static_cast<long>(p_desk->Flags);
					color_rec.C = p_desk->Icon.ToLong();
				}
				setGroupData(ctlgroupImg, &rec);
				SetClusterData(CTL_MENULIST_FLAGS, flags);
				{
					color_rec.SetupStdColorList();
 					color_rec.C = (color_rec.C == 0) ? static_cast<long>(PPDesktop::GetDefaultBgColor()) : color_rec.C;
					setGroupData(ctlgroupBkgnd, &color_rec);
				}
			}
			{
				disableCtrls(!CheckRight(PPR_INS), CTL_MENULIST_FLAGS, CTL_MENULIST_IMAGE, CTLSEL_MENULIST_BKGND, CTL_MENULIST_SELBKGND, 0L);
				showCtrl(STDCTL_IMGADDBUTTON, CheckRight(PPR_INS));
				showCtrl(STDCTL_IMGPSTBUTTON, CheckRight(PPR_INS));
				showCtrl(STDCTL_IMGDELBUTTON, CheckRight(PPR_INS));
			}
			return 1;
		}
		bool   CheckRight(int rt) const { return (IsMaster || ObjRts.CheckDesktopID(0, rt)); }

		const  int IsMaster;
		const  PPCommandGroupCategory CmdGrpC;
		PPCommandFolder::CommandGroupList List;
		S_GUID InitUuid;
		S_GUID PrevUuid;
		SString DbSymb;
	};
	int    ok = -1;
	PPCommandGroup command_group;
	EditMenusDlg * p_dlg = new EditMenusDlg(kind, rInitUuid);
	PPCommandMngr * p_mgr = 0;
	THROW_MEM(p_dlg);
	THROW(CheckDialogPtr(&p_dlg));
	if(pData)
		command_group = *pData;
	else {
		THROW(p_mgr = GetCommandMngr(PPCommandMngr::ctrfSkipObsolete, kind, 0));
		THROW(p_mgr->Load__2(&command_group, 0, PPCommandMngr::fRWByXml)); //@erik v10.7.2
	}
	p_dlg->setDTS(&command_group);
	if(ExecView(p_dlg) == cmOK) {
		p_dlg->getDTS(&command_group);
		if(pData)
			*pData = command_group;
		else {
			command_group.Type = kind; // @v11.0.0
			THROW(p_mgr->Save__2(&command_group, PPCommandMngr::fRWByXml)); //@erik v10.7.2 Save__ => Save__2
		}
		ok = 1;
	}
	CATCHZOKPPERR
	delete p_dlg;
	ZDELETE(p_mgr);
	return ok;
}

int EditCommandGroupSingle(PPCommandGroup * pData)
{
	class CommandGroupSingleDialog : public PPListDialog {
		DECL_DIALOG_DATA(PPCommandGroup);
		enum {
			ctlgroupImg   = 1,
			ctlgroupBkgnd = 2
		};
	public:
		CommandGroupSingleDialog() : PPListDialog(DLG_CMDGROUP, CTL_CMDGROUP_CMDLIST), CmdGrpC(cmdgrpcUndef)
		{
			addGroup(ctlgroupImg, new ImageBrowseCtrlGroup(CTL_CMDGROUP_IMAGE, cmAddImage, cmDelImage, 1));
			addGroup(ctlgroupBkgnd, new ColorCtrlGroup(CTL_CMDGROUP_BGCOLOR, CTLSEL_CMDGROUP_BGCOLOR, cmSelBkgnd, CTL_CMDGROUP_SELBGCOLOR));
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			SString temp_buf;
			CmdGrpC = Data.Type;
			{
				if(CmdGrpC == cmdgrpcDesktop)
					PPLoadString("desktop", temp_buf);
				else if(CmdGrpC == cmdgrpcMenu)
					PPLoadString("menu", temp_buf);
				if(temp_buf.NotEmpty())
					setTitle(temp_buf);
			}
			if(Data.DbSymb.NotEmpty()) {
				temp_buf.Z().Cat("DB").CatDiv(':', 2).Cat(Data.DbSymb);
				setStaticText(CTL_CMDGROUP_ST_INFO, temp_buf);
			}
			setCtrlString(CTL_CMDGROUP_NAME, Data.Name);
			setCtrlLong(CTL_CMDGROUP_ID, Data.GetID());
			setCtrlString(CTL_CMDGROUP_UUID, temp_buf.Z().Cat(Data.Uuid, S_GUID::fmtIDL));
			AddClusterAssoc(CTL_CMDGROUP_FLAGS, 0, 0x0001);
			if(CmdGrpC == cmdgrpcDesktop) {
				ImageBrowseCtrlGroup::Rec img_rec;
				img_rec.Path = Data.GetLogo();
				setGroupData(ctlgroupImg, &img_rec);
				long flags = static_cast<long>(Data.Flags);
				SetClusterData(CTL_CMDGROUP_FLAGS, flags);
				{
					ColorCtrlGroup::Rec color_rec;
					color_rec.SetupStdColorList();
 					color_rec.C = Data.Icon.ToLong();
					setGroupData(ctlgroupBkgnd, &color_rec);
				}
			}
			else {
				disableCtrl(CTL_CMDGROUP_FLAGS, true);
				disableCtrl(CTL_CMDGROUP_IMAGE, true);
				disableCtrl(CTL_CMDGROUP_BGCOLOR, true);
				disableCtrl(CTLSEL_CMDGROUP_BGCOLOR, true);
				disableCtrl(CTL_CMDGROUP_SELBGCOLOR, true);
				enableCommand(cmAddImage, 0);
				enableCommand(cmDelImage, 0);
				enableCommand(cmPasteImage, 0);
				enableCommand(cmSelBkgnd, 0);
			}
			updateList(-1);
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			getCtrlString(CTL_CMDGROUP_NAME, Data.Name);
			if(CmdGrpC == cmdgrpcDesktop) {
				long   flags = 0;
				ColorCtrlGroup::Rec color_rec;
				ImageBrowseCtrlGroup::Rec img_rec;
				GetClusterData(CTL_CMDGROUP_FLAGS, &flags);
				Data.Flags = static_cast<uint16>(flags);
				getGroupData(ctlgroupBkgnd, &color_rec);
				Data.Icon.Z().Cat(color_rec.C);
				getGroupData(ctlgroupImg, &img_rec);
				Data.SetLogo(img_rec.Path);				
			}
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			PPListDialog::handleEvent(event);
		}
		virtual int setupList()
		{
			int    ok = -1;
			StrAssocArray * p_list = 0;
			StdTreeListBoxDef * p_def = 0;
			if(P_Box) {
				THROW_MEM(p_list = new StrAssocArray);
				THROW(Data.GetCommandList(p_list, 0));
				THROW_MEM(p_def = new StdTreeListBoxDef(p_list, lbtDisposeData|lbtDblClkNotify, 0));
				P_Box->setDef(p_def);
				P_Box->Draw_();
				ok = 1;
			}
			CATCH
				if(p_def)
					delete p_def;
				else
					delete p_list;
				ok = 0;
			ENDCATCH
			return ok;
		}
		virtual int addItem(long * pPos, long * pID)
		{
			int    ok = -1;
			long   parent_id = 0;
			PPCommandItem * p_new_item = 0;
			PPCommandItem new_sep(PPCommandItem::kSeparator);
			PPCommand new_cmd;
			PPCommandFolder new_cmdfolder;
			TDialog * p_dlg = 0;
			if(Data.Type == cmdgrpcDesktop) {
				if(EditCmdItem(&Data, &new_cmd, CmdGrpC) > 0)
					p_new_item = static_cast<PPCommandItem *>(&new_cmd);
			}
			else if(Data.Type == cmdgrpcMenu) {
				StrAssocArray cmd_list;
				long   v = 1;
				THROW(Data.GetCommandList(&cmd_list, 1));
				p_dlg = new TDialog(DLG_ADDCMD);
				THROW(CheckDialogPtr(&p_dlg));
				p_dlg->AddClusterAssocDef(CTL_ADDCMD_WHAT,  0, 1);
				p_dlg->AddClusterAssoc(CTL_ADDCMD_WHAT,  1, 2);
				p_dlg->AddClusterAssoc(CTL_ADDCMD_WHAT,  2, 3);
				p_dlg->SetClusterData(CTL_ADDCMD_WHAT, v);
				P_Box->getCurID(&parent_id);
				{
					long   parent_id2 = 0;
					const  PPCommandItem * p_selitem = Data.SearchByIDRecursive_Const(parent_id, &parent_id2);
					if(!p_selitem || !p_selitem->IsKind(PPCommandItem::kFolder))
						parent_id = parent_id2;
				}
				SetupStrAssocCombo(p_dlg, CTLSEL_ADDCMD_PARENT, cmd_list, parent_id, 0, 0);
				if(ExecView(p_dlg) == cmOK) {
					p_dlg->GetClusterData(CTL_ADDCMD_WHAT, &v);
					p_dlg->getCtrlData(CTLSEL_ADDCMD_PARENT, &parent_id);
					if(v == 1) {
						if(EditCmdItem(&Data, &new_cmd, CmdGrpC) > 0)
							p_new_item = static_cast<PPCommandItem *>(&new_cmd);
					}
					else if(v == 2) {
						if(EditName(new_cmdfolder.Name) > 0)
							p_new_item = static_cast<PPCommandItem *>(&new_cmdfolder);
					}
					else {
						new_sep.Name.Z().CatCharN('-', 40);
						p_new_item = &new_sep;
					}
				}
			}
			if(p_new_item) {
				uint p = 0;
				p_new_item->ID = Data.GetUniqueID();
				if(parent_id) {
					PPCommandItem * p_fi = Data.SearchByIDRecursive(parent_id, 0);
					PPCommandFolder * p_folder = (p_fi && p_fi->IsKind(PPCommandItem::kFolder)) ? static_cast<PPCommandFolder *>(p_fi) : 0;
					if(p_folder)
						THROW(p_folder->Add(-1, p_new_item));
				}
				else {
					THROW(Data.Add(-1, p_new_item));
				}
				{
					StrAssocArray new_cmd_list;
					THROW(Data.GetCommandList(&new_cmd_list, 0));
					new_cmd_list.Search(p_new_item->GetID(), &(p = 0));
					ASSIGN_PTR(pPos, p);
				}
				ASSIGN_PTR(pID,  p_new_item->GetID());
				ok = 1;
			}
			CATCHZOKPPERR
			delete p_dlg;
			return ok;
		}
		virtual int editItem(long pos, long id)
		{
			int    ok = -1;
			PPCommandItem * p_item = Data.SearchByIDRecursive(id, 0);
			if(p_item) {
				if(p_item->IsKind(PPCommandItem::kCommand))
					ok = EditCmdItem(&Data, static_cast<PPCommand *>(p_item), CmdGrpC);
				else if(oneof2(p_item->GetKind(), PPCommandItem::kFolder, PPCommandItem::kGroup))
					ok = EditName(p_item->Name);
			}
			return ok;
		}
		virtual int delItem(long pos, long id)
		{
			int    ok = -1;
			if(CONFIRM(PPCFM_DELITEM)) {
				long   parent_id = 0;
				const  PPCommandItem * p_item = Data.SearchByIDRecursive_Const(id, &parent_id);
				PPCommandFolder * p_folder = 0;
				if(p_item) {
					uint   p = 0;
					if(parent_id) {
						PPCommandItem * p_pitem = Data.SearchByIDRecursive(parent_id, 0);
						p_folder = (p_pitem && oneof2(p_pitem->GetKind(), PPCommandItem::kFolder, PPCommandItem::kGroup)) ? static_cast<PPCommandFolder *>(p_pitem) : 0;
					}
					else
						p_folder = &Data;
					if(p_folder && p_folder->SearchByID(id, &p))
						ok = p_folder->Remove(p);
				}
			}
			return ok;
		}
		virtual int moveItem(long pos, long id, int up)
		{
			int    ok = -1;
			if(id > 0) {
				long   pos = 0;
				PPCommandFolder * p_folder = 0;
				PPCommandItem * p_item = GetItem(id, &pos, &p_folder);
				if(p_folder) {
					const uint items_count = p_folder->GetCount();
					if(items_count > 1) {
						uint   p;
						uint   nb_pos = 0;
						PPID   mm = 0;
						const  PPCommandItem * p_citem = 0;
						for(p = 0; (p_citem = p_folder->Next(&p));) {
							if(up)
								mm = (p_citem->GetID() > mm && p_citem->GetID() < id) ? p_citem->GetID() : mm;
							else
								mm = ((!mm || p_citem->GetID() < mm) && p_citem->GetID() > id) ? p_citem->GetID() : mm;
						}
						p_citem = p_folder->SearchByID(mm, &nb_pos);
						if(p_citem) {
							PPCommandItem * p_nbitem = p_citem->Dup();
							if(p_nbitem) {
								const uint _upos = static_cast<uint>(pos);
								p_folder->Remove(_upos > nb_pos ? _upos : nb_pos);
								p_folder->Remove(_upos > nb_pos ? nb_pos : _upos);
								id = p_item->GetID();
								p_item->ID   = p_nbitem->GetID();
								p_nbitem->ID = id;
								p_folder->Add(-1, p_item);
								p_folder->Add(-1, p_nbitem);
								ZDELETE(p_nbitem);
								ok = 1;
							}
						}
					}
				}
				ZDELETE(p_item);
			}
			return ok;
		}
		PPCommandItem * GetItem(long id, long * pPos, PPCommandFolder ** ppFolder)
		{
			long   parent_id = 0;
			const  PPCommandItem * p_item = Data.SearchByIDRecursive_Const(id, &parent_id);
			PPCommandItem * p_retitem = 0;
			if(p_item) {
				uint   p = 0;
				if(parent_id) {
					PPCommandItem * p_pitem = Data.SearchByIDRecursive(parent_id, 0);
					(*ppFolder) = (p_pitem && oneof2(p_pitem->GetKind(), PPCommandItem::kFolder, PPCommandItem::kGroup)) ? static_cast<PPCommandFolder *>(p_pitem) : 0;
				}
				else
					(*ppFolder) = &Data;
				if(*ppFolder) {
					p_retitem = ((*ppFolder)->SearchByID(id, &p))->Dup();
					ASSIGN_PTR(pPos, p);
				}
			}
			return p_retitem;
		}
		PPCommandGroupCategory CmdGrpC;
	};
	DIALOG_PROC_BODY(CommandGroupSingleDialog, pData);
}
//
//
//
int SelectCommandGroup(S_GUID & rUuid, long * pResourceTemplateId, SString * pName, PPCommandGroupCategory kind, bool asTemplate, const PPCommandGroup * pGrp)
{
	int    ok = -1;
	long   resource_template_id = 0;
	SString db_symb, buf, left, right;
	StrAssocArray list;
	PPCommandFolder::CommandGroupList cgdata;
	PPCommandFolder::GetCommandGroupList(0, kind, cgdata);
	cgdata.GetStrAssocList(list);
	if(kind == cmdgrpcMenu && asTemplate) {
		TVRez * p_rez = P_SlRez;
		uint   locm_id = 0;
		PPLoadText(PPTXT_DEFAULTMENUS, buf);
		StringSet ss(';', buf);
		for(uint i = 0; ss.get(&i, buf);) {
			buf.Divide(',', left, right);
			list.Add(left.ToLong() + DEFAULT_MENUS_OFFS, right);
		}
		//fseek(p_rez->getStream(), 0, SEEK_SET);
		p_rez->Seek(0, SEEK_SET);
		for(ulong pos = 0; p_rez->enumResources(0x04, &locm_id, &pos) > 0;) {
			long _id = static_cast<long>(locm_id) + DEFAULT_MENUS_OFFS;
			if(list.GetText(_id, buf) <= 0)
				list.Add(_id, buf.Z().Cat(_id - DEFAULT_MENUS_OFFS));
		}
	}
	{
		// PPTXT_CMDPOOLSEL "Выберите рабочий стол,@{desktop};Выберите шаблон рабочего стола,Шаблон рабочего стола;Выберите меню,Меню;Выберите шаблон меню,Шаблон меню"
		int    text_index = -1;
		if(kind == cmdgrpcDesktop)
			text_index = (asTemplate ? 1 : 0);
		else if(kind == cmdgrpcMenu)
			text_index = (asTemplate ? 3 : 2);
		if(PPGetSubStr(PPTXT_CMDPOOLSEL, text_index, buf))
			buf.Divide(',', left, right);
		else {
			left.Z();
			right.Z();
		}
		long surr_id = 0;
		uint idx = 0;
		PPCommandFolder::CommandGroupList::Entry cgentry;
		if(cgdata.SearchByUuid(rUuid, &idx)) {
			cgdata.Get(idx, cgentry);
			surr_id = cgentry.SurrID;
		}
		THROW(ok = AdvComboBoxSelDialog(list, left, right, /*pID*/&surr_id, pName, 0));
		if(ok > 0 && surr_id) {
			if(surr_id > DEFAULT_MENUS_OFFS) {
				resource_template_id = surr_id;
				rUuid.Z();
				ok = 1;
			}
			else if(cgdata.SearchBySurrID(surr_id, &idx)) {
				cgdata.Get(idx, cgentry);
				rUuid = cgentry.Uuid;
				ok = 1;
			}
		}
	}
	CATCHZOK
	ASSIGN_PTR(pResourceTemplateId, resource_template_id);
	return ok;
}
//
// Load menus from resource
//
struct MITH {
	uint   versionNumber;
	uint   offset;
};

struct MIT {
	uint   mtOption;
	uint   mtID;
	char   mtString[256];
};

static int readMIT(TVRez & rez, MIT & mit, long ofs)
{
	if(rez.getStreamPos() >= ofs)
		return 0;
	else {
		mit.mtOption = rez.getUINT();
		if(!(mit.mtOption & MF_POPUP))
			mit.mtID = rez.getUINT();
		rez.getString(mit.mtString, 1);
		return 1;
	}
}

void readMenuRez(HMENU hm, TVRez * rez, long length)
{
	MIT  mit;
	SString menu_text;
	SString temp_buf;
	while(readMIT(*rez, mit, length)) {
		if(!mit.mtOption && !mit.mtID && !mit.mtString[0])
			AppendMenu(hm, MF_SEPARATOR, 0, 0);
		else {
			menu_text = mit.mtString;
			if(menu_text.Len() > 2 && menu_text[0] == '@') {
				if(PPLoadString(menu_text+1, temp_buf))
					menu_text = temp_buf.Transf(CTRANSF_INNER_TO_OUTER);
			}
			if(mit.mtOption & MF_POPUP) {
				HMENU hPopMenu = CreateMenu();
				AppendMenu(hm, (mit.mtOption | MF_STRING) & ~MF_END, reinterpret_cast<UINT_PTR>(hPopMenu), SUcSwitch(menu_text));
				readMenuRez(hPopMenu, rez, length);
			}
			else {
				AppendMenu(hm, (mit.mtOption | MF_STRING) & ~MF_END, mit.mtID, SUcSwitch(menu_text));
				if(mit.mtOption & MF_END)
					return;
			}
		}
	}
}

//void ReadMenu(HMENU hm, PPID parentID, PPCommandFolder * pMenu, StrAssocArray * pItems) //@erik v10.7.5
void ReadMenu(HMENU hm, PPID parentID, PPCommandGroup * pMenu, StrAssocArray * pItems) //@erik v10.7.5
{
	SString name;
	if(pMenu && pItems) {
		SString temp_buf;
		for(uint i = 0; i < pItems->getCount(); i++) {
			StrAssocArray::Item item_ = pItems->Get(i);
			if(item_.ParentId == parentID) {
				const PPCommandItem * p_item = pMenu->SearchByIDRecursive_Const(item_.Id, 0);
				if(p_item) {
					char   name_buf[256];
					name = p_item->Name;
					if(name.C(0) == '@' && PPLoadString(name.ShiftLeft(), temp_buf) > 0)
						name = temp_buf;
					name.Transf(CTRANSF_INNER_TO_OUTER);
					name.CopyTo(name_buf, sizeof(name_buf));
					if(p_item->IsKind(PPCommandItem::kFolder)) {
						HMENU h_pop_menu = CreateMenu();
						::AppendMenu(hm, MF_POPUP|MF_STRING, reinterpret_cast<UINT_PTR>(h_pop_menu), SUcSwitch(name_buf)); // @unicodeproblem
						ReadMenu(h_pop_menu, p_item->GetID(), pMenu, pItems); // @recursion
					}
					else if(p_item->IsKind(PPCommandItem::kSeparator))
						::AppendMenu(hm, MF_SEPARATOR, 0, 0);
					else {
						PPCommandDescr descr;
						descr.LoadResource(static_cast<const PPCommand *>(p_item)->CmdID);
						::AppendMenu(hm, MF_STRING, static_cast<UINT_PTR>(descr.MenuCm), SUcSwitch(name_buf)); // @unicodeproblem
					}
				}
			}
		}
	}
}

static void PostprocessLoadedMenu(HMENU hMenu)
{
	if(hMenu) {
		HMENU  h_popup = CreateMenu();
		SString temp_buf;
		PPLoadStringS("cmd_window", temp_buf).Transf(CTRANSF_INNER_TO_OUTER);
		::AppendMenu(hMenu, MF_POPUP | MF_STRING, (UINT)h_popup, SUcSwitch(temp_buf));
		// @v11.2.5 UserInterfaceSettings uiset;
		// @v11.2.5 uiset.Restore();
		PPLoadStringS("cmd_menutree", temp_buf).Transf(CTRANSF_INNER_TO_OUTER);
		// @v11.0.0 ::AppendMenu(h_popup, ((uiset.Flags & uiset.fShowLeftTree) ? MF_UNCHECKED : MF_CHECKED)|MF_STRING, cmShowTree, SUcSwitch(temp_buf));
		::AppendMenu(h_popup, MF_CHECKED|MF_STRING, cmShowTree, SUcSwitch(temp_buf)); // @v11.0.0 always checked
		PPLoadStringS("cmd_toolpane", temp_buf).Transf(CTRANSF_INNER_TO_OUTER);
		::AppendMenu(h_popup, MF_CHECKED | MF_STRING, cmShowToolbar, SUcSwitch(temp_buf));
	}
}

HMENU PPLoadCommandMenu(const S_GUID & rUuid, int * pNotFound)
{
	int    not_found = 1;
	HMENU  m = 0;
	StrAssocArray * p_items = 0;
	PPCommandGroup * p_menu = 0;
	if(!!rUuid) {
		PPCommandMngr * p_mgr = GetCommandMngr(PPCommandMngr::ctrfReadOnly, cmdgrpcMenu);
		PPCommandGroup menus;
		if(p_mgr && p_mgr->Load__2(&menus, 0, PPCommandMngr::fRWByXml)>0) { //@erik v10.7.5
			const PPCommandItem * p_item = menus.SearchByUuid(rUuid, 0);
			m = CreateMenu();
			//p_menu = (p_item && p_item->Kind == PPCommandItem::kFolder) ? static_cast<PPCommandFolder *>(p_item->Dup()) : 0; //@erik v10.7.5
			p_menu = (p_item && p_item->IsKind(PPCommandItem::kGroup)) ? static_cast<PPCommandGroup*>(p_item->Dup()) : 0;
			if(p_menu && p_menu->Type == cmdgrpcMenu) {
				p_items = new StrAssocArray;
				if(p_items && p_menu->GetCommandList(p_items, 0)) { // add p_menu->Type == PPCommandGroup::tMenu // @erik v10.7.6
					ReadMenu(m, 0, p_menu, p_items);
					not_found = 0;
				}
			}
		}
		ZDELETE(p_mgr);
	}
	PostprocessLoadedMenu(m);
	ASSIGN_PTR(pNotFound, not_found);
	ZDELETE(p_items);
	ZDELETE(p_menu);
	return m;
}

HMENU PPLoadResourceMenu(TVRez * pRez, long menuID, int * pNotFound)
{
	int    not_found = 1;
	HMENU  m = 0;
	StrAssocArray * p_items = 0;
	PPCommandGroup * p_menu = 0;
	if(pRez) {
		m = CreateMenu();
		MITH   mith;
		long   length, menuOfs;
		if(pRez->findResource(menuID, 0x04, &menuOfs, &length)) {
			length += menuOfs;
			mith.versionNumber = pRez->getUINT();
			mith.offset = pRez->getUINT();
			//fseek(pRez->getStream(), mith.offset, SEEK_CUR);
			pRez->Seek(mith.offset, SEEK_CUR);
			readMenuRez(m, pRez, length);
			not_found = 0;
		}
	}
	PostprocessLoadedMenu(m);
	ASSIGN_PTR(pNotFound, not_found);
	ZDELETE(p_items);
	ZDELETE(p_menu);
	return m;
}

HMENU PPLoadMenu(TVRez * rez, long menuID, int fromRc, int * pNotFound)
{
	int    not_found = 1;
	HMENU  m = 0;
	StrAssocArray * p_items = 0;
	PPCommandGroup * p_menu = 0;
	if(!fromRc && menuID) {
		PPCommandMngr * p_mgr = GetCommandMngr(PPCommandMngr::ctrfReadOnly, cmdgrpcMenu);
		PPCommandGroup menus;
		if(p_mgr && p_mgr->Load__2(&menus, 0, PPCommandMngr::fRWByXml)>0) { //@erik v10.7.5
			const PPCommandItem * p_item = menus.SearchByID(menuID, 0);
			m = CreateMenu();
			//p_menu = (p_item && p_item->Kind == PPCommandItem::kFolder) ? static_cast<PPCommandFolder *>(p_item->Dup()) : 0; //@erik v10.7.5
			p_menu = (p_item && p_item->IsKind(PPCommandItem::kGroup)) ? static_cast<PPCommandGroup*>(p_item->Dup()) : 0;
			if(p_menu && p_menu->Type == cmdgrpcMenu) {
				p_items = new StrAssocArray;
				if(p_items && p_menu->GetCommandList(p_items, 0)) { // add p_menu->Type == PPCommandGroup::tMenu // @erik v10.7.6
					ReadMenu(m, 0, p_menu, p_items);
					not_found = 0;
				}
			}
		}
		ZDELETE(p_mgr);
	}
	else if(rez != 0) {
		m = CreateMenu();
		MITH   mith;
		long   length, menuOfs;
		if(rez->findResource(menuID, 0x04, &menuOfs, &length)) {
			length += menuOfs;
			mith.versionNumber = rez->getUINT();
			mith.offset = rez->getUINT();
			//fseek(rez->getStream(), mith.offset, SEEK_CUR);
			rez->Seek(mith.offset, SEEK_CUR);
			readMenuRez(m, rez, length);
			not_found = 0;
		}
	}
	PostprocessLoadedMenu(m);
	ASSIGN_PTR(pNotFound, not_found);
	ZDELETE(p_items);
	ZDELETE(p_menu);
	return m;
}

void MenuResToMenu(PPCommandFolder * pFold, LAssocArray * pCmdDescrs, TVRez * rez, long length)
{
	MIT    mit;
	while(readMIT(*rez, mit, length)) {
		if(mit.mtOption & MF_POPUP) {
			PPCommandFolder fold;
			(fold.Name = mit.mtString).Transf(CTRANSF_OUTER_TO_INNER);
			MenuResToMenu(&fold, pCmdDescrs, rez, length); // @recursion
			pFold->Add(-1, static_cast<const PPCommandItem *>(&fold));
		}
		else if(!mit.mtOption && !mit.mtID && !mit.mtString[0])
			pFold->AddSeparator(-1);
		else {
			uint pos = 0;
			long menu_cm = mit.mtID;
			PPCommand cmd;
			(cmd.Name = mit.mtString).Transf(CTRANSF_OUTER_TO_INNER);
			if(pCmdDescrs->lsearch(&menu_cm, &pos, CMPF_LONG, sizeof(long)))
				cmd.CmdID = pCmdDescrs->at(pos).Key;
			pFold->Add(-1, static_cast<const PPCommandItem *>(&cmd));
			if(mit.mtOption & MF_END)
				return;
		}
	}
}

int MenuResToMenu(uint resMenuID, PPCommandFolder * pMenu)
{
	int    ok = -1;
	TVRez * p_slrez = P_SlRez;
	MITH   mith;
	long   length = 0;
	long   menu_ofs = 0;
	LAssocArray descrs;
	THROW(PPCommandDescr::GetResourceList(descrs));
	if(p_slrez->findResource(resMenuID, 0x04, &menu_ofs, &length)) {
		length += menu_ofs;
		mith.versionNumber = p_slrez->getUINT();
		mith.offset = p_slrez->getUINT();
		//fseek(p_slrez->getStream(), mith.offset, SEEK_CUR);
		p_slrez->Seek(mith.offset, SEEK_CUR);
		MenuResToMenu(pMenu, &descrs, p_slrez, length);
		ok = 1;
	}
	CATCHZOK
	return ok;
}
//
//
//
IMPLEMENT_PPFILT_FACTORY(UserMenu); UserMenuFilt::UserMenuFilt() : PPBaseFilt(PPFILT_USERMENU, 0, 0)
{
	SetFlatChunk(offsetof(UserMenuFilt, ReserveStart),
		offsetof(UserMenuFilt, Reserve)-offsetof(UserMenuFilt, ReserveStart)+sizeof(Reserve));
	SetBranchSString(offsetof(UserMenuFilt, DbSymb));
	Init(1, 0);
}

PPViewUserMenu::PPViewUserMenu() : PPView(0, &Filt, PPVIEW_USERMENU, implBrowseArray|implDontEditNullFilter, 0), P_DsList(0),
	P_MenuList(0), P_DesktopList(0)
{
}

PPViewUserMenu::~PPViewUserMenu()
{
	delete P_DsList;
	delete P_MenuList;
	delete P_DesktopList;
}

/*virtual*/int PPViewUserMenu::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	BExtQuery::ZDelete(&P_IterQuery);
	Counter.Init();
	CATCHZOK
	return ok;
}

/*virtual*/int PPViewUserMenu::EditBaseFilt(PPBaseFilt * pBaseFilt) { return -1; } // @stub

int PPViewUserMenu::CmpSortIndexItems(PPViewBrowser * pBrw, const BrwItem * pItem1, const BrwItem * pItem2)
{
	return Implement_CmpSortIndexItems_OnArray(pBrw, pItem1, pItem2);
}

static IMPL_CMPFUNC(PPViewUserMenuBrwItem, i1, i2)
{
	int    si = 0;
	PPViewBrowser * p_brw = static_cast<PPViewBrowser *>(pExtraData);
	if(p_brw) {
		PPViewUserMenu * p_view = static_cast<PPViewUserMenu *>(p_brw->P_View);
		if(p_view) {
			const PPViewUserMenu::BrwItem * p_item1 = static_cast<const PPViewUserMenu::BrwItem *>(i1);
			const PPViewUserMenu::BrwItem * p_item2 = static_cast<const PPViewUserMenu::BrwItem *>(i2);
			si = p_view->CmpSortIndexItems(p_brw, p_item1, p_item2);
		}
	}
	return si;
}

int PPViewUserMenu::MakeList(PPViewBrowser * pBrw)
{
	int    ok = 1;
	SString temp_buf;
	const  int is_sorting_needed = BIN(pBrw && pBrw->GetSettledOrderList().getCount());
	ZDELETE(P_MenuList);
	ZDELETE(P_DesktopList);
	if(P_DsList)
		P_DsList->clear();
	else
		P_DsList = new SArray(sizeof(BrwItem));
	//
	uint cmd_mgr_flags = PPCommandMngr::ctrfReadOnly/*|PPCommandMngr::ctrfSkipObsolete*/;
	if(!Filt.Kind || Filt.Kind == Filt.kMenu) {
		PPCommandMngr * p_mgr = GetCommandMngr(cmd_mgr_flags, /*0*/cmdgrpcMenu);
		if(p_mgr) {
			P_MenuList = new PPCommandGroup;
			if(p_mgr->Load__2(P_MenuList, Filt.DbSymb, PPCommandMngr::fRWByXml) > 0) {
				const PPCommandItem * p_item = 0;
				for(uint i = 0; (p_item = P_MenuList->Next(&i)) != 0;) {
					if(p_item->IsKind(PPCommandItem::kGroup)) {
						const PPCommandGroup * p_group = static_cast<const PPCommandGroup *>(p_item);
						BrwItem entry;
						MEMSZERO(entry);
						entry.ID = p_group->GetID();
						entry.Uuid = p_group->Uuid;
						entry.Kind = UserMenuFilt::kMenu;
						STRNSCPY(entry.Name, p_group->Name);
						STRNSCPY(entry.DbSymb, p_group->DbSymb);
						P_DsList->insert(&entry);
					}
				}
			}
			ZDELETE(p_mgr);
		}
		{
			TVRez * p_rez = P_SlRez;
			if(p_rez) {
				uint   locm_id = 0;
				SString left_buf, right_buf;
				PPLoadText(PPTXT_DEFAULTMENUS, temp_buf);
				StringSet ss_defmenu(';', temp_buf);
				/*
				{
					for(uint i = 0; ss.get(&i, temp_buf) > 0;) {
						temp_buf.Divide(',', left, right);
						list.Add(left.ToLong() + DEFAULT_MENUS_OFFS, right);
					}
					if(list.GetText(_id, buf) <= 0)
						list.Add(_id, buf.Z().Cat(_id - DEFAULT_MENUS_OFFS));
				}
				*/
				//fseek(p_rez->getStream(), 0, SEEK_SET);
				p_rez->Seek(0, SEEK_SET);
				for(ulong pos = 0; p_rez->enumResources(0x04, &locm_id, &pos) > 0;) {
					long _id = static_cast<long>(locm_id) + DEFAULT_MENUS_OFFS;
					{
						PPCommandFolder new_menu_folder;
						MenuResToMenu(locm_id, &new_menu_folder);
						BrwItem entry;
						MEMSZERO(entry);
						entry.ID = _id;
						entry.Uuid.Z();
						entry.Kind = UserMenuFilt::kMenu;
						entry.Flags |= entry.fReservedMenu;
						for(uint i = 0; ss_defmenu.get(&i, temp_buf);) {
							temp_buf.Divide(',', left_buf, right_buf);
							if(left_buf.ToLong() == static_cast<long>(locm_id)) {
								STRNSCPY(entry.Name, right_buf);
								break;
							}
						}
						if(isempty(entry.Name))
							STRNSCPY(entry.Name, new_menu_folder.Name);
						STRNSCPY(entry.DbSymb, "undefined");
						P_DsList->insert(&entry);
					}
				}
			}
		}
	}
	if(!Filt.Kind || Filt.Kind == Filt.kDesktop) {
		PPCommandMngr * p_mgr = GetCommandMngr(cmd_mgr_flags, /*1*/cmdgrpcDesktop);
		if(p_mgr) {
			P_DesktopList = new PPCommandGroup;
			if(p_mgr->Load__2(P_DesktopList, Filt.DbSymb, PPCommandMngr::fRWByXml) > 0) {
				const PPCommandItem * p_item = 0;
				for(uint i = 0; (p_item = P_DesktopList->Next(&i)) != 0;) {
					if(p_item->IsKind(PPCommandItem::kGroup)) {
						const PPCommandGroup * p_group = static_cast<const PPCommandGroup *>(p_item);
						BrwItem entry;
						MEMSZERO(entry);
						entry.ID = p_group->GetID();
						entry.Uuid = p_group->Uuid;
						entry.Kind = UserMenuFilt::kDesktop;
						STRNSCPY(entry.Name, p_group->Name);
						STRNSCPY(entry.DbSymb, p_group->DbSymb);
						P_DsList->insert(&entry);
					}
				}
			}
			ZDELETE(p_mgr);
		}
	}
	if(pBrw) {
		pBrw->Helper_SetAllColumnsSortable();
		if(is_sorting_needed) {
			P_DsList->sort(PTR_CMPFUNC(PPViewUserMenuBrwItem), pBrw);
		}
	}
	//CATCHZOK
	return ok;
}

int PPViewUserMenu::_GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	int    ok = 1;
	assert(pBlk->P_SrcData && pBlk->P_DestData); // Функция вызывается только из одной локации и эти members != 0 равно как и pBlk != 0
	const  BrwItem * p_item = static_cast<const BrwItem *>(pBlk->P_SrcData);
	int    r = 0;
	/*
		"ID",           0, int32,        0, 10, BCO_USERPROC
		"UUID",         1, int32,        0, 10, BCO_USERPROC
		"Kind",         2, zstring(20),  0, 10, BCO_USERPROC
		"DbSymb",       3, zstring(48),  0, 10, BCO_USERPROC
		"Name",         4, zstring(64),  0, 10, BCO_USERPROC
	*/
	switch(pBlk->ColumnN) {
		case 0: pBlk->Set(p_item->ID); break; // @id
		case 1: // @uuid
			p_item->Uuid.ToStr(S_GUID::fmtIDL, pBlk->TempBuf);
			pBlk->Set(pBlk->TempBuf);
			break; 
		case 2: // @kind
			{
				const char * p_tsign = 0;
				if(p_item->Kind == UserMenuFilt::kDesktop)
					pBlk->TempBuf = "desktop";
				else if(p_item->Kind == UserMenuFilt::kMenu)
					pBlk->TempBuf = "menu";
				else
					pBlk->TempBuf.Z();
				if(p_item->Flags & p_item->fReservedMenu) {
					pBlk->TempBuf.Space().Cat("reserved");
				}
				pBlk->Set(pBlk->TempBuf);
			}
			break; 
		case 3: // dbSymb
			pBlk->TempBuf = p_item->DbSymb;
			pBlk->Set(pBlk->TempBuf);
			break; 
		case 4: // name
			pBlk->TempBuf = p_item->Name;
			pBlk->Set(pBlk->TempBuf);
			break; 
	}
	return ok;
}

void PPViewUserMenu::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw) {
		pBrw->SetDefUserProc([](SBrowserDataProcBlock * pBlk) -> int
			{
				return (pBlk && pBlk->ExtraPtr) ? static_cast<PPViewUserMenu *>(pBlk->ExtraPtr)->_GetDataForBrowser(pBlk) : 0;				
			}, this);
		//pBrw->SetCellStyleFunc(CellStyleFunc, pBrw);
		pBrw->Helper_SetAllColumnsSortable(); // @v11.0.0
	}
	//pBrw->Advise(PPAdviseBlock::evSysJournalChanged, PPACN_EVENTDETECTION, PPOBJ_EVENTSUBSCRIPTION, 0);
}

SArray * PPViewUserMenu::CreateBrowserArray(uint * pBrwId, SString * pSubTitle)
{
	SArray * p_array = 0;
	THROW(MakeList(0));
	p_array = new SArray(*P_DsList);
	CATCH
		ZDELETE(P_DsList);
	ENDCATCH
	ASSIGN_PTR(pBrwId, BROWSER_USERMENU);
	return p_array;
}

int PPViewUserMenu::AddItem(PPCommandGroupCategory kind, const S_GUID & rSampleUuid, long sampleId, S_GUID * pUuid)
{
	int    ok = -1;
	S_GUID result_uuid;
	PPCommandGroup * p_new_entry = 0;
	if(!!rSampleUuid) {
		const PPCommandGroup * p_sample_entry = GetEntryByUuid(rSampleUuid);
		THROW(p_sample_entry);
		kind = p_sample_entry->Type;
		p_new_entry = new PPCommandGroup(*p_sample_entry);
		p_new_entry->Uuid.Z().Generate();
		p_new_entry->Name.Z();
		p_new_entry->ID = 0;
	}
	else if(sampleId > DEFAULT_MENUS_OFFS) {
		PPCommandFolder reserved_menu_folder;
		THROW(MenuResToMenu(sampleId - DEFAULT_MENUS_OFFS, &reserved_menu_folder) > 0);
		p_new_entry = new PPCommandGroup(cmdgrpcMenu, 0, 0);
		p_new_entry->PPCommandFolder::Copy(reserved_menu_folder);
		p_new_entry->Kind = PPCommandItem::kGroup; // PPCommandFolder::Copy has changed Kind so we have to revert it
		p_new_entry->DbSymb = "undefined";
		p_new_entry->Flags = reserved_menu_folder.Flags;
		p_new_entry->Icon = reserved_menu_folder.Icon;
		{
			assert(p_new_entry->Type == cmdgrpcMenu);
			PPCommandMngr * p_mgr = GetCommandMngr(PPCommandMngr::ctrfSkipObsolete, p_new_entry->Type, 0); 
			long   max_id = 0;
			if(p_mgr && p_mgr->GetMaxEntryID(&max_id) > 0) {
				long   uniq_id = max_id+1;
				p_new_entry->ID = uniq_id;
				p_new_entry->SetUniqueID(&uniq_id);
			}
			delete p_mgr;
		}
	}
	else {
		if(!oneof2(kind, cmdgrpcDesktop, cmdgrpcMenu)) {
			uint   val = 0;
			if(SelectorDialog(DLG_SELNEWCMDGROUP, CTL_SELNEWCMDGROUP_SEL, &val) > 0) {
				if(val == 0)
					kind = cmdgrpcMenu;
				else if(val == 1)
					kind = cmdgrpcDesktop;
			}
		}
		if(kind == cmdgrpcDesktop)
			p_new_entry = new PPCommandGroup(cmdgrpcDesktop, 0, 0);
		else if(kind == cmdgrpcMenu)
			p_new_entry = new PPCommandGroup(cmdgrpcMenu, 0, 0);
	}
	if(p_new_entry) {
		ok = EditCommandGroupSingle(p_new_entry);
		if(ok > 0) {
			PPCommandMngr * p_mgr = GetCommandMngr(PPCommandMngr::ctrfSkipObsolete, p_new_entry->Type, 0); 
			if(p_mgr) {
				if(p_new_entry->GetID() == 0) {
					long  max_id = 0;
					if(p_mgr->GetMaxEntryID(&max_id) > 0)
						p_new_entry->ID = max_id+1;
				}
				if(!p_mgr->Save__2(p_new_entry, PPCommandMngr::fRWByXml))
					ok = PPErrorZ();
				else {
					result_uuid = p_new_entry->Uuid;
				}
				ZDELETE(p_mgr);
			}
		}		
	}
	CATCHZOK
	ASSIGN_PTR(pUuid, result_uuid);
	return ok;
}

PPCommandGroup * PPViewUserMenu::GetEntryByUuid(const S_GUID & rUuid)
{
	PPCommandGroup * p_data = 0;
	if(!p_data && P_MenuList) {
		p_data = P_MenuList->GetGroup(cmdgrpcUndef, rUuid);
		if(p_data)
			p_data->Type = cmdgrpcMenu;
		//assert(!p_data || p_data->Type == cmdgrpcMenu);
	}
	if(!p_data && P_DesktopList) {
		p_data = P_DesktopList->GetGroup(cmdgrpcUndef, rUuid);
		if(p_data)
			p_data->Type = cmdgrpcDesktop;
		//assert(!p_data || p_data->Type == cmdgrpcDesktop);
	}
	return p_data;
}

int PPViewUserMenu::EditItem(const S_GUID & rUuid)
{
	int    ok = -1;
	PPCommandGroup * p_data = GetEntryByUuid(rUuid);
	if(p_data) {
		ok = EditCommandGroupSingle(p_data);
		if(ok > 0) {
			PPCommandMngr * p_mgr = GetCommandMngr(PPCommandMngr::ctrfSkipObsolete, p_data->Type, 0); 
			if(p_mgr) {
				if(!p_mgr->Save__2(p_data, PPCommandMngr::fRWByXml))
					ok = PPErrorZ();
				ZDELETE(p_mgr);
			}
		}
	}
	return ok;
}

int PPViewUserMenu::DeleteItem(const S_GUID & rUuid)
{
	int    ok = -1;
	if(!!rUuid) {
		PPCommandGroup * p_data = GetEntryByUuid(rUuid);
		if(p_data && CONFIRM(PPCFM_DELETE)) {
			PPCommandMngr * p_mgr = GetCommandMngr(PPCommandMngr::ctrfSkipObsolete, p_data->Type, 0);				
			if(p_mgr) {
				if(p_mgr->DeleteGroupByUuid(p_data->Type, rUuid))
					ok = 1;
				else
					ok = PPErrorZ();
			}
		}
	}
	return ok;
}

/*virtual*/int PPViewUserMenu::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		const BrwItem * p_item = static_cast<const BrwItem *>(pHdr);
		ok = -1;
		S_GUID new_uuid;
		switch(ppvCmd) {
			case PPVCMD_USERSORT: ok = 1; break; // @v11.0.0 The rest will be done below
			case PPVCMD_ADDITEM:
				ok = AddItem(cmdgrpcUndef, ZEROGUID, 0, &new_uuid);
				break;
			case PPVCMD_DESKTOPADD:
				ok = AddItem(cmdgrpcDesktop, ZEROGUID, 0, &new_uuid);
				break;
			case PPVCMD_MENUADD:
				ok = AddItem(cmdgrpcMenu, ZEROGUID, 0, &new_uuid);
				break;
			case PPVCMD_ADDBYSAMPLE:
				if(p_item && (!!p_item->Uuid || p_item->ID > DEFAULT_MENUS_OFFS)) {
					ok = AddItem(cmdgrpcUndef, p_item->Uuid, p_item->ID, &new_uuid);
				}
				break;
			case PPVCMD_EDITITEM:
				if(p_item)
					ok = EditItem(p_item->Uuid);
				break;
			case PPVCMD_DELETEITEM:
				if(p_item)
					ok = DeleteItem(p_item->Uuid);
				break;
		}
		if(ok > 0) {
			AryBrowserDef * p_def = static_cast<AryBrowserDef *>(pBrw->getDef());
			if(p_def) {
				if(!!new_uuid) {
					if(MakeList(pBrw)) {
						long id_to_locate = 0;
						for(uint i = 0; i < P_DsList->getCount(); i++) {
							const BrwItem * p_iter_item = static_cast<const BrwItem *>(P_DsList->at(i));
							if(p_iter_item->Uuid == new_uuid)
								id_to_locate = p_iter_item->ID;
						}
						p_def->setArray(new SArray(*P_DsList), 0, 1);
						pBrw->search2(&id_to_locate, CMPF_LONG, srchFirst, 0);
					}
				}
				else if(ppvCmd == PPVCMD_EDITITEM) {
					assert(p_item);
					const long preserve_id = p_item->ID;
					if(MakeList(pBrw)) {
						p_def->setArray(new SArray(*P_DsList), 0, 1);
						pBrw->search2(&preserve_id, CMPF_LONG, srchFirst, 0);
					}
				}
				else if(ppvCmd == PPVCMD_DELETEITEM) {
					if(MakeList(pBrw)) {
						p_def->setArray(new SArray(*P_DsList), 0, 0);
					}
				}
				else if(ppvCmd == PPVCMD_USERSORT) {
					const long preserve_id = p_item ? p_item->ID : 0;
					MemLeakTracer mlt;
					if(MakeList(pBrw)) {
						p_def->setArray(new SArray(*P_DsList), 0, 0);
						if(preserve_id)
							pBrw->search2(&preserve_id, CMPF_LONG, srchFirst, 0);
					}
				}
			}
		}
	}
	return ok;
}
