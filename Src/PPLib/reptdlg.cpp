// REPTDLG.CPP
// Copyright (c) A.Sobolev 2004, 2005, 2006, 2007, 2008, 2010, 2011, 2013, 2016, 2017, 2019, 2020, 2021, 2022, 2025
//
#include <pp.h>
#pragma hdrstop
//
//
//
class RepDailyDialog : public TDialog {
public:
	RepDailyDialog() : TDialog(DLG_REP_DAYLY)
	{
	}
	virtual int TransmitData(int dir, void * pData) // DateRepeating *
	{
		int    s = 0;
		if(dir > 0) {
			Data = *static_cast<const DateRepeating *>(pData);
			long   temp_quant_sec = Data.Dtl.D.QuantSec;
			setCtrlData(CTL_REPEATING_NUMSECS, &temp_quant_sec);
			setCtrlData(CTL_REPEATING_NUMPRD, &Data.Dtl.D.NumPrd);
			s = 1;
		}
		else if(dir < 0) {
			Data.Prd = PRD_DAY;
			getCtrlData(CTL_REPEATING_NUMPRD, &Data.Dtl.D.NumPrd);
			long temp_quant_sec = 0;
			getCtrlData(CTL_REPEATING_NUMSECS, &temp_quant_sec);
			Data.Dtl.D.QuantSec = (uint16)temp_quant_sec;
			Data.Dtl.D.QuantSec = (Data.Dtl.D.QuantSec < 0) ? 0 : Data.Dtl.D.QuantSec;
			Data.Dtl.D.QuantSec = (Data.Dtl.D.QuantSec > 43200) ? 43200 : Data.Dtl.D.QuantSec; // ���� ������ 12 ����� = 43200 ���
			Data.Dtl.D.NumPrd = (Data.Dtl.D.QuantSec != 0) ? 1 : Data.Dtl.D.NumPrd;
			if(pData)
				*static_cast<DateRepeating *>(pData) = Data;
			s = 1;
		}
		else
			s = TDialog::TransmitData(dir, pData);
		return s;
	}
private:
	virtual int FASTCALL valid(ushort cmd)
	{
		if(cmd == cmOK) {
			TransmitData(-1, 0);
			if(Data.Dtl.D.NumPrd <= 0 || Data.Dtl.D.NumPrd > 366)
				return PPSetError(PPERR_USERINPUT);
		}
		return 1;
	}
	DateRepeating Data;
};
//
//
//
class RepWeeklyDialog : public TDialog {
public:
	RepWeeklyDialog() : TDialog(DLG_REP_WEEKLY)
	{
	}
	virtual int TransmitData(int dir, void * pData) // DateRepeating *
	{
		int    s = 0;
		if(dir > 0) {
			Data = *static_cast<const DateRepeating *>(pData);
			const long weekdays = static_cast<long>(Data.Dtl.W.Weekdays);
			setCtrlData(CTL_REPEATING_NUMPRD, &Data.Dtl.W.NumPrd);
			AddClusterAssoc(CTL_REPEATING_WEEKDAYS, 0, 0x0001);
			AddClusterAssoc(CTL_REPEATING_WEEKDAYS, 1, 0x0002);
			AddClusterAssoc(CTL_REPEATING_WEEKDAYS, 2, 0x0004);
			AddClusterAssoc(CTL_REPEATING_WEEKDAYS, 3, 0x0008);
			AddClusterAssoc(CTL_REPEATING_WEEKDAYS, 4, 0x0010);
			AddClusterAssoc(CTL_REPEATING_WEEKDAYS, 5, 0x0020);
			AddClusterAssoc(CTL_REPEATING_WEEKDAYS, 6, 0x0040);
			SetClusterData(CTL_REPEATING_WEEKDAYS, weekdays);
			s = 1;
		}
		else if(dir < 0) {
			long   weekdays = 0;
			Data.Prd = PRD_WEEK;
			getCtrlData(CTL_REPEATING_NUMPRD, &Data.Dtl.W.NumPrd);
			GetClusterData(CTL_REPEATING_WEEKDAYS, &weekdays);
			Data.Dtl.W.Weekdays = (uint8)weekdays;
			if(pData)
				*static_cast<DateRepeating *>(pData) = Data;
			s = 1;
		}
		else
			s = TDialog::TransmitData(dir, pData);
		return s;
	}
private:
	virtual int FASTCALL valid(ushort cmd)
	{
		if(cmd == cmOK) {
			TransmitData(-1, 0);
			if(Data.Dtl.W.NumPrd <= 0 || Data.Dtl.W.NumPrd > 100)
				return PPSetError(PPERR_USERINPUT);
			else if((Data.Dtl.W.Weekdays & 0x7f) == 0)
				return PPSetError(PPERR_NOWEEKDAYS);
		}
		return 1;
	}
	DateRepeating Data;
};
//
//
//
class RepMonthlyDialog : public TDialog {
public:
	RepMonthlyDialog() : TDialog(DLG_REP_MONTHLY), __Lock(0), Kind(1), MonthCount(1), MonthNo(1), DayOfMonth(1), WeekNo(1), DayOfWeek(1)
	{
	}
	virtual int TransmitData(int dir, void * pData)
	{
		int    s = 1;
		if(dir > 0) {
			Data = *static_cast<const DateRepeating *>(pData);
			Data.Prd = PRD_MONTH;
			Data.GetMonthlyPeriod(&MonthCount, &MonthNo);
			Kind = static_cast<long>(Data.RepeatKind);
			PrevKind = Kind;
			{
				__Lock = 1;
				setCtrlData(CTL_REPEATING_NUMPRD, &MonthCount);
				setCtrlData(CTL_REPEATING_DTLNM,  &MonthNo);
				__Lock = 0;
			}
			AddClusterAssoc(CTL_REPEATING_KIND, 0, 1);
			AddClusterAssoc(CTL_REPEATING_KIND, 1, 2);
			SetClusterData(CTL_REPEATING_KIND, Kind);
			if(Kind == 1) {
				DayOfMonth = Data.Dtl.ME.DayOfMonth;
				setCtrlData(CTL_REPEATING_DTLMD, &DayOfMonth);
				disableCtrls(0, CTL_REPEATING_DTLMD, 0);
				disableCtrls(1, CTLSEL_REPEATING_DTLWN, CTLSEL_REPEATING_DTLWD, 0);
			}
			else {
				WeekNo = Data.Dtl.MY.WeekNo;
				DayOfWeek = Data.Dtl.MY.DayOfWeek;
				SetupStringCombo(this, CTLSEL_REPEATING_DTLWN,  PPTXT_WEEKNO,   WeekNo);
				SetupStringCombo(this, CTLSEL_REPEATING_DTLWD,  PPTXT_WEEKDAYS, DayOfWeek);
				disableCtrls(1, CTL_REPEATING_DTLMD, 0);
				disableCtrls(0, CTLSEL_REPEATING_DTLWN, CTLSEL_REPEATING_DTLWD, 0);
			}
			// @debug {
			{
				DateRepeating test_data;
				TransmitData(-1, &test_data);
				assert(test_data == Data);
			}
			// } @debug
		}
		else if(dir < 0) {
			Helper_GetData();
			if(Kind == 1) {
				Data.SetMonthly(MonthCount, MonthNo, DayOfMonth);
			}
			else {
				Data.SetMonthly(MonthCount, MonthNo, WeekNo, DayOfWeek);
			}
			if(pData)
				*static_cast<DateRepeating *>(pData) = Data;
		}
		else
			s = TDialog::TransmitData(dir, pData);
		return s;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isClusterClk(CTL_REPEATING_KIND)) {
			GetClusterData(CTL_REPEATING_KIND, &Kind);
			if(Kind != PrevKind) {
				Data.Init(Data.Prd, Kind, ZERODATE);
				if(Kind == 1) {
					setCtrlData(CTL_REPEATING_DTLMD, &DayOfMonth);
					disableCtrls(0, CTL_REPEATING_DTLMD, 0);
					disableCtrls(1, CTLSEL_REPEATING_DTLWN, CTLSEL_REPEATING_DTLWD, 0);
				}
				else {
					SetupStringCombo(this, CTLSEL_REPEATING_DTLWN,  PPTXT_WEEKNO,   WeekNo);
					SetupStringCombo(this, CTLSEL_REPEATING_DTLWD,  PPTXT_WEEKDAYS, DayOfWeek);
					disableCtrls(1, CTL_REPEATING_DTLMD, 0);
					disableCtrls(0, CTLSEL_REPEATING_DTLWN, CTLSEL_REPEATING_DTLWD, 0);
				}
				PrevKind = Kind;
			}
		}
		else if(event.isCmd(cmInputUpdated)) {
			if(!__Lock) {
				__Lock = 1;
				if(event.isCtlEvent(CTL_REPEATING_NUMPRD)) {
					if(Helper_GetData() == 100)
						setCtrlData(CTL_REPEATING_DTLNM, &MonthNo);
				}
				else if(event.isCtlEvent(CTL_REPEATING_DTLNM)) {
					if(Helper_GetData() == 100)
						setCtrlData(CTL_REPEATING_NUMPRD, &MonthCount);
				}
				__Lock = 0;
			}
		}
		else
			return;
		clearEvent(event);
	}
	int    Helper_GetData();

	DateRepeating Data;
	long   PrevKind;
	long   Kind;
	int    MonthCount;
	int    MonthNo;
	int    DayOfMonth;
	int    WeekNo;
	int    DayOfWeek;
	int    __Lock;
};

int RepMonthlyDialog::Helper_GetData()
{
	int    ok = 1;
	getCtrlData(CTL_REPEATING_NUMPRD, &MonthCount);
	getCtrlData(CTL_REPEATING_DTLNM,  &MonthNo);
	GetClusterData(CTL_REPEATING_KIND, &Kind);
	Data.Prd = PRD_MONTH;
	Data.RepeatKind = static_cast<int16>(Kind);
	if(Kind == 1) {
		getCtrlData(CTL_REPEATING_DTLMD, &DayOfMonth);
		if(Data.SetMonthly(MonthCount, MonthNo, DayOfMonth) == 100)
			ok = 100;
		DayOfMonth = (int)Data.Dtl.ME.DayOfMonth;
	}
	else {
		getCtrlData(CTLSEL_REPEATING_DTLWN, &WeekNo);
		getCtrlData(CTLSEL_REPEATING_DTLWD, &DayOfWeek);
		if(Data.SetMonthly(MonthCount, MonthNo, WeekNo, DayOfWeek) == 100)
			ok = 100;
		WeekNo = static_cast<int>(Data.Dtl.MY.WeekNo);
		DayOfWeek = static_cast<int>(Data.Dtl.MY.DayOfWeek);
	}
	Data.GetMonthlyPeriod(&MonthCount, &MonthNo);
	return ok;
}
//
//
//
class RepAnnDialog : public TDialog {
public:
	RepAnnDialog() : TDialog(DLG_REP_ANN)
	{
	}
	virtual int TransmitData(int dir, void * pData)
	{
		int    s = 1;
		if(dir > 0) {
			Data = *static_cast<const DateRepeating *>(pData);
			long   kind = static_cast<long>(Data.RepeatKind);
			PrevKind = kind;
			SetupStringCombo(this, CTLSEL_REPEATING_DTLMON, PPTXT_MONTHES, Data.Dtl.AY.Month);
			AddClusterAssoc(CTL_REPEATING_KIND, 0, 1);
			AddClusterAssoc(CTL_REPEATING_KIND, 1, 2);
			SetClusterData(CTL_REPEATING_KIND, kind);
			if(kind == 1) {
				setCtrlData(CTL_REPEATING_DTLMD, &Data.Dtl.AE.DayOfMonth);
			}
			else {
				SetupStringCombo(this, CTLSEL_REPEATING_DTLWN,  PPTXT_WEEKNO,   Data.Dtl.AY.WeekNo);
				SetupStringCombo(this, CTLSEL_REPEATING_DTLWD,  PPTXT_WEEKDAYS, Data.Dtl.AY.DayOfWeek);
			}
		}
		else if(dir < 0) {
			long   kind = 0;
			long   month = getCtrlLong(CTLSEL_REPEATING_DTLMON);
			Data.Dtl.AY.Month = static_cast<int16>(month);
			GetClusterData(CTL_REPEATING_KIND, &kind);
			Data.Prd = PRD_ANNUAL;
			Data.RepeatKind = static_cast<int16>(kind);
			if(kind == 1) {
				getCtrlData(CTL_REPEATING_DTLMD, &Data.Dtl.AE.DayOfMonth);
			}
			else {
				long   temp_id = 0;
				if(getCtrlData(CTLSEL_REPEATING_DTLWN, &temp_id))
					Data.Dtl.AY.WeekNo = static_cast<uint8>(temp_id);
				if(getCtrlData(CTLSEL_REPEATING_DTLWD, &temp_id))
					Data.Dtl.AY.DayOfWeek = static_cast<uint8>(temp_id);
			}
			if(pData)
				*static_cast<DateRepeating *>(pData) = Data;
		}
		else
			s = TDialog::TransmitData(dir, pData);
		return s;
	}
private:
	//virtual int FASTCALL valid(ushort cmd) // @cmValidateCommand
	bool ValidateCommand(long cmd)
	{
		int    ok = 1;
		uint   sel = 0;
		if(cmd == cmOK) {
			TransmitData(-1, 0);
			THROW_PP(oneof2(Data.RepeatKind, 1, 2), PPERR_USERINPUT);
			if(Data.RepeatKind == 1) {
				sel = CTL_REPEATING_DTLMD;
				THROW_PP(Data.Dtl.AE.DayOfMonth >= 1 && Data.Dtl.AE.DayOfMonth <= 31, PPERR_USERINPUT);
			}
			else {
				sel = CTLSEL_REPEATING_DTLWD;
				THROW_PP(Data.Dtl.AY.DayOfWeek >= 1 && Data.Dtl.AY.DayOfWeek <= 7, PPERR_USERINPUT);
				sel = CTLSEL_REPEATING_DTLWN;
				THROW_PP(Data.Dtl.AY.WeekNo >= 1 && Data.Dtl.AY.WeekNo <= 5, PPERR_USERINPUT);
			}
		}
		CATCHZOKPPERRBYDLG
		return ok;
	}
	DECL_HANDLE_EVENT
	{
		if(event.isCmd(cmValidateCommand)) { // @v12.2.6
			const long validated_command = event.message.infoLong;
			if(!ValidateCommand(validated_command)) {
				NegativeReplyOnValidateCommand(event);
			}
		}
		TDialog::handleEvent(event);
		if(event.isCmd(cmClusterClk)) {
			long   kind = 0;
			GetClusterData(CTL_REPEATING_KIND, &kind);
			if(kind != PrevKind) {
				Data.Init(Data.Prd, kind, ZERODATE);
				if(kind == 1) {
					setCtrlData(CTL_REPEATING_DTLMD, &Data.Dtl.AE.DayOfMonth);
					disableCtrls(0, CTL_REPEATING_DTLMD, 0);
					disableCtrls(1, CTLSEL_REPEATING_DTLWN, CTLSEL_REPEATING_DTLWD, 0);
				}
				else {
					SetupStringCombo(this, CTLSEL_REPEATING_DTLWN,  PPTXT_WEEKNO,   Data.Dtl.AY.WeekNo);
					SetupStringCombo(this, CTLSEL_REPEATING_DTLWD,  PPTXT_WEEKDAYS, Data.Dtl.AY.DayOfWeek);
					disableCtrls(1, CTL_REPEATING_DTLMD, 0);
					disableCtrls(0, CTLSEL_REPEATING_DTLWN, CTLSEL_REPEATING_DTLWD, 0);
				}
				PrevKind = kind;
			}
			clearEvent(event);
		}
	}
	DateRepeating Data;
	long   PrevKind;
};
//
//
//
class RepAfterPrdDialog : public TDialog {
public:
	RepAfterPrdDialog() : TDialog(DLG_REP_APRD)
	{
	}
	virtual int TransmitData(int dir, void * pData)
	{
		int    s = 1;
		if(dir > 0) {
			Data = *static_cast<const DateRepeating *>(pData);
			long   kind = static_cast<long>(Data.RepeatKind);
			setCtrlLong(CTL_REPEATING_NUMPRD, static_cast<long>(Data.Dtl.RA.NumPrd));
			AddClusterAssocDef(CTL_REPEATING_KIND,  0, PRD_DAY);
			AddClusterAssoc(CTL_REPEATING_KIND,  1, PRD_WEEK);
			AddClusterAssoc(CTL_REPEATING_KIND,  2, PRD_MONTH);
			AddClusterAssoc(CTL_REPEATING_KIND,  3, PRD_ANNUAL);
			SetClusterData(CTL_REPEATING_KIND, static_cast<long>(Data.RepeatKind));
			AddClusterAssocDef(CTL_REPEATING_DTLAS, 0, 1);
			AddClusterAssoc(CTL_REPEATING_DTLAS, 1, 0);
			SetClusterData(CTL_REPEATING_DTLAS, Data.Dtl.RA.AfterStart);
		}
		else if(dir < 0) {
			long   v = 0;
			Data.Dtl.RA.NumPrd = static_cast<int16>(getCtrlLong(CTL_REPEATING_NUMPRD));
			GetClusterData(CTL_REPEATING_KIND, &v);
			Data.RepeatKind = static_cast<int16>(v);
			GetClusterData(CTL_REPEATING_DTLAS, &v);
			Data.Dtl.RA.AfterStart = static_cast<int16>(v);
			if(pData)
				*static_cast<DateRepeating *>(pData) = Data;
		}
		else
			s = TDialog::TransmitData(dir, pData);
		return s;
	}
private:
	/* @v12.2.6 virtual int FASTCALL RepAfterPrdDialog::valid(ushort cmd)
	{
		if(cmd == cmOK) {
			TransmitData(-1, 0);
			if(Data.Dtl.RA.NumPrd == 0) {
				selectCtrl(CTL_REPEATING_NUMPRD);
				return PPSetError(PPERR_USERINPUT);
			}
		}
		return 1;
	}*/
	DECL_HANDLE_EVENT
	{
		if(event.isCmd(cmValidateCommand)) {
			const long cmd = event.message.infoLong;
			bool  local_ok = true;
			if(cmd == cmOK) {
				TransmitData(-1, 0);
				if(Data.Dtl.RA.NumPrd == 0) {
					selectCtrl(CTL_REPEATING_NUMPRD);
					PPSetError(PPERR_USERINPUT);
					NegativeReplyOnValidateCommand(event);
					local_ok = false;
				}
			}
		}
		TDialog::handleEvent(event);
	}
	DateRepeating Data;
	long   PrevKind;
};
//
//
//
static void LogTest(PPLogger & rLogger, LDATE dt)
{
	char   date_buf[32];
	char   log_buf[256];
	datefmt(&dt, DATF_DMY|DATF_CENTURY, date_buf);
	sprintf(log_buf, "%10s%20d", date_buf, dayofweek(&dt, 1));
	rLogger.Log(log_buf);
}

void RepeatingDialog::test()
{
	PPLogger logger;
	LDATE  dt;
	DateRepeating data;
	getDTS(&data);
	for(DateRepIterator dr_iter(data, InitDate, ZERODATE, 24); (dt = dr_iter.Next()) != ZERODATE;)
		LogTest(logger, dt);
}

RepeatingDialog::RepeatingDialog(uint options) : EmbedDialog(DLG_REPEATING), InitDate(ZERODATE), Options(options)
{
	P_Data = (Options & fEditTime) ? (new DateTimeRepeating) : (new DateRepeating);
	showCtrl(CTL_REPEATING_DURATION, LOGIC(Options & fEditDuration));
	SetupTimePicker(this, CTL_REPEATING_RUNTIME, CTLTM_REPEATING_RUNTIME);
}

RepeatingDialog::~RepeatingDialog()
{
	if(Options & fEditTime)
		delete static_cast<DateTimeRepeating *>(P_Data);
	else
		delete P_Data;
}

void RepeatingDialog::setInitDate(LDATE dt)
{
	InitDate = dt;
}

int RepeatingDialog::embedOneChild(TDialog * pDlg)
{
	if(CheckDialogPtrErr(&pDlg)) {
		Embed(pDlg);
		setChildPos(CTL_REPEATING_PRD);
		return 1;
	}
	else
		return 0;
}

int RepeatingDialog::embedChild(long prd)
{
	int    ok = 0;
	switch(prd) {
		case PRD_DAY:            ok = embedOneChild(new RepDailyDialog);   break;
		case PRD_WEEK:           ok = embedOneChild(new RepWeeklyDialog);  break;
		case PRD_MONTH:          ok = embedOneChild(new RepMonthlyDialog); break;
		case PRD_ANNUAL:         ok = embedOneChild(new RepAnnDialog);     break;
		case PRD_REPEATAFTERPRD: ok = embedOneChild(new RepAfterPrdDialog); break;
		default: Embed(0); ok = 1; break;
	}
	return ok;
}

IMPL_HANDLE_EVENT(RepeatingDialog)
{
	EmbedDialog::handleEvent(event);
	if(TVCOMMAND)
		if(TVCMD == cmClusterClk) {
			long prd = GetClusterData(CTL_REPEATING_PRD);
			if(prd != PrevPrd) {
				if(Options & fEditTime)
					;//((DateTimeRepeating*)P_Data)->Init(prd);
				else
					P_Data->Init(prd, 1, ZERODATE);
				embedChild(prd);
				CALLPTRMEMB(P_ChildDlg, TransmitData(+1, P_Data));
				PrevPrd = prd;
			}
		}
		else if(TVCMD == cmTest)
			test();
		else
			return;
	else
		return;
	clearEvent(event);
}

int RepeatingDialog::setDTS(const DateRepeating * pData)
{
	if(Options & fEditTime) {
		*static_cast<DateTimeRepeating *>(P_Data) = *static_cast<const DateTimeRepeating *>(pData);
		if(static_cast<const DateTimeRepeating *>(P_Data)->Time == ZEROTIME)
			getcurtime(&static_cast<DateTimeRepeating *>(P_Data)->Time);
		setCtrlData(CTL_REPEATING_RUNTIME, &static_cast<DateTimeRepeating *>(P_Data)->Time);
	}
	else {
		HWND   hwnd = ::GetDlgItem(H(), CTL_REPEATING_RUNTIME);
		TLabel * p_label = GetCtrlLabel(CTL_REPEATING_RUNTIME);
		if(p_label) {
			HWND hwnd_l = GetDlgItem(H(), p_label->GetId());
			ShowWindow(hwnd_l, SW_HIDE);
		}
		ShowWindow(hwnd, SW_HIDE);
		showCtrl(CTLTM_REPEATING_RUNTIME, false);
		*P_Data = *pData;
	}
	SETIFZ(P_Data->Prd, PRD_WEEK);
	DisableClusterItem(CTL_REPEATING_PRD, 5, !(Options & fEditRepeatAfterItem));
	AddClusterAssoc(CTL_REPEATING_PRD, 0, PRD_UNDEF);
	AddClusterAssocDef(CTL_REPEATING_PRD, 1, PRD_DAY);
	AddClusterAssoc(CTL_REPEATING_PRD, 2, PRD_WEEK);
	AddClusterAssoc(CTL_REPEATING_PRD, 3, PRD_MONTH);
	AddClusterAssoc(CTL_REPEATING_PRD, 4, PRD_ANNUAL);
	AddClusterAssoc(CTL_REPEATING_PRD, 5, PRD_REPEATAFTERPRD); // ��������� ������ ����� ������������ ���������� �������
	SetClusterData(CTL_REPEATING_PRD, P_Data->Prd);
	embedChild(P_Data->Prd);
	CALLPTRMEMB(P_ChildDlg, TransmitData(+1, P_Data));
	return 1;
}

int RepeatingDialog::setDuration(long sec)
{
	if(Options & fEditDuration) {
		LTIME tm;
		tm.settotalsec(sec);
		setCtrlData(CTL_REPEATING_DURATION, &tm);
		return 1;
	}
	else
		return 0;
}

int RepeatingDialog::getDuration(long * pSec)
{
	if(Options & fEditDuration) {
		LTIME tm;
		tm.settotalsec(0);
		getCtrlData(CTL_REPEATING_DURATION, &tm);
		ASSIGN_PTR(pSec, tm.totalsec());
		return 1;
	}
	else
		return 0;
}

int RepeatingDialog::getDTS(DateRepeating * pData)
{
	int    ok = 1;
	long   temp_long = static_cast<long>(P_Data->Prd);
	GetClusterData(CTL_REPEATING_PRD, &temp_long);
	P_Data->Prd = static_cast<int16>(temp_long);
	if(P_ChildDlg)
		if(P_ChildDlg->IsCommandValid(cmOK))
			P_ChildDlg->TransmitData(-1, P_Data);
		else
			ok = PPErrorZ();
	if(ok) {
		if(Options & fEditTime) {
			getCtrlData(CTL_REPEATING_RUNTIME, &static_cast<DateTimeRepeating *>(P_Data)->Time);
			if(pData)
				*static_cast<DateTimeRepeating *>(pData) = *static_cast<const DateTimeRepeating *>(P_Data);
		}
		else
			ASSIGN_PTR(pData, *P_Data);
	}
	return ok;
}
