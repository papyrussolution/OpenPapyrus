// RcToUI.CPP
// Copyright (c) Starodub A. 2007, 2010, 2015, 2016, 2017
//
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <slib.h>

struct CtrlInfo;

void error(char * s)
{
	printf("Error: %s\n", s);
	exit(-1);
}

struct SpcSymbEntry {
	char   chr;
	int8   Amp;
	const  char * str;
};

SpcSymbEntry SpcSymbTab[] = {
	{ '\x26', 1, "amp"    },
	{ '\x2f', 0, "fwsl"   },
	{ '\x3c', 1, "lt"     },
	{ '\x3d', 0, "eq"     },
	{ '\x3e', 0, "gt"     },
	{ '\x5b', 0, "lsq"    },
	{ '\x5d', 0, "rsq"    }
};

int SLAPI ReplaceSpecSymb(SString & rBuf)
{
	SString subst;
	for(size_t i = 0; i < (sizeof(SpcSymbTab)/sizeof(SpcSymbEntry)); i++) {
		(subst = 0).CatChar('&').Cat(SpcSymbTab[i].str).Semicol();
		char   pattern[8];
		pattern[0] = SpcSymbTab[i].chr;
		pattern[1] = 0;
		rBuf.ReplaceStr(pattern, subst, 0);
	}
	return 1;
}

struct CtrlType {
	enum {
		ctrlDialogEx = 1,
		ctrlPushButton,
		ctrlDefPushButton,
		ctrlControl,
		ctrlListBox,
		ctrlTreeListBox,
		ctrlRadioButton,
		ctrlCheckBox,
		ctrlEditText,
		ctrlCText,
		ctrlLText,
		ctrlGroupBox,
		ctrlComboBox,
		ctrlCaption,
		ctrlFont,
		ctrlStyle,
		ctrlExStyle,
		ctrlEnd,
		ctrlBegin,
		ctrlDosEditText,
		ctrlDosLabel,
		ctrlDosRadioButtons,
		ctrlDosButton,
		ctrlDosStatic,
		ctrlDosCheckBoxes,
		ctrlDosCombo,
		ctrlDosListBox,
		ctrlDosEnd,
		ctrlDosDialog
	};

	char TypeName[64];
	char QtTypeName[64];
	long Type;
};

const CtrlType ControlsType[] = {
	{"DIALOGEX",      "QDialog",     CtrlType::ctrlDialogEx},
	{"PUSHBUTTON",    "QToolButton", CtrlType::ctrlPushButton},
	{"DEFPUSHBUTTON", "QToolButton", CtrlType::ctrlDefPushButton},
	{"CONTROL",       "",            CtrlType::ctrlControl},
	{"LISTBOX",       "QListView",   CtrlType::ctrlListBox},
	{"EDITTEXT",      "QLineEdit",   CtrlType::ctrlEditText},
	{"CTEXT",         "QLabel",      CtrlType::ctrlCText},
	{"LTEXT",         "QLabel",      CtrlType::ctrlLText},
	{"GROUPBOX",      "QGroupBox",   CtrlType::ctrlGroupBox},
	{"COMBOBOX",      "QComboBox",   CtrlType::ctrlComboBox},
	{"CAPTION",       "",            CtrlType::ctrlCaption},
	{"FONT",          "",            CtrlType::ctrlFont},
	{"EXSTYLE",       "",            CtrlType::ctrlExStyle},
	{"STYLE",         "",            CtrlType::ctrlStyle},
	{"END",           "",            CtrlType::ctrlEnd},
	{"BEGIN",         "",            CtrlType::ctrlBegin}
};

const CtrlType DosControlsType[] = {
	{"TV_INPUTLINE",    "", CtrlType::ctrlDosEditText},
	{"TV_LABEL",        "", CtrlType::ctrlDosLabel},
	{"TV_RADIOBUTTONS", "", CtrlType::ctrlDosRadioButtons},
	{"TV_BUTTON",       "", CtrlType::ctrlDosButton},
	{"TV_STATIC",       "", CtrlType::ctrlDosStatic},
	{"TV_CHECKBOXES",   "", CtrlType::ctrlDosCheckBoxes},
	{"TV_COMBO",        "", CtrlType::ctrlDosCombo},
	{"TV_LISTBOX",      "", CtrlType::ctrlDosListBox},
	{"TV_END",          "", CtrlType::ctrlDosEnd},
	{"TV_DIALOG",       "", CtrlType::ctrlDosDialog},
	{"BEGIN",           "", CtrlType::ctrlDosDialog},
	{"EMD",             "", CtrlType::ctrlDosDialog}

};

class CtrlInfoArray : public TSCollection <CtrlInfo> {
public:
	CtrlInfoArray() : TSCollection <CtrlInfo> () {}
};

struct CtrlInfo {
	CtrlInfo() {P_Controls = 0; Init();}
	int Init();
	int IsDialog() const {return (ID.Len() && CtrlT.Type == CtrlType::ctrlDialogEx) ? 1 : 0;}
	int SupportExtraData() const;
	CtrlInfo & operator=(const CtrlInfo &);
	int AddControl(const CtrlInfo * pCtrlI);
	int ProcessControls();

	CtrlType CtrlT;
	SString ID;
	SString LinkedCtrlID;
	SString Cmd;
	SString Text;
	SString Font;
	SString Style;
	SString ExStyle;
	RECT    Coord;
	SString Extras;
	long    MaxSize;

	CtrlInfoArray * P_Controls;
};

int CtrlInfo::Init()
{
	memset(&CtrlT, 0, sizeof(CtrlT));
	ID           = 0;
	Text         = 0;
	Font         = 0;
	Style        = 0;
	ExStyle      = 0;
	LinkedCtrlID = 0;
	Cmd          = 0;
	Extras       = 0;
	MaxSize      = 0;
	memset(&Coord, 0, sizeof(Coord));
	Extras = 0;
	if(P_Controls) {
		P_Controls->freeAll();
		ZDELETE(P_Controls);
	}
	return 1;
}

CtrlInfo & CtrlInfo::operator=(const CtrlInfo & rCtrlI)
{
	Init();
	CtrlT        = rCtrlI.CtrlT;
	ID           = rCtrlI.ID;
	Text         = rCtrlI.Text;
	Font         = rCtrlI.Font;
	Style        = rCtrlI.Style;
	Coord        = rCtrlI.Coord;
	Extras       = rCtrlI.Extras;
	LinkedCtrlID = rCtrlI.LinkedCtrlID;
	Cmd          = rCtrlI.Cmd;
	ExStyle      = rCtrlI.ExStyle;
	MaxSize      = rCtrlI.MaxSize;

	if(rCtrlI.P_Controls) {
		P_Controls = new CtrlInfoArray;
		for(uint i = 0; i < rCtrlI.P_Controls->getCount(); i++) {
			CtrlInfo * p_ctrl_i = 0;
			p_ctrl_i = new CtrlInfo;
			ASSIGN_PTR(p_ctrl_i, *rCtrlI.P_Controls->at(i));
            P_Controls->insert(p_ctrl_i);
		}
	}
	return *this;
}

int CtrlInfo::SupportExtraData() const
{
	return (ID.Len() && (CtrlT.Type == CtrlType::ctrlListBox || CtrlT.Type == CtrlType::ctrlEditText ||
			CtrlT.Type == CtrlType::ctrlPushButton || CtrlT.Type == CtrlType::ctrlDefPushButton ||
			CtrlT.Type == CtrlType::ctrlTreeListBox || CtrlT.Type == CtrlType::ctrlCText ||
			CtrlT.Type == CtrlType::ctrlLText)) ? 1 : 0;
}

int CtrlInfo::AddControl(const CtrlInfo * pCtrlI)
{
	int    ok = 1;
	CtrlInfo * p_ctrl_i = 0;
	THROW(pCtrlI);
	THROW(p_ctrl_i = new CtrlInfo);
	ASSIGN_PTR(p_ctrl_i, *pCtrlI);
	p_ctrl_i->Font.Strip();
	p_ctrl_i->Style.Strip();
	p_ctrl_i->ExStyle.Strip();
	p_ctrl_i->ID.Strip();
	p_ctrl_i->Text.Strip();
	p_ctrl_i->Extras.Strip();
	if(!P_Controls)
		THROW(P_Controls = new CtrlInfoArray);
	P_Controls->insert(p_ctrl_i);
	CATCHZOK
	return ok;
}

IMPL_CMPFUNC(CtrlInfo, i1, i2)
{
	CtrlInfo * p_i1 = (CtrlInfo*)i1;
	CtrlInfo * p_i2 = (CtrlInfo*)i2;
	if(p_i1->CtrlT.Type < p_i2->CtrlT.Type)
		return -1;
	else if(p_i1->CtrlT.Type > p_i2->CtrlT.Type)
		return 1;
	return 0;
}

int CtrlInfo::ProcessControls()
{
	if(P_Controls) {
		long   i;
		SArray del_ctrls(sizeof(long));
		SArray grpbxs(sizeof(long));
		CtrlInfo ctrl_i;
		ctrl_i.CtrlT.Type = CtrlType::ctrlGroupBox;
		for(uint p = 0; P_Controls->lsearch(&ctrl_i, &p, PTR_CMPFUNC(CtrlInfo)) > 0; p++) {
			RECT grpb_r = P_Controls->at(p)->Coord;
			if(grpbxs.lsearch(&p, 0, PTR_CMPFUNC(long)) <= 0)
				grpbxs.insert(&p);
			for(i = 0; i < (long)P_Controls->getCount(); i++) {
				RECT ctrl_r = P_Controls->at(i)->Coord;
				if(i != p && ctrl_r.left >= grpb_r.left && ctrl_r.top >= grpb_r.top &&
					(ctrl_r.left + ctrl_r.right) <= (grpb_r.left + grpb_r.right) && (ctrl_r.top + ctrl_r.bottom) <= (grpb_r.top + grpb_r.bottom) &&
					del_ctrls.lsearch(&i, 0, PTR_CMPFUNC(long)) <= 0) {
					ctrl_r.top    -= grpb_r.top;
					ctrl_r.left   -= grpb_r.left;
					P_Controls->at(i)->Coord = ctrl_r;
					P_Controls->at(p)->AddControl(P_Controls->at(i));
					del_ctrls.insert(&i);
				}
			}
		}
		del_ctrls.sort(PTR_CMPFUNC(long));
		for(i = 0; i < (long)grpbxs.getCount(); i++)
			P_Controls->at(*(long*)grpbxs.at(i))->ProcessControls(); // @recursion
		for(i = (long)del_ctrls.getCount() - 1; i >= 0; i--)
			P_Controls->atFree(*(long*)del_ctrls.at(i));
	}
	return 1;
}

const double CoordModifier = 1.5;
const long   GridSize      = 5;

class RcProcessor {
public:
	RcProcessor() {InRcPos = -1;}

	int Init(const char * pRcwPath, const char * pRcPath);
	int Run();
private:
	int NextControl(CtrlInfo * pCtrlInfo);
	int BeginDialog(CtrlInfo * pDlgInfo);
	int EndDialog();
	const CtrlType * GetCtrlType(SString & rBuf, int dos = 0, int getCtrlTail = 0);
	int   LoadExtraData(CtrlInfo * pCtrlInfo);
	int   PutDialog(CtrlInfo * pCtrlInfo);
	int   PutControl(CtrlInfo * pCtrlInfo, int level = 0);

	SString Path;
	SString OutBuf;
	CtrlInfo DialogInfo;
	SFile InRcw;
	SFile InRc;
	long InRcPos;
};

int RcProcessor::Init(const char * pRcwPath, const char * pRcPath)
{
	SPathStruc sp;
	Path.CopyFrom(pRcwPath);
	sp.Split(Path);
	InRcw.Close();
	InRcw.Open(pRcwPath, SFile::mRead);
	InRc.Close();
	InRc.Open(pRcPath, SFile::mRead);
	sp.Merge(0, SPathStruc::fNam|SPathStruc::fExt, Path);
	return InRcw.IsValid() && InRc.IsValid();
}

int RcProcessor::Run()
{
	int ok = 1;
	CtrlInfo ctrl_i;
	while(NextControl(&ctrl_i) > 0) {
		if(ctrl_i.CtrlT.Type == CtrlType::ctrlDialogEx) {
			DialogInfo = ctrl_i;
			InRcPos = -1;
		}
		else if(DialogInfo.IsDialog()) {
			if(ctrl_i.CtrlT.Type == CtrlType::ctrlBegin || ctrl_i.CtrlT.Type == CtrlType::ctrlFont ||
				ctrl_i.CtrlT.Type == CtrlType::ctrlStyle || ctrl_i.CtrlT.Type == CtrlType::ctrlExStyle
			)
				;
			else if(ctrl_i.CtrlT.Type == CtrlType::ctrlEnd) {
				THROW(DialogInfo.ProcessControls());
				THROW(PutDialog(&DialogInfo));
				DialogInfo.Init();
				OutBuf = 0;
			}
			else if(ctrl_i.CtrlT.Type == CtrlType::ctrlCaption)
				DialogInfo.Text = ctrl_i.Text;
			else
				THROW(DialogInfo.AddControl(&ctrl_i));
		}
		ctrl_i.Init();
	}
	CATCHZOK
	return ok;
}

const CtrlType * RcProcessor::GetCtrlType(SString & rBuf, int dos /*=0*/, int getCtrlTail /*=0*/)
{
	int stop = 0;
	SString buf;
	const CtrlType * p_ctrl_t = 0;
	SFile * p_in = (dos) ? &InRc : &InRcw;
	const CtrlType * p_ctrls = (dos) ? DosControlsType : ControlsType;
	uint ctrls_count = (dos) ? sizeof(DosControlsType)/sizeof(CtrlType) : sizeof(ControlsType)/sizeof(CtrlType);
	while(!stop && p_in && p_in->ReadLine(buf)) {
		int found = 0;
		size_t org_buf_len = buf.Len();
		buf.ReplaceChar('\t', ' ').Strip();
		for(uint i = 0; !stop && i < ctrls_count; i++) {
			uint p = 0;
			if((p_ctrls[i].Type == CtrlType::ctrlDialogEx || p_ctrls[i].Type == CtrlType::ctrlDosDialog) && buf.Search(p_ctrls[i].TypeName, 0, 0, &p))
				found = 1;
			else if(p_ctrls[i].Type != CtrlType::ctrlDialogEx && p_ctrls[i].Type != CtrlType::ctrlDosDialog && buf.CmpPrefix(p_ctrls[i].TypeName, 0) == 0)
				found = 1;
			if(found) {
				stop = 1;
				if(!getCtrlTail) {
					SString buf2;
					size_t tail_len = p + strlen(p_ctrls[i].TypeName) + 1;
					p_ctrl_t = &p_ctrls[i];
					if(p_ctrl_t->Type != CtrlType::ctrlEnd && p_ctrl_t->Type != CtrlType::ctrlBegin && p_ctrl_t->Type != CtrlType::ctrlDosEnd) {
						GetCtrlType(buf2, dos, 1);
						buf.Cat(buf2).Strip();
					}
					rBuf = 0;
					if(p)
						rBuf = buf.Sub(0, p - 1, rBuf);
					buf2 = buf.Sub(tail_len, buf.Len() - tail_len, buf2);
					rBuf.CatChar(' ').Cat(buf2);
					rBuf.Strip();
				}
			}
		}
		if(getCtrlTail) {
			if(found)
				p_in->Seek(-(long)(org_buf_len + 1), SEEK_CUR);
			else
				rBuf.Cat(buf);
		}
	}
	return p_ctrl_t;
}

int RcProcessor::LoadExtraData(CtrlInfo * pCtrlInfo)
{
	int    ok = -1;
	if(pCtrlInfo && pCtrlInfo->SupportExtraData() && InRc.IsValid()) {
		uint   p = 0;
		int    found = 0;
		long   fpos = 0;
		const  uint num_extras = 15;
		SString extras[num_extras], out_extras[7];
		SString buf;
		const CtrlType * p_ctrlt = 0;
		StringSet ss(",");
		if(InRcPos < 0) {
			InRc.Seek(0, SEEK_SET);
			while(!found && (p_ctrlt = GetCtrlType(buf, 1, 0))) {
				if(p_ctrlt->Type == CtrlType::ctrlDosDialog && buf.Cmp(DialogInfo.ID, 0) == 0) {
					found = 1;
					InRcPos = InRc.Tell();
				}
			}
		}
		else {
			InRc.Seek(InRcPos, SEEK_SET);
			found = 1;
		}
		if(found) {
			found = 0;
			SString id;
			while(!found && (p_ctrlt = GetCtrlType(buf, 1, 0)) && p_ctrlt->Type != CtrlType::ctrlEnd) {
				ss.setBuf(buf, buf.Len() + 1);
				ss.get(&(p = 0), (buf = 0)); // x
				ss.get(&p, (buf = 0)); // y
				ss.get(&p, (buf = 0)); // w
				ss.get(&p, (buf = 0)); // h
				ss.get(&p, (id = 0));  // id
				id.Strip();
				found = (pCtrlInfo->ID.Cmp(id, 0) == 0) ? 1 : 0;
			}
		}
		if(found) {
			uint   i;
			for(i = 0; i < 15 && ss.get(&p, extras[i]) > 0; i++) 
				extras[i].Strip();
			if(p_ctrlt->Type == CtrlType::ctrlDosCombo) {
				out_extras[2] = extras[0];
				pCtrlInfo->LinkedCtrlID = extras[1];
			}
			else if(p_ctrlt->Type == CtrlType::ctrlDosLabel)
				(pCtrlInfo->LinkedCtrlID = extras[1]).TrimRightChr('\r').TrimRightChr('\n');
			else if(p_ctrlt->Type == CtrlType::ctrlDosButton)
				(pCtrlInfo->Cmd = extras[1]).TrimRightChr('\r').TrimRightChr('\n');
			else if(p_ctrlt->Type == CtrlType::ctrlDosEditText) {
				(out_extras[0] = extras[0]).Comma().Cat(extras[1]);
				pCtrlInfo->MaxSize = extras[1].ToLong();
				(out_extras[1]  = extras[3]).Comma().Cat(extras[4]).Comma().Cat(extras[5]);
			}
			else if(p_ctrlt->Type == CtrlType::ctrlDosListBox) {
				int    need_comma = 0;
				out_extras[0] = extras[1];
				for(i = 8; i < num_extras; i++) {
					if(extras[i].Len()) {
						if(need_comma)
							out_extras[3].Comma();
						out_extras[3].Cat(extras[i]);
						need_comma = 1;
					}
				}
				out_extras[3].ReplaceChar('\"', ' ');
				out_extras[3].TrimRightChr('\n').TrimRightChr('\r').ReplaceStr("\\0", "", 0).Strip().ToChar();
				ReplaceSpecSymb(out_extras[3]);
				out_extras[3].ToUtf8();
			}
			pCtrlInfo->Extras = 0;
			for(i = 0; i < 6; i++) {
				if(out_extras[i].Len())
					pCtrlInfo->Extras.Cat(out_extras[i]);
				if(i != 5)
					pCtrlInfo->Extras.CatChar('`');
			}
			ok = 1;
		}
	}
	return ok;
}

int RcProcessor::NextControl(CtrlInfo * pCtrlInfo)
{
	int ok = -1;
	SString buf;
	const CtrlType * p_ctrl_t = 0;
	CtrlInfo ctrl_i;
	ctrl_i.Init();
	if(p_ctrl_t = GetCtrlType(buf)) {
		uint p = 0;
		StringSet ss(" ");
		ctrl_i.CtrlT = *p_ctrl_t;
		ok = 1;
		switch(ctrl_i.CtrlT.Type) {
			case CtrlType::ctrlPushButton:
			case CtrlType::ctrlDefPushButton:
			case CtrlType::ctrlCText:
			case CtrlType::ctrlLText:
			case CtrlType::ctrlGroupBox:
			case CtrlType::ctrlListBox:
			case CtrlType::ctrlEditText:
			case CtrlType::ctrlComboBox:
				{
					if(DialogInfo.IsDialog()) {
						if(ctrl_i.CtrlT.Type != CtrlType::ctrlListBox && ctrl_i.CtrlT.Type != CtrlType::ctrlEditText && ctrl_i.CtrlT.Type != CtrlType::ctrlComboBox) {
							ss.setDelim("\"");
							ss.setBuf(buf, buf.Len() + 1);
							ss.get(&(p = 0), buf);
							ss.get(&p, ctrl_i.Text);
							ss.get(&p, (buf = 0));
							buf.ShiftLeft();
						}
						ss.setBuf(0, 0);
						ss.setDelim(",");
						ss.setBuf(buf, buf.Len() + 1);
						ss.get(&(p = 0), ctrl_i.ID);
						ss.get(&p, (buf = 0));
						ctrl_i.Coord.left = buf.ToLong();
						ss.get(&p, (buf = 0));
						ctrl_i.Coord.top = buf.ToLong();
						ss.get(&p, (buf = 0));
						ctrl_i.Coord.right = buf.ToLong();
						ss.get(&p, (buf = 0));
						ctrl_i.Coord.bottom = buf.ToLong();
						ss.get(&p, ctrl_i.Style);
					}
				}
				break;
			case CtrlType::ctrlControl:
				{
					if(DialogInfo.IsDialog()) {
						SString type_info;
						ss.setDelim(",");
						ss.setBuf(buf, buf.Len() + 1);
						ss.get(&(p = 0), ctrl_i.Text);
						ctrl_i.Text.Strip().ShiftLeft().TrimRight();
						ss.get(&p, ctrl_i.ID);
						ss.get(&p, (type_info = 0));
						ss.get(&p, ctrl_i.Style);
						if(type_info.Strip().CmpNC("\"SysTreeView32\"") == 0) {
							STRNSCPY(ctrl_i.CtrlT.QtTypeName, "QTreeView");
							ctrl_i.CtrlT.Type = CtrlType::ctrlTreeListBox;
						}
						else if(type_info.CmpNC("\"SysListView32\"") == 0) {
							STRNSCPY(ctrl_i.CtrlT.QtTypeName, "QListView");
							ctrl_i.CtrlT.Type = CtrlType::ctrlListBox;
						}
						else if(ctrl_i.Style.Search("BS_AUTOCHECKBOX", 0, 1, 0)) {
							STRNSCPY(ctrl_i.CtrlT.QtTypeName, "QCheckBox");
							ctrl_i.CtrlT.Type = CtrlType::ctrlCheckBox;
						}
						else {
							STRNSCPY(ctrl_i.CtrlT.QtTypeName, "QRadioButton");
							ctrl_i.CtrlT.Type = CtrlType::ctrlRadioButton;
						}
						ss.get(&p, (buf = 0));
						ctrl_i.Coord.left = buf.ToLong();
						ss.get(&p, (buf = 0));
						ctrl_i.Coord.top = buf.ToLong();
						ss.get(&p, (buf = 0));
						ctrl_i.Coord.right = buf.ToLong();
						ss.get(&p, (buf = 0));
						ctrl_i.Coord.bottom = buf.ToLong();
					}
				}
				break;
			case CtrlType::ctrlDialogEx:
				{
					ss.setBuf(buf, buf.Len() + 1);
					ss.get(&p, (ctrl_i.ID = 0));
					buf.ShiftLeft(ctrl_i.ID.Len());
					ss.setBuf(0, 0);
					ss.setDelim(",");
					ss.setBuf(buf, buf.Len() + 1);
					ss.get(&(p = 0), (buf = 0));
					ctrl_i.Coord.left   = buf.ToLong();
					ss.get(&p, (buf = 0));
					ctrl_i.Coord.top    = buf.ToLong();
					ss.get(&p, (buf = 0));
					ctrl_i.Coord.right  = buf.ToLong();
					ss.get(&p, (buf = 0));
					ctrl_i.Coord.bottom = buf.ToLong();
				}
				break;
			case CtrlType::ctrlCaption:
				ctrl_i.Text = buf;
				break;
			case CtrlType::ctrlFont:
				break;
			case CtrlType::ctrlStyle:
				break;
			case CtrlType::ctrlEnd:
				break;
		}
		ctrl_i.Text.ShiftLeftChr('\n');
		ctrl_i.Text.ShiftLeftChr('\r');
		ctrl_i.Text.ShiftLeftChr('\"');
		ctrl_i.Text.TrimRightChr('\n');
		ctrl_i.Text.TrimRightChr('\r');
		ctrl_i.Text.TrimRightChr('\"');
		ReplaceSpecSymb(ctrl_i.Text);
		LoadExtraData(&ctrl_i);
	}
	if(ok > 0)
		ASSIGN_PTR(pCtrlInfo, ctrl_i);
	return ok;
}

int RcProcessor::PutControl(CtrlInfo * pCtrlInfo, int level /*=0*/)
{
	int ok = -1;
	if(pCtrlInfo && strlen(pCtrlInfo->CtrlT.QtTypeName)) {
		SString tab;
		for(int i = 0; i <= level; i++)
			tab.CatChar('\t');
		if(pCtrlInfo->IsDialog())
			OutBuf.Cat(tab).Cat("<class>Dialog</class>\n");
		OutBuf.Cat(tab).Cat("<widget class=\"").Cat(pCtrlInfo->CtrlT.QtTypeName).Cat("\" name=\"").Cat(pCtrlInfo->ID).Cat("\">\n");
		OutBuf.Cat(tab).Cat("\t<property name=\"geometry\">\n");
		OutBuf.Cat(tab).Cat("\t\t<rect>\n");
		OutBuf.Cat(tab).Cat("\t\t\t<x>").Cat((long)(pCtrlInfo->Coord.left * CoordModifier)).Cat("</x>\n");
		OutBuf.Cat(tab).Cat("\t\t\t<y>").Cat((long)(pCtrlInfo->Coord.top * CoordModifier)).Cat("</y>\n");
		OutBuf.Cat(tab).Cat("\t\t\t<width>").Cat((long)(pCtrlInfo->Coord.right * CoordModifier)).Cat("</width>\n");
		OutBuf.Cat(tab).Cat("\t\t\t<height>").Cat((long)(pCtrlInfo->Coord.bottom * CoordModifier)).Cat("</height>\n");
		OutBuf.Cat(tab).Cat("\t\t</rect>\n");
		OutBuf.Cat(tab).Cat("\t</property>\n");
		if(pCtrlInfo->Text.Strip().Len()) {
			pCtrlInfo->Text.ToUtf8();
			OutBuf.Cat(tab).Cat("\t<property name=\"");
			if(pCtrlInfo->CtrlT.Type == CtrlType::ctrlDialogEx)
				OutBuf.Cat("windowTitle");
			else if(pCtrlInfo->CtrlT.Type == CtrlType::ctrlGroupBox)
				OutBuf.Cat("title");
			else
				OutBuf.Cat("text");
			OutBuf.Cat("\">\n");
			OutBuf.Cat(tab).Cat("\t\t<string>").Cat(pCtrlInfo->Text).Cat("</string>\n");
			OutBuf.Cat(tab).Cat("\t</property>\n");
			if(pCtrlInfo->CtrlT.Type == CtrlType::ctrlCText || pCtrlInfo->CtrlT.Type == CtrlType::ctrlLText)
				OutBuf.Cat(tab).Cat("\t<property name=\"wordWrap\">\n").Cat(tab).Cat("\t\t<bool>true</bool>\n").Cat(tab).Cat("\t</property>\n");
		}
		if(pCtrlInfo->P_Controls)
			for(uint i = 0; i < pCtrlInfo->P_Controls->getCount(); i++)
				PutControl(pCtrlInfo->P_Controls->at(i), level + 1); // @recursion
		{
			SString buf;
			if(pCtrlInfo->Extras.Len()) {
				StringSet ss("`");
				ss.setBuf(pCtrlInfo->Extras, pCtrlInfo->Extras.Len() + 1);
				for(uint i = 1, p = 0; ss.get(&p, (buf = 0)) > 0; i++) {
					OutBuf.Cat(tab).Cat("\t<property name=\"PpyExtra").Cat((long)i).Cat("\">\n");
					OutBuf.Cat(tab).Cat("\t\t<string>").Cat(buf).Cat("</string>\n");
					OutBuf.Cat(tab).Cat("\t</property>\n");
				}
			}
			if(pCtrlInfo->Cmd.Len()) {
				OutBuf.Cat(tab).Cat("\t<property name=\"PpyCmd\">\n");
				OutBuf.Cat(tab).Cat("\t\t<string>").Cat(pCtrlInfo->Cmd).Cat("</string>\n");
				OutBuf.Cat(tab).Cat("\t</property>\n");
			}
			if(pCtrlInfo->LinkedCtrlID.Len()) {
				OutBuf.Cat(tab).Cat("\t<property name=\"PpyLinkedCtrlID\">\n");
				OutBuf.Cat(tab).Cat("\t\t<string>").Cat(pCtrlInfo->LinkedCtrlID).Cat("</string>\n");
				OutBuf.Cat(tab).Cat("\t</property>\n");
			}
		}
		OutBuf.Cat(tab).Cat("</widget>\n");
		ok = 1;
	}
	return ok;
}

int RcProcessor::PutDialog(CtrlInfo * pDlgInfo)
{
	int ok = -1;
	SFile out;
	if(pDlgInfo && pDlgInfo->IsDialog()) {
		SString path;
		OutBuf.CopyFrom("<ui version=\"4.0\">\n");
		THROW(PutControl(pDlgInfo));
		OutBuf.Cat("\t<designerdata>\n");
		OutBuf.Cat("\t\t<property name=\"gridDeltaX\">\n");
		OutBuf.Cat("\t\t\t<number>").Cat(GridSize).Cat("</number>\n");
		OutBuf.Cat("\t\t</property>\n");
		OutBuf.Cat("\t\t<property name=\"gridDeltaY\">\n");
		OutBuf.Cat("\t\t\t<number>").Cat(GridSize).Cat("</number>\n");
		OutBuf.Cat("\t\t</property>\n");
		OutBuf.Cat("\t\t<property name=\"gridSnapX\">\n");
		OutBuf.Cat("\t\t\t<bool>true</bool>\n");
		OutBuf.Cat("\t\t</property>\n");
		OutBuf.Cat("\t\t<property name=\"gridSnapY\">\n");
		OutBuf.Cat("\t\t\t<bool>true</bool>\n");
		OutBuf.Cat("\t\t</property>\n");
		OutBuf.Cat("\t\t<property name=\"gridVisible\">\n");
		OutBuf.Cat("\t\t\t<bool>true</bool>\n");
		OutBuf.Cat("\t\t</property>\n");
		OutBuf.Cat("\t</designerdata>\n");
		OutBuf.Cat("</ui>");
		(path = Path).SetLastSlash().Cat(pDlgInfo->ID).Cat(".ui");
		THROW(out.Open(path, SFile::mWrite));
		out.Write((const char*)OutBuf, OutBuf.Len());
		ok = 1;
	}
	CATCHZOK
	return ok;
}

void main(int argc, char ** argv)
{
	char str[128];
	RcProcessor rc_prcssr;
	if(argc == 3 && rc_prcssr.Init(argv[1], argv[2]) <= 0)
		printf("Usage: RcToUI.exe ppw.rc ppdlg.rc");
	else if(rc_prcssr.Run() < 0) {
		sprintf(str, "Processing error", argv[1]);
		sprintf(str, "Processing error", argv[2]);
		error(str);
	}
}

//
// UITagParser
//
#define MAXTAG_VALSIZE 64

#if 0 // {

struct Tag {
	char Name[32];
	long Type;
};

const char

class UITagParser : XTagParser {
public:
	UIProcessor();

	int Init(const char * pRcwFile, const char * pRcFile);
	int ProcessNext(const char * pUIPath);
protected:
	virtual int SLAPI ProcessTag(const char * pTag, long);
private:
	SFile UIFile;
	SFile RcFile;
	SFile RcwFile;
};

int UITagParser::Init(const char * pRcwPath, const char * pRcPath)
{
	RcFile.Open(pRcPath, SFile::mWrite);
	RcwFile.Open(pRcwPath, SFile::mWrite);
	return RcFile.IsValid() && RcwFile.IsValid();
}

int UITagParser::ProcessNext(const char * pUIPath)
{
	int ok = 1;
	SString path;
	SDirEntry e;
	for(SDirec sdir(pUIPath); sdir.Next(&e) > 0 && ok > 0; ) {
		path.CopyFrom(pUIPath).SetLastSlash().Cat(e.FileName);
		ok = Run(path);
	}
}

int UITagParser::ProcessTag(const char * pTag, long)
{
	int tok = tokErr;
	char tag_buf[64];
	SString tag, buf, buf2;
	buf.CopyFrom(pTag);
	buf.Divide(' ', tag, buf2);
	while((tok = GetToken(tag, tag_buf, sizeof(tag_buf))) != tokEOF && tok != tokEndTag && tok != tokErr) {
		if(tok == tokTag) {
			SymbNum = 0;
			if(ProcessTag(tag_buf, 0) == tokErr) {
				tok = tokErr;
				break;
			}
		}
		else {
			if(SymbNum < (long) MAXTAG_VALSIZE && tag_buf[0] != '\n' && tag_buf[0] != '\r' &&
				tag_buf[0] != '\0') {
				P_TagValBuf[SymbNum] = tag_buf[0];
				SymbNum++;
			}
		}
	}
	if(tok != tokErr) {
		if(SymbNum < (long) MAXTAG_VALSIZE)
			P_TagValBuf[SymbNum] = '\0';
		if(!SaveTagVal(pTag))
			tok = tokErr;
	}
	return tok;
}

int SLAPI UITagParser::SaveTagVal(const char * pTag)
{
	int    ok = 1;
	int    tag_idx = 0;
	char   buf[MAXTAG_VALSIZE];
	memzero(buf, sizeof(buf));

	for(uint i = 0; i < tags_count; i++) {
		if(strnicmp(pTag, Tags[i].Name) == 0)
			tag = Tags[i].Symb;
	}
	switch(tag) {

	}
	/*
	if(PPSearchSubStr(TagNamesStr, &tag_idx, pTag, ALBATROS_MAXTAGSIZE) > 0) {
#ifdef __WIN32__
		if(P_TagValBuf && strip(P_TagValBuf)[0] != 0)
			decode64(P_TagValBuf, strlen(P_TagValBuf), buf, 0);
#endif
		CharToOem(buf, buf);
		switch(tag_idx) {
			// if get head items
			case PPALTAGNAM_ORDDT:
				strtodate(buf, DATF_DMY, &P_Order->Head.OrderDate);
				break;
			case PPALTAGNAM_ORDCODE:
				STRNSCPY(P_Order->Head.OrderCode, buf);
				break;
			case PPALTAGNAM_CLID:
				if(!strtolong(buf, &P_Order->Head.ClientID))
					ok = 0;
				break;
			case PPALTAGNAM_CLNAM:
				STRNSCPY(P_Order->Head.ClientName, buf);
				break;
			case PPALTAGNAM_CLINN:
				STRNSCPY(P_Order->Head.ClientINN, buf);
				break;
			case PPALTAGNAM_CLCITY:
				STRNSCPY(P_Order->Head.ClientCity, buf);
				break;
			case PPALTAGNAM_CLADDR:
				STRNSCPY(P_Order->Head.ClientAddr, buf);
				break;
			case PPALTAGNAM_CLPHONE:
				STRNSCPY(P_Order->Head.ClientPhone, buf);
				break;
			case PPALTAGNAM_CLMAIL:
				STRNSCPY(P_Order->Head.ClientMail, buf);
				break;
			case PPALTAGNAM_CLBNKACCT:
				STRNSCPY(P_Order->Head.ClientBankAcc, buf);
				break;
			case PPALTAGNAM_ORDAMT:
				if(!strtodoub(buf, &P_Order->Head.OrderAmount))
					ok = 0;
				break;
			case PPALTAGNAM_PCTDIS:
				if(!strtodoub(buf, &P_Order->Head.PctDis))
					ok = 0;
				break;
			// if get tag AlbatrosOrderItem
			case PPALTAGNAM_ALBORDITEM:
				P_Order->Items.insert(&OrderItem);
				MEMSZERO(OrderItem);
				break;
			// if get item item
			case PPALTAGNAM_GOODSID:
				if(!strtolong(buf, &OrderItem.GoodsID))
					ok = 0;
				break;
			case PPALTAGNAM_GOODSNAM:
				STRNSCPY(OrderItem.GoodsName, buf);
				break;
			case PPALTAGNAM_GOODSCODE:
				STRNSCPY(OrderItem.GoodsCode, buf);
				break;
			case PPALTAGNAM_UPP:
				if(!strtodoub(buf, &OrderItem.UnitsPerPack))
					ok = 0;
				break;
			case PPALTAGNAM_QTTY:
				if(!strtodoub(buf, &OrderItem.Qtty))
					ok = 0;
				break;
			case PPALTAGNAM_PRICE:
				if(!strtodoub(buf, &OrderItem.Price))
					ok = 0;
				break;
			case PPALTAGNAM_DISC:
				if(!strtodoub(buf, &OrderItem.Discount))
					ok = 0;
				break;
			case PPALTAGNAM_AMOUNT:
				if(!strtodoub(buf, &OrderItem.Amount))
					ok = 0;
				break;
			default:
				ok = -1;
		}
	}
	else
		ok = 0;
	*/
	return ok ? ok : (SLibError = SLERR_INVFORMAT, ok);
}

#endif // } 0
