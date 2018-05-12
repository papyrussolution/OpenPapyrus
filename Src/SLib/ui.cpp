// UI.CPP
// Copyright (c) A.Sobolev 2011, 2016, 2018
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
//
//
// static
int UiItemKind::GetTextList(StrAssocArray & rList)
{
	int    ok = 1;
	rList.Z();
	UiItemKind item;
	for(int i = kDialog; i < kCount; i++) {
		if(item.Init(i)) {
			rList.Add(i, item.Text);
		}
	}
	return ok;
}

//static
int UiItemKind::GetIdBySymb(const char * pSymb)
{
	int    id = 0;
	UiItemKind item;
	for(int i = kDialog; !id && i < kCount; i++) {
		if(item.Init(i) && item.Symb.Cmp(pSymb, 0) == 0)
			id = i;
	}
	return id;
}

UiItemKind::UiItemKind(int kind)
{
	Init(kind);
}

int UiItemKind::Init(int kind)
{
	int    ok = 1;
	Id = 0;
	P_Cls = 0;
	Symb.Z();
	Text.Z();
	const char * p_text_sign = 0;
	switch(kind) {
		case kUnkn:
			break;
		case kDialog:
			p_text_sign = "ui_dialog";
			Symb = "dialog";
			break;
		case kInput:
			p_text_sign = "ui_input";
			Symb = "input";
			break;
		case kStatic:
			p_text_sign = "ui_static";
			Symb = "statictext";
			break;
		case kPushbutton:
			p_text_sign = "ui_pushbutton";
			Symb = "button";
			break;
		case kCheckbox:
			p_text_sign = "ui_checkbox";
			Symb = "checkbox";
			break;
		case kRadioCluster:
			p_text_sign = "ui_radiocluster";
			Symb = "radiocluster";
			break;
		case kCheckCluster:
			p_text_sign = "ui_checkcluster";
			Symb = "checkcluster";
			break;
		case kCombobox:
			p_text_sign = "ui_combobox";
			Symb = "combobox";
			break;
		case kListbox:
			p_text_sign = "ui_listbox";
			Symb = "listbox";
			break;
		case kTreeListbox:
			p_text_sign = "ui_treelistbox";
			Symb = "treelistbox";
			break;
		case kFrame:
			p_text_sign = "ui_frame";
			Symb = "framebox";
			break;
		default:
			ok = 0;
			break;
	}
	if(p_text_sign) {
		Id = kind;
		SLS.LoadString(p_text_sign, Text);
	}
	return ok;
}
