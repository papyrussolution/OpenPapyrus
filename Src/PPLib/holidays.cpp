// HOLIDAYS.CPP
// Copyright (c) A.Sobolev 2004, 2005, 2006, 2007, 2008, 2009, 2012, 2015, 2016, 2017
//
#include <pp.h>
#pragma hdrstop

//static
int SLAPI PPHolidays::GetKind(LDATE dt)
{
	int    d, m, y;
	decodedate(&d, &m, &y, &dt);
	if(!dt)
		return 1;
	else if(d >= 1 && d <= 7 && m == 0 && y == 0) // day of week
		return 3;
	else if(y == 0) // calendar date
		return 2;
	else // simple date
		return 1;
}

// static
SString & SLAPI PPHolidays::Format(LDATE dt, SString & rBuf)
{
	rBuf = 0;
	int    d, m, y;
	decodedate(&d, &m, &y, &dt);
	if(d >= 1 && d <= 7 && m == 0 && y == 0) { // day of week
		GetDayOfWeekText(dowtRuShrt, d, rBuf);
		rBuf.Transf(CTRANSF_OUTER_TO_INNER);
	}
	else if(y == 0) { // calendar date
		SString temp_buf;
		SGetMonthText(m, MONF_SHORT|MONF_OEM, temp_buf);
		rBuf.Cat(d).Space().Cat(/*getMonthText(m, MONF_SHORT|MONF_OEM, temp_buf)*/temp_buf);
	}
	else // simple date
		rBuf.Cat(dt, DATF_DMY);
	return rBuf;
}
//
//
//
class HldDialog : public TDialog {
public:
	HldDialog() : TDialog(DLG_HOLIDAY)
	{
		SetupCalDate(CTLCAL_HOLIDAY_DATE, CTL_HOLIDAY_DATE);
	}
	int    setDTS(LDATE dt, PPID locID);
	int    getDTS(LDATE * pDt);
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCmd(cmClusterClk)) {
			setupDate();
			clearEvent(event);
		}
	}
	int    setupDate();

	LDATE  Dt;
	int    PrevKind;
};

int HldDialog::setupDate()
{
	int    kind = (getCtrlUInt16(CTL_HOLIDAY_KIND) + 1);
	long   dw = dayofweek(&Dt, 1);
	if(kind != PrevKind) {
		LDATE dt = ZERODATE;
		setCtrlData(CTL_HOLIDAY_DATE, &dt);
		setCtrlData(CTLSEL_HOLIDAY_DAYOFWEEK, &dt);
		Dt = dt;
	}
	else if(kind == 1) {
		setCtrlData(CTL_HOLIDAY_DATE, &Dt);
		setCtrlData(CTLSEL_HOLIDAY_DAYOFWEEK, &dw);
	}
	else if(kind == 2) {
		getCtrlData(CTL_HOLIDAY_DATE, &Dt);
		if(Dt != ZERODATE) {
			int    d, m, y;
			decodedate(&d, &m, &y, &Dt);
			char   temp_buf[32];
			char * p = temp_buf;
			p += strlen(itoa(d, p, 10));
			*p++ = '/';
			itoa(m, p, 10);
			TInputLine * p_il = (TInputLine *)getCtrlView(CTL_HOLIDAY_DATE);
			CALLPTRMEMB(p_il, setText(temp_buf));
		}
		setCtrlData(CTLSEL_HOLIDAY_DAYOFWEEK, &(dw = 0));
	}
	else if(kind == 3) {
		LDATE  dt = ZERODATE;
		setCtrlData(CTL_HOLIDAY_DATE, &dt);
		setCtrlData(CTLSEL_HOLIDAY_DAYOFWEEK, &dw);
	}
	disableCtrl(CTLSEL_HOLIDAY_DAYOFWEEK, kind != 3);
	disableCtrls(kind == 3, CTL_HOLIDAY_DATE, CTLCAL_HOLIDAY_DATE, 0);
	PrevKind = kind;
	return 1;
}

int HldDialog::setDTS(LDATE dt, PPID locID)
{
	Dt = dt;
	SString loc_name;
	int    kind = PPHolidays::GetKind(Dt);
	PrevKind = kind;
	setCtrlUInt16(CTL_HOLIDAY_KIND, kind - 1);
	GetExtLocationName(locID, loc_name);
	setStaticText(CTL_HOLIDAY_ST_LOC, loc_name);
	SetupStringCombo(this, CTLSEL_HOLIDAY_DAYOFWEEK, PPTXT_WEEKDAYS, 0);
	setupDate();
	return 1;
}

int HldDialog::getDTS(LDATE * pDt)
{
	int    ok = 1;
	uint   sel = 0;
	ushort v = getCtrlUInt16(CTL_HOLIDAY_KIND);
	int    kind = (v + 1);
	if(kind == 1) {
		getCtrlData(sel = CTL_HOLIDAY_DATE, &Dt);
		if(!checkdate(Dt, 0))
			ok = PPSetErrorSLib();
	}
	else if(kind == 2) {
		int    d, m, y;
		getCtrlData(sel = CTL_HOLIDAY_DATE, &Dt);
		if(!checkdate(Dt, 0))
			ok = PPSetErrorSLib();
		else {
			decodedate(&d, &m, &y, &Dt);
			encodedate(d, m, 0, &Dt);
		}
	}
	else if(kind == 3) {
		long   dw = 0;
		getCtrlData(sel = CTL_HOLIDAY_DAYOFWEEK, &dw);
		if(dw <= 0 || dw > 7)
			ok = PPSetError(PPERR_INVDAYOFWEEK);
		else
			encodedate((int)dw, 0, 0, &Dt);
	}
	if(!ok)
		PPErrorByDialog(this, sel);
	else
		ASSIGN_PTR(pDt, Dt);
	return ok;
}

static int SLAPI EditHoliday(PPID locID, LDATE * pDt)
{
	int    ok = -1;
	HldDialog * dlg = new HldDialog();
	if(CheckDialogPtr(&dlg, 1)) {
		dlg->setDTS(*pDt, locID);
		for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
			if(dlg->getDTS(pDt))
				ok = valid_data = 1;
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}
//
//
//
class HolidaysDialog : public PPListDialog {
public:
	HolidaysDialog() : PPListDialog(DLG_HOLIDAYS, CTL_HOLIDAYS_LIST)
	{
		P_Driver = 0;
	}
	int    setDTS(PPID initLocID, PPHolidays * pDriver)
	{
		P_Driver = pDriver;
		SetupPPObjCombo(this, CTLSEL_HOLIDAYS_LOC, PPOBJ_LOCATION, initLocID, 0, 0);
		updateList(-1);
		return 1;
	}
private:
	DECL_HANDLE_EVENT;
	virtual int setupList();
	virtual int addItem(long * pPos, long * pID);
	virtual int delItem(long pos, long id);
	void   scan();
	void   test()
	{
	}
	PPHolidays * P_Driver;
};

void HolidaysDialog::scan()
{
	if(P_Driver) {
		PPPredictConfig cfg;
		DateRange period;
		period.SetZero();
		PrcssrPrediction::GetPredictCfg(&cfg);
		PPID   loc_id = getCtrlLong(CTLSEL_HOLIDAYS_LOC);
		if(loc_id && cfg.OpID && DateRangeDialog(0, 0, &period) > 0)
			if(BillObj->P_Tbl->ScanHolidays(loc_id, cfg.OpID, &period, P_Driver) > 0)
				updateList(-1);
	}
}

IMPL_HANDLE_EVENT(HolidaysDialog)
{
	PPListDialog::handleEvent(event);
	if(event.isCmd(cmHolidaysScan))
		scan();
	else if(event.isCmd(cmHolidaysTest))
		test();
	else if(event.isCbSelected(CTLSEL_HOLIDAYS_LOC)) {
		PPID   loc_id = getCtrlLong(CTLSEL_HOLIDAYS_LOC);
		enableCommand(cmHolidaysScan, loc_id != 0);
		enableCommand(cmHolidaysTest, loc_id != 0);
		updateList(-1);
	}
	else
		return;
	clearEvent(event);
}

int HolidaysDialog::setupList()
{
	if(P_Driver) {
		SString sub;
		PPID   loc_id = getCtrlLong(CTLSEL_HOLIDAYS_LOC);
		for(LDATE dt = ZERODATE; P_Driver->EnumHolidays(loc_id, &dt) > 0;)
			if(!addStringToList(dt, PPHolidays::Format(dt, sub)))
				return 0;
	}
	return 1;
}

int HolidaysDialog::addItem(long * /*pPos*/, long * pID)
{
	int    ok = -1, r;
	PPID   loc_id = 0;
	LDATE  dt = ZERODATE;
	THROW(CheckCfgRights(PPCFGOBJ_PREDICTHOLYDAYS, PPR_MOD, 0));
	getCtrlData(CTLSEL_HOLIDAYS_LOC, &loc_id);
	while(ok < 0 && EditHoliday(loc_id, &dt) > 0) {
		if((r = P_Driver->SetHoliday(loc_id, dt, 0)) > 0) {
			ASSIGN_PTR(pID, dt);
			ok = 1;
		}
		else if(r < 0)
			PPError(PPERR_DUPHOLIDAY, 0);
		else
			PPError();
	}
	CATCHZOKPPERR
	return ok;
}

int HolidaysDialog::delItem(long /*pos*/, long id)
{
	int    ok = -1;
	PPID   loc_id = 0;
	LDATE dt = *(LDATE *)&id;
	THROW(CheckCfgRights(PPCFGOBJ_PREDICTHOLYDAYS, PPR_MOD, 0));
	getCtrlData(CTLSEL_HOLIDAYS_LOC, &loc_id);
	THROW((ok = P_Driver->SetHoliday(loc_id, dt, 1)) > 0)
	CATCHZOKPPERR
	return ok;
}

int SLAPI EditHolidays()
{
	int    ok = -1;
	PredictSalesCore t;
	HolidaysDialog * dlg = new HolidaysDialog();
	THROW(CheckCfgRights(PPCFGOBJ_PREDICTHOLYDAYS, PPR_READ, 0));
	THROW(CheckDialogPtr(&dlg, 0));
	dlg->setDTS(LConfig.Location, &t);
	while(ok <= 0 && ExecView(dlg) == cmOK) {
		THROW(CheckCfgRights(PPCFGOBJ_PREDICTHOLYDAYS, PPR_MOD, 0));
		if(t.Finish(0, 1)) {
			ok = 1;
			// @todo Перенести эту строку в функцию PredictSalesCore::Finish
			DS.LogAction(PPACN_CONFIGUPDATED, PPCFGOBJ_PREDICTHOLYDAYS, 0, 0, 1);
		}
		else
			ok = PPErrorZ();
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

