// V_DLG.CPP
// Copyright (c) A.Sobolev 2011, 2016, 2018
//
#include <pp.h>
#pragma hdrstop

// @construction

int EditDialogSpec(DlContext * pCtx, uint dlgId);

//
//
//
IMPLEMENT_PPFILT_FACTORY(Dialog); SLAPI DialogFilt::DialogFilt() : PPBaseFilt(PPFILT_DIALOG, 0, 0)
{
	SetFlatChunk(offsetof(DialogFilt, ReserveStart),
		offsetof(DialogFilt, ReserveEnd)-offsetof(DialogFilt, ReserveStart)+sizeof(ReserveEnd));
	SetBranchSString(offsetof(DialogFilt, DlFileName));
	SetBranchSString(offsetof(DialogFilt, Serial));
	SetBranchSString(offsetof(DialogFilt, SubText));
	Init(1, 0);
}

SLAPI PPViewDialog::PPViewDialog() : PPView(0, &Filt, PPVIEW_DIALOG)
{
	ImplementFlags |= implBrowseArray;
}

SLAPI PPViewDialog::~PPViewDialog()
{
}

int SLAPI PPViewDialog::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	return 1;
}

int SLAPI PPViewDialog::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	SString file_name, text_buf;
	StrAssocArray temp_list;
	char   c_buf[1024]; // Буфер для извлечения констант
	THROW(Helper_InitBaseFilt(pBaseFilt));
	file_name = Filt.DlFileName;
	if(!file_name.NotEmptyS()) {
		file_name = "D:/PAPYRUS/SRC/RSRC/DL600/PPDLG.BIN"; // @debug
	}
	List.freeAll();
	THROW(Ctx.Init(file_name));
	Ctx.GetDialogList(&temp_list);
	temp_list.SortByText();
	for(uint i = 0; i < temp_list.getCount(); i++) {
		StrAssocArray::Item t = temp_list.Get(i);
		const DlScope * p_scope = Ctx.GetScope_Const(t.Id, DlScope::kUiDialog);
		if(p_scope) {
			DialogViewItem item;
			MEMSZERO(item);
			item.Id = t.Id;
			STRNSCPY(item.Symb, t.Txt);
			{
				text_buf.Z();
				if(Ctx.GetConstData(p_scope->GetConst(DlScope::cuifCtrlText), c_buf, sizeof(c_buf)))
					text_buf = (const char *)c_buf;
				text_buf.ToOem().CopyTo(item.Text, sizeof(item.Text));
				//
				text_buf.Z();
				if(Ctx.GetConstData(p_scope->GetConst(DlScope::cuifSymbSeries), c_buf, sizeof(c_buf)))
					text_buf = (const char *)c_buf;
				text_buf.CopyTo(item.Serial, sizeof(item.Serial));
			}
			THROW_SL(List.insert(&item));
		}
	}
	CATCHZOK
	return ok;
}

SArray * SLAPI PPViewDialog::CreateBrowserArray(uint * pBrwId, SString * pSubTitle)
{
	SArray * p_array = new SArray(List);
	uint   brw_id = BROWSER_DIALOG;
	ASSIGN_PTR(pBrwId, brw_id);
	return p_array;
}

int SLAPI PPViewDialog::Detail(const void * pHdr, PPViewBrowser * pBrw)
{
	PPID   id = pHdr ? *(PPID *)pHdr : 0;
	if(id) {
		EditDialogSpec(&Ctx, id);
	}
	return -1;
}

int SLAPI PPViewDialog::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	/*
	if(ok == -2) {
		PPID   id = pHdr ? *(PPID *)pHdr : 0;
		switch(ppvCmd) {
		}
	}
	*/
	return ok;
}
//
//
//
class WhatmanObjectUiCtrl : public TWhatmanObject {
public:
	struct ToolItemExtData {
		int32  UiKindID;
	};
	WhatmanObjectUiCtrl();
	~WhatmanObjectUiCtrl();
	int    Set(DlContext * pCtx, DlScope * pScope);
	int    Set(DlContext * pCtx, DlScope * pScope, DlScope * pInnerScope, uint fldId);
	int    SetNew(DlContext * pCtx, DlScope * pScope, uint uiKind);
protected:
	void   Init();
	virtual TWhatmanObject * Dup() const;
	//virtual int Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
	virtual int Draw(TCanvas2 & rCanv);
	virtual int HandleCommand(int cmd, void * pExt);

	int    FASTCALL Copy(const WhatmanObjectUiCtrl & rS);
	int    SetKind(uint kind);
	int    CreateTextLayout(SPaintToolBox & rTb, STextLayout & rTlo, int adj = 0);
	//
	uint32 UiKind;
	DlContext * P_DlCtx; // @notowned @transient
	DlScope * P_Scope;   // @notowned @transient Диалог, владеющий данным элементом.
	SdbField   F;        //
	UiRelRect Rect;
	SString UiText;
	WhatmanObjectUiCtrl * P_LinkObj;

	int    TidFont;
};

IMPLEMENT_WTMOBJ_FACTORY(UiCtrl, "@wtmo_uictrl");

void WhatmanObjectUiCtrl::Init()
{
	Options |= (oMovable | oResizable | oMultSelectable);
	P_DlCtx = 0;
	P_Scope = 0;
	F.Init();
	SetKind(UiItemKind::kUnkn);
	P_LinkObj = 0;
	TidFont = 0;
	Rect.Reset();
}

WhatmanObjectUiCtrl::WhatmanObjectUiCtrl() : TWhatmanObject("UiCtrl")
{
	Init();
}

WhatmanObjectUiCtrl::~WhatmanObjectUiCtrl()
{
}

int WhatmanObjectUiCtrl::SetKind(uint kind)
{
	int    ok = 1;
	Options &= ~oFrame;
	Options |= oMultSelectable;
	switch(kind) {
		case UiItemKind::kUnkn:
			break;
		case UiItemKind::kDialog:
			Options |= oFrame;
			Options &= ~oMultSelectable;
			break;
		case UiItemKind::kInput:
			break;
		case UiItemKind::kStatic:
			break;
		case UiItemKind::kPushbutton:
			break;
		case UiItemKind::kCheckbox:
			break;
		case UiItemKind::kRadiobutton:
			break;
		case UiItemKind::kRadioCluster:
			Options |= oFrame;
			break;
		case UiItemKind::kCheckCluster:
			Options |= oFrame;
			break;
		case UiItemKind::kCombobox:
			break;
		case UiItemKind::kListbox:
			break;
		case UiItemKind::kTreeListbox:
			break;
		case UiItemKind::kFrame:
			Options |= oFrame;
			break;
		case UiItemKind::kLabel:
			break;
	}
	UiKind = kind;
	return ok;
}

int WhatmanObjectUiCtrl::Set(DlContext * pCtx, DlScope * pScope)
{
	return Set(pCtx, pScope, 0, 0);
}

int WhatmanObjectUiCtrl::Set(DlContext * pCtx, DlScope * pScope, DlScope * pInnerScope, uint fldId)
{
	int    ok = 1;
	char   c_buf[1024]; // Буфер для извлечения констант
	Init();
	P_DlCtx = pCtx;
	P_Scope = pScope;
	if(P_DlCtx && P_Scope) {
		DlScope * p_scope = NZOR(pInnerScope, pScope);
		if(fldId != 0 && fldId != UINT_MAX) {
			THROW_SL(p_scope->GetFieldByID(fldId, 0, &F));
			if(P_DlCtx->GetConstData(p_scope->GetFldConst(fldId, DlScope::cuifCtrlKind), c_buf, sizeof(c_buf)))
				SetKind(*(uint32 *)c_buf);
			if(P_DlCtx->GetConstData(p_scope->GetFldConst(fldId, DlScope::cuifCtrlText), c_buf, sizeof(c_buf)))
				UiText = (const char *)c_buf;
			if(P_DlCtx->GetConstData(p_scope->GetFldConst(fldId, DlScope::cuifCtrlRect), c_buf, sizeof(c_buf)))
				Rect = *(UiRelRect *)c_buf;
		}
		else if(fldId == 0) {
			SetKind(UiItemKind::kDialog);
			if(P_DlCtx->GetConstData(p_scope->GetConst(DlScope::cuifCtrlText), c_buf, sizeof(c_buf)))
				UiText = (const char *)c_buf;
			if(P_DlCtx->GetConstData(p_scope->GetConst(DlScope::cuifCtrlRect), c_buf, sizeof(c_buf)))
				Rect = *(UiRelRect *)c_buf;
		}
		{
			TRect bounds; // UiRelRect
			bounds.a.x = Rect.L.X.Val;
			bounds.a.y = Rect.L.Y.Val;
			bounds.b.x = Rect.R.X.Val;
			bounds.b.y = Rect.R.Y.Val;
			SetBounds(bounds);
		}
	}
	CATCHZOK
	return ok;
}

int WhatmanObjectUiCtrl::SetNew(DlContext * pCtx, DlScope * pScope, uint uiKind)
{
	int    ok = 1;
	Init();
	P_DlCtx = pCtx;
	P_Scope = pScope;
	if(P_DlCtx && P_Scope)
		SetKind(uiKind);
	/*
	CATCHZOK
	*/
	return ok;
}

int FASTCALL WhatmanObjectUiCtrl::Copy(const WhatmanObjectUiCtrl & rS)
{
	int    ok = 1;
	TWhatmanObject::Copy(rS);
	UiKind = rS.UiKind;
	P_DlCtx = rS.P_DlCtx;
	P_Scope = rS.P_Scope;
	F = rS.F;
	P_LinkObj = rS.P_LinkObj;
	Rect = rS.Rect;
	UiText = rS.UiText;
	return ok;
}

TWhatmanObject * WhatmanObjectUiCtrl::Dup() const
{
	WhatmanObjectUiCtrl * p_obj = new WhatmanObjectUiCtrl();
	if(p_obj)
		p_obj->Copy(*this);
	return p_obj;
}

int WhatmanObjectUiCtrl::CreateTextLayout(SPaintToolBox & rTb, STextLayout & rTlo, int adj)
{
	int    ok = -1;
	rTlo.Reset();
	if(UiText.NotEmpty()) {
		SString temp_buf;
		char   c_buf[1024]; // Буфер для извлечения констант из DlScope
		if(!TidFont) {
			SString font_face;
			if(P_DlCtx && P_Scope) {
				if(P_DlCtx->GetConstData(P_Scope->GetConst(DlScope::cuifFont), c_buf, sizeof(c_buf))) {
					if(((const char *)c_buf)[0])
						temp_buf = (const char *)c_buf;
				}
			}
			temp_buf.SetIfEmpty("MS Sans Serif(8)");
			SFontDescr fd(0, 0, 0);
			fd.FromStr(temp_buf);
			TidFont = rTb.CreateFont(0, fd.Face, fd.Size, fd.Flags);
		}
		if(TidFont) {
			int    tool_text_pen_id = rTb.CreateColor(0, SColor(SClrBlack));
			int    tool_text_brush_id = SPaintToolBox::rbr3DFace;
			int    tid_cs = rTb.CreateCStyle(0, TidFont, tool_text_pen_id, tool_text_brush_id);

			SParaDescr pd;
			if(adj == ADJ_CENTER)
				pd.Flags |= SParaDescr::fJustCenter;
			else if(adj == ADJ_RIGHT)
				pd.Flags |= SParaDescr::fJustRight;
			int    tid_para = rTb.CreateParagraph(0, &pd);
			rTlo.SetText((temp_buf = UiText).ToOem());
			rTlo.SetOptions(STextLayout::fWrap, tid_para, tid_cs);
			ok = 1;
		}
	}
	return ok;
}

int WhatmanObjectUiCtrl::Draw(TCanvas2 & rCanv)
{
	const float frame_text_delta = 6.0f;
	const  int _cxborder = GetSystemMetrics(SM_CXBORDER);
	const  int _cyborder = GetSystemMetrics(SM_CYBORDER);
	int    ok = 1;
	SPaintToolBox & r_tb = rCanv.GetToolBox();
	STextLayout tlo;
	TRect  b = GetBounds();
	r_tb.CreateReservedObjects();
	if(UiKind == UiItemKind::kDialog) {
		rCanv.DrawEdge(b, TCanvas2::edgeRaised, TCanvas2::borderRect | TCanvas2::borderAdjust);
		rCanv.Rect(b, 0, SPaintToolBox::rbr3DFace);
	}
	else if(UiKind == UiItemKind::kPushbutton) {
		rCanv.DrawEdge(b, TCanvas2::edgeRaised, TCanvas2::borderRect | TCanvas2::borderAdjust);
		rCanv.Rect(b, 0, SPaintToolBox::rbr3DFace);
		if(CreateTextLayout(r_tb, tlo, ADJ_CENTER) > 0) {
			FRect fb(b);
			fb.Grow((float)-_cxborder, (float)-_cyborder);
			tlo.SetBounds(fb);
			tlo.Arrange(rCanv.operator SDrawContext(), r_tb);
			rCanv.DrawTextLayout(&tlo);
		}
	}
	else if(UiKind == UiItemKind::kRadioCluster) {
		rCanv.DrawEdge(b, TCanvas2::edgeEtched, TCanvas2::borderRect);
		if(CreateTextLayout(r_tb, tlo) > 0) {
			FRect fb(b);
			fb.a.X += frame_text_delta;
			fb.b.X -= frame_text_delta;
			fb.b.Y = fb.a.Y + frame_text_delta;
			fb.a.Y -= frame_text_delta;
			tlo.SetOptions(tlo.fPrecBkg);
			tlo.SetBounds(fb);
			tlo.Arrange(rCanv.operator SDrawContext(), r_tb);
			rCanv.DrawTextLayout(&tlo);
		}
	}
	else if(UiKind == UiItemKind::kCheckCluster) {
		rCanv.DrawEdge(b, TCanvas2::edgeEtched, TCanvas2::borderRect);
		if(CreateTextLayout(r_tb, tlo) > 0) {
			FRect fb(b);
			fb.b.Y = fb.a.Y + frame_text_delta;
			fb.a.Y -= frame_text_delta;
			tlo.SetBounds(fb);
			tlo.Arrange(rCanv.operator SDrawContext(), r_tb);
			rCanv.DrawTextLayout(&tlo);
		}
	}
	else if(UiKind == UiItemKind::kRadiobutton) {
		SImageBuffer img_buf;
		HBITMAP h_bmp = (HBITMAP)LoadImage(0, MAKEINTRESOURCE(OBM_CHECKBOXES), IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
		if(img_buf.LoadBmp(0, h_bmp, 5, 13) > 0) {
			FPoint fp((float)(b.a.x + _cxborder), (float)(b.a.y + _cyborder));
			LMatrix2D mtx;
			rCanv.PushTransform();
			rCanv.AddTransform(mtx.InitTranslate(fp));
			rCanv.Draw(&img_buf);
			rCanv.PopTransform();
		}
		if(CreateTextLayout(r_tb, tlo) > 0) {
			FRect fb(b);
			if((fb.a.X + 20.0f) < fb.b.X)
				fb.a.X += 20.0f;
			fb.a.Y += 2;
			tlo.SetBounds(fb);
			tlo.Arrange(rCanv.operator SDrawContext(), r_tb);
			rCanv.DrawTextLayout(&tlo);
		}
	}
	else if(UiKind == UiItemKind::kCheckbox) {
		SImageBuffer img_buf;
		HBITMAP h_bmp = (HBITMAP)LoadImage(0, MAKEINTRESOURCE(OBM_CHECKBOXES), IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
		if(img_buf.LoadBmp(0, h_bmp, 1, 13) > 0) {
			FPoint fp((float)(b.a.x + _cxborder), (float)(b.a.y + _cyborder));
			LMatrix2D mtx;
			rCanv.PushTransform();
			rCanv.AddTransform(mtx.InitTranslate(fp));
			rCanv.Draw(&img_buf);
			rCanv.PopTransform();
		}
		if(CreateTextLayout(r_tb, tlo) > 0) {
			FRect fb(b);
			if((fb.a.X + 20.0f) < fb.b.X)
				fb.a.X += 20.0f;
			fb.a.Y += 2;
			tlo.SetBounds(fb);
			tlo.Arrange(rCanv.operator SDrawContext(), r_tb);
			rCanv.DrawTextLayout(&tlo);
		}
	}
	else if(UiKind == UiItemKind::kFrame) {
		rCanv.DrawEdge(b, TCanvas2::edgeEtched, TCanvas2::borderRect);
		if(CreateTextLayout(r_tb, tlo) > 0) {
			FRect fb(b);
			fb.b.Y = fb.a.Y + frame_text_delta;
			fb.a.Y -= frame_text_delta;
			tlo.SetBounds(fb);
			tlo.Arrange(rCanv.operator SDrawContext(), r_tb);
			rCanv.DrawTextLayout(&tlo);
		}
	}
	else if(UiKind == UiItemKind::kInput) {
		rCanv.DrawEdge(b, TCanvas2::edgeSunken, TCanvas2::borderRect | TCanvas2::borderAdjust);
	}
	else if(UiKind == UiItemKind::kStatic) {
		if(CreateTextLayout(r_tb, tlo) > 0) {
			FRect fb(b);
			fb.Grow((float)-_cxborder, (float)-_cyborder);
			tlo.SetBounds(fb);
			tlo.Arrange(rCanv.operator SDrawContext(), r_tb);
			rCanv.DrawTextLayout(&tlo);
		}
	}
	else if(UiKind == UiItemKind::kLabel) {
		if(CreateTextLayout(r_tb, tlo) > 0) {
			FRect fb(b);
			fb.Grow((float)-_cxborder, (float)-_cyborder);
			tlo.SetBounds(fb);
			tlo.Arrange(rCanv.operator SDrawContext(), r_tb);
			rCanv.DrawTextLayout(&tlo);
		}
	}
	else if(UiKind == UiItemKind::kCombobox) {
		rCanv.DrawEdge(b, TCanvas2::edgeSunken, TCanvas2::borderRect | TCanvas2::borderAdjust);
	}
	else if(UiKind == UiItemKind::kListbox) {
		rCanv.DrawEdge(b, TCanvas2::edgeSunken, TCanvas2::borderRect | TCanvas2::borderAdjust);
	}
	else if(UiKind == UiItemKind::kTreeListbox) {
		rCanv.DrawEdge(b, TCanvas2::edgeSunken, TCanvas2::borderRect | TCanvas2::borderAdjust);
	}
	else {
		rCanv.DrawFrame(b, 1, SPaintToolBox::rbrWindowFrame);
	}
	return ok;
}

class UiWtmToolDialog : public TDialog {
public:
	UiWtmToolDialog();
	int    setDTS(const TWhatmanToolArray::Item * pData);
	int    getDTS(TWhatmanToolArray::Item * pData);
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCbSelected(CTLSEL_WTMTOOL_KIND)) {
			SString temp_buf;
			long   ui_kind_id = getCtrlLong(CTLSEL_WTMTOOL_KIND);
			getCtrlString(CTL_WTMTOOL_NAME, temp_buf);
			if(!temp_buf.NotEmptyS()) {
				UiItemKind uik(ui_kind_id);
				if(uik.Text.NotEmptyS())
					setCtrlString(CTL_WTMTOOL_NAME, uik.Text);
			}
			clearEvent(event);
		}
	}

	TWhatmanToolArray::Item Data;
};

UiWtmToolDialog::UiWtmToolDialog() : TDialog(DLG_WTMTOOLUI)
{
	FileBrowseCtrlGroup::Setup(this, CTLBRW_WTMTOOL_PICPATH, CTL_WTMTOOL_PICPATH, 2, 0, PPTXT_FILPAT_SPIC,
		FileBrowseCtrlGroup::fbcgfFile|FileBrowseCtrlGroup::fbcgfSaveLastPath);
}

int UiWtmToolDialog::setDTS(const TWhatmanToolArray::Item * pData)
{
	int    ok = 1;
	StrAssocArray * p_wtmo_list = 0;
	Data = *pData;
	WhatmanObjectUiCtrl::ToolItemExtData tied;
	if(Data.ExtSize >= sizeof(WhatmanObjectUiCtrl::ToolItemExtData))
		tied = *(WhatmanObjectUiCtrl::ToolItemExtData *)Data.ExtData;
	else
		MEMSZERO(tied);
	setCtrlString(CTL_WTMTOOL_NAME, Data.Text);
	setCtrlString(CTL_WTMTOOL_SYMB, Data.Symb);
	disableCtrl(CTL_WTMTOOL_SYMB, Data.Symb.NotEmpty());
	setCtrlString(CTL_WTMTOOL_PICPATH, Data.PicPath);
	setCtrlData(CTL_WTMTOOL_FIGSIZE, &Data.FigSize);
	setCtrlData(CTL_WTMTOOL_PICSIZE, &Data.PicSize);
	AddClusterAssoc(CTL_WTMTOOL_FLAGS, 0, TWhatmanToolArray::Item::fDontEnlarge);
	AddClusterAssoc(CTL_WTMTOOL_FLAGS, 1, TWhatmanToolArray::Item::fDontKeepRatio);
	SetClusterData(CTL_WTMTOOL_FLAGS, Data.Flags);
	{
		ComboBox * p_combo = (ComboBox *)getCtrlView(CTLSEL_WTMTOOL_WTMOBJ);
		if(p_combo) {
			p_combo->setListWindow(CreateListWindow(TWhatmanObject::MakeStrAssocList(), lbtDisposeData|lbtDblClkNotify),
				TWhatmanObject::GetRegIdBySymb(Data.WtmObjSymb));
			disableCtrl(CTLSEL_WTMTOOL_WTMOBJ, 1);
		}
	}
	{
		ComboBox * p_combo = (ComboBox *)getCtrlView(CTLSEL_WTMTOOL_KIND);
		if(p_combo) {
			StrAssocArray * p_ui_kind_list = new StrAssocArray;
			UiItemKind::GetTextList(*p_ui_kind_list);
			p_combo->setListWindow(CreateListWindow(p_ui_kind_list, lbtDisposeData|lbtDblClkNotify), tied.UiKindID);
		}
	}
	return ok;
}

int UiWtmToolDialog::getDTS(TWhatmanToolArray::Item * pData)
{
	int    ok = 1;
	long   wtmo_id = getCtrlLong(CTLSEL_WTMTOOL_WTMOBJ);
	if(wtmo_id)
		TWhatmanObject::GetRegSymbById(wtmo_id, Data.WtmObjSymb);
	long   ui_kind_id = getCtrlLong(CTLSEL_WTMTOOL_KIND);
	getCtrlString(CTL_WTMTOOL_NAME, Data.Text);
	getCtrlString(CTL_WTMTOOL_SYMB, Data.Symb);
	getCtrlString(CTL_WTMTOOL_PICPATH, Data.PicPath);
	getCtrlData(CTL_WTMTOOL_FIGSIZE, &Data.FigSize);
	getCtrlData(CTL_WTMTOOL_PICSIZE, &Data.PicSize);
	GetClusterData(CTL_WTMTOOL_FLAGS, &Data.Flags);

	WhatmanObjectUiCtrl::ToolItemExtData * p_tied = (WhatmanObjectUiCtrl::ToolItemExtData *)Data.ExtData;
	Data.ExtSize = sizeof(WhatmanObjectUiCtrl::ToolItemExtData);
	p_tied->UiKindID = ui_kind_id;
	ASSIGN_PTR(pData, Data);
	return ok;
}

int WhatmanObjectUiCtrl::HandleCommand(int cmd, void * pExt)
{
	int    ok = -1;
	switch(cmd) {
		case cmdSetupByTool:
			{
				const TWhatmanToolArray::Item * p_item = (const TWhatmanToolArray::Item *)pExt;
				if(p_item) {
					const ToolItemExtData * p_ext = (p_item->ExtSize >= sizeof(ToolItemExtData)) ? (const ToolItemExtData *)p_item->ExtData : 0;
					if(p_ext && p_ext->UiKindID) {
						UiKind = p_ext->UiKindID;
						TPoint p;
						Rect.L.Set(p = 0);
						p = p_item->FigSize;
						Rect.R.Set(p);
						ok = 1;
					}
				}
			}
			break;
		case cmdSetBounds:
			{
				TRect * p_rect = (TRect *)pExt;
				Rect.Set(*p_rect);
			}
			ok = 1;
			break;
		case cmdEditTool:
			ok = PPDialogProcBody <UiWtmToolDialog, TWhatmanToolArray::Item> ((TWhatmanToolArray::Item *)pExt);
			break;
		case cmdObjInserted:
			if(P_DlCtx && P_Scope) {
				PPWhatmanWindow * p_win = (PPWhatmanWindow *)GetOwnerWindow();
				if(p_win) {
					char   c_buf[1024]; // Буфер для извлечения констант
					if(oneof2(UiKind, DlScope::ckCheckCluster, DlScope::ckRadioCluster)) {
						if(P_DlCtx->GetConstData(P_Scope->GetFldConst(F.ID, DlScope::cuifCtrlScope), c_buf, sizeof(c_buf))) {
							DLSYMBID inner_scope_id = *(uint32 *)c_buf;
							DlScope * p_inner_scope = P_DlCtx->GetScope(inner_scope_id);
							if(p_inner_scope && p_inner_scope->GetCount()) {
								SdbField ctrl_item;
								for(uint k = 0; p_inner_scope->EnumFields(&k, &ctrl_item);) {
									WhatmanObjectUiCtrl * p_item_obj = new WhatmanObjectUiCtrl;
									int    r = 0;
									if(p_item_obj->Set(P_DlCtx, P_Scope, p_inner_scope, ctrl_item.ID)) {
										if(UiKind == DlScope::ckCheckCluster)
											p_item_obj->SetKind(DlScope::ckCheckbox);
										else if(UiKind == DlScope::ckRadioCluster)
											p_item_obj->SetKind(DlScope::ckRadiobutton);
										p_item_obj->P_LinkObj = this;
										p_win->AddObject(p_item_obj, 0 /*bounds*/);
										r = 1;
									}
									if(!r)
										delete p_item_obj;
								}
							}
						}
					}
					else if(UiKind == DlScope::ckInput) {
						if(UiText.NotEmpty()) {
							UiRelRect urr;
							urr.Reset();
							if(P_DlCtx->GetConstData(P_Scope->GetFldConst(F.ID, DlScope::cuifLabelRect), c_buf, sizeof(c_buf)))
								urr = *(UiRelRect *)c_buf;
							if(urr.IsEmpty()) {
								TRect b = GetBounds();
								int16  top = b.a.y;
								b.a.y = top-16;
								b.b.y = top-3;
								urr.Set(b);
							}
							WhatmanObjectUiCtrl * p_label_obj = new WhatmanObjectUiCtrl;
							int    r = 0;
							if(p_label_obj->Set(P_DlCtx, P_Scope, 0, -1)) {
								p_label_obj->SetKind(DlScope::ckLabel);
								p_label_obj->Rect = urr;
								{
									TRect bounds;
									bounds.a.x = urr.L.X.Val;
									bounds.a.y = urr.L.Y.Val;
									bounds.b.x = urr.R.X.Val;
									bounds.b.y = urr.R.Y.Val;
									p_label_obj->SetBounds(bounds);
								}
								p_label_obj->UiText = UiText;
								p_label_obj->P_LinkObj = this;
								p_win->AddObject(p_label_obj, 0 /*bounds*/);
								r = 1;
							}
							if(!r)
								delete p_label_obj;
						}
					}
				}
			}
			break;
	}
	return ok;
}
//
//
//
class PPDialogEditWindow : public PPWhatmanWindow {
public:
	friend int EditDialogSpec(DlContext * pCtx, uint dlgId);

	PPDialogEditWindow(DlContext * pCtx) : PPWhatmanWindow(modeEdit)
	{
		P_DCtx = pCtx;
		P_Scope = 0;
	}
	int    Load(uint dlgId);
private:
	DlContext * P_DCtx; // @notowned
	DlScope * P_Scope;  // @notowned
};

int PPDialogEditWindow::Load(uint dlgId)
{
	int    ok = 1;
	THROW(P_DCtx);
	THROW(P_Scope = P_DCtx->GetScope(dlgId, DlScope::kUiDialog));
	{
		SdbField ctrl;
		WhatmanObjectUiCtrl * p_obj = new WhatmanObjectUiCtrl;
		THROW(p_obj->Set(P_DCtx, P_Scope));
		AddObject(p_obj, 0 /*bounds*/);
		WhatmanObjectUiCtrl * p_ctrl_obj = 0;
		for(uint j = 0; P_Scope->EnumFields(&j, &ctrl);) {
			p_ctrl_obj = new WhatmanObjectUiCtrl;
			THROW(p_ctrl_obj->Set(P_DCtx, P_Scope, 0, ctrl.ID));
			AddObject(p_ctrl_obj, 0 /*bounds*/);
		}
	}
	CATCHZOK
	return ok;
}

int EditDialogSpec(DlContext * pCtx, uint dlgId)
{
	class FrameWindow : public TWindowBase {
	public:
		enum {
			zoneLeft = 1,
			zoneCenter
		};
		FrameWindow() : TWindowBase()
		{
			SRectLayout::Item li;
			Layout.Add(zoneLeft, li.SetLeft(20, 1));
			Layout.Add(zoneCenter, li.SetCenter());
		}
	};
	int    ok = -1;
	PPWhatmanWindow * p_tool_win = 0;
	PPDialogEditWindow * p_edit_win = 0;
	FrameWindow * p_frame_win = new FrameWindow;
	p_frame_win->changeBounds(TRect(10, 10, 900, 900));
	p_frame_win->Create((long)APPL->H_MainWnd, TWindowBase::coPopup);
	{
		THROW_MEM(p_tool_win = new PPWhatmanWindow(PPWhatmanWindow::modeToolbox));
		p_frame_win->AddChild(p_tool_win, TWindowBase::coChild, FrameWindow::zoneLeft);
	}
	{
		THROW_MEM(p_edit_win = new PPDialogEditWindow(pCtx));
		p_frame_win->AddChild(p_edit_win, TWindowBase::coChild, FrameWindow::zoneCenter);
		{
			TWhatman::Param p = p_edit_win->W.GetParam();
			p.Unit = UNIT_GR_PIXEL;
			p.UnitFactor = 10;
			p.Flags |= (TWhatman::Param::fRule | TWhatman::Param::fGrid);
			p.Flags &= ~(TWhatman::Param::fDisableReszObj | TWhatman::Param::fDisableMoveObj | TWhatman::Param::fSnapToGrid);
			p_edit_win->W.SetParam(p);
		}
		THROW(p_edit_win->Load(dlgId));
	}
	{
		SString file_name;
		PPGetFilePath(PPPATH_WTM, "dialog.wta", file_name);
		THROW(p_tool_win->LoadTools(file_name));
	}
	ShowWindow(p_frame_win->H(), SW_SHOWNORMAL);
	p_frame_win->invalidateAll(1);
	UpdateWindow(p_frame_win->H());
	CATCH
		ZDELETE(p_frame_win);
		ok = PPErrorZ();
	ENDCATCH
	return ok;
}

