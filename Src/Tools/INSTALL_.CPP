// INSTALL_.CPP
// Copyright (c) A.Sobolev 1998, 2011
//
#include <pp.h>
#include <ppdlgs.h>
#pragma hdrstop
#include <strstore.h>

#undef PPErrCode

int PPErrCode;

const char * SLAPI onecstr(char c) // PPUTIL.CPP
{
	static char s[2];
	s[0] = c;
	s[1] = 0;
	return s;
}

TView * SLAPI ValidView(TView * v) // PPTVUTIL.CPP
	{ return APPL->validView(v); }

ushort SLAPI ExecView(TView * v) // PPTVUTIL.CPP
	{ return v ? APPL->P_DeskTop->execView(v) : cmError; }

int SLAPI CheckDialogPtr(void * d, int /*genErrMsg*/)
{
	int ok = 1;
	TDialog ** dlg = (TDialog**)d;
	if(ValidView(*dlg) == 0) {
		*(void**)d = 0;
		ok = (PPErrCode = PPERR_DLGLOADFAULT, 0);
		/*
		if(genErrMsg)
			PPError();
		*/
	}
	return ok;
}

int SLAPI PPMessage(uint options, int msgcode, char * addInfo) // PPMSG.CPP
{
	char * buf  = 0;
	char * fbuf = 0;
	int group;
	int addcode = 0;
	switch(options & 0x00ff) {
		case mfError:
			group = PPMSG_ERROR;
			switch(msgcode) {
				case PPERR_DBENGINE:
					if(BtrError) {
						group   = addcode = msgcode;
						msgcode = BtrError;
					}
					break;
				case PPERR_SLIB:
					if(SLibError) {
						group   = addcode = msgcode;
						msgcode = SLibError;
					}
					break;
				case PPERR_DBQUERY:
					break;
			}
			break;
		case mfWarn: group = PPMSG_WARNING;      break;
		case mfInfo: group = PPMSG_INFORMATION;  break;
		case mfConf: group = PPMSG_CONFIRMATION; break;
		default:
			return 0;
	}
	if(!(buf = new char[PP_MSGLEN+1])) {
		addInfo = "Недостаточно памяти";
		msgcode = 0;
	}
	else
		*buf = 3; // Сообщение будет центрироваться
	if(msgcode) {
__loadstring:
		if(!PPLoadString(group, msgcode, buf+1, PP_MSGLEN))
			if(SLibError == SLERR_NOFOUND && addcode) {
				group   = PPMSG_ERROR;
				msgcode = addcode;
				addcode = 0;
				goto __loadstring;
			}
			else {
				sprintf(buf+1, "Невозможно загрузить сообщение: (%d)%d",
					group, msgcode);
				addInfo = buf;
				msgcode = 0;
			}
		else if(addInfo && (fbuf = new char [1024]) != 0) {
			sprintf(fbuf, buf, addInfo);
			delete buf;
			buf = fbuf;
		}
	}
	int r = PPOutputMessage(msgcode ? buf : addInfo, options);
	delete buf;
	return r;
}

int SLAPI PPOutputMessage(char * msg, uint options) // PPMSG.CPP
{
	return messageBox(msg, options);
}

long   WaitPeriod = 0;               //
static TDialog * waitDialog = 0;     //
static int       active = 0;         // PPMSG.CPP
static TView   * prevView = 0;       //
static StringStore * _PPStrStore = 0;//

int SLAPI PPWait(int begin) // PPMSG.CPP
{
	if(begin) {
		if(!active) {
			if(waitDialog == 0) {
				waitDialog = (TDialog*)ValidView(new TDialog(DLG_WAIT));
				if(waitDialog == 0)
					return 0;
				waitDialog->getCtrlView(CTL_WAIT_TEXT)->growMode |= (gfGrowHiX | gfGrowHiY);
			}
			prevView = APPL->P_DeskTop->current;
			APPL->P_DeskTop->insert(waitDialog);
			active = 1;
			WaitPeriod = clock();
		}
	}
	else if(active && waitDialog) {
	   	WaitPeriod = clock() - WaitPeriod;
	   	ushort save;
	   	if(prevView) {
			save = prevView->options;
			prevView->options |= ofSelectable;
		}
		TObject::destroy(waitDialog);
		waitDialog = 0;
		if(prevView) {
			prevView->options = save;
			prevView = 0;
		}
		active = 0;
	}
	return active;
}

int SLAPI PPWaitMsg(char * msg) //PPMSG.CPP
{
	if(active && waitDialog) {
		TStaticText * st = (TStaticText*)waitDialog->getCtrlView(CTL_WAIT_TEXT);
		if(st) {
			int l = strlen(msg);
			char buf[80];
			if(l > 70)
				msg[70] = 0;
			buf[0] = '\003';
			strcpy(buf+1, msg);
			TRect r = waitDialog->getBounds();
			int s = MAX(21, l+4);
			r.a.x = r.b.x - s;
			waitDialog->locate(r);
			st->setText(buf);
			st->drawView();
		}
	}
	return 1;
}

int SLAPI PPWaitMsg(int msgGrpID, int msgID, char * addInfo) // PPMSG.CPP
{
	char buf[128], msg[80], * p;
	if(PPLoadString(msgGrpID, msgID, msg, sizeof(msg))) {
		if(addInfo)
			sprintf(p = buf, msg, addInfo);
		else
			p = msg;
		return PPWaitMsg(p);
	}
	return 0;
}

int SLAPI PPLoadString(int group, int code, char * buf, size_t bufLen) // PPMSG.CPP
{
	buf[0] = 0;
	if(group == 0 || code == 0)
		return 1;
	return _PPStrStore ? _PPStrStore->getString(group, code, buf, bufLen) : 0;
}

